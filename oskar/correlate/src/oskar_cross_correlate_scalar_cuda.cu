/*
 * Copyright (c) 2011-2018, The University of Oxford
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the University of Oxford nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "correlate/oskar_cross_correlate_scalar_cuda.h"
#include "correlate/private_correlate_functions_inline.h"
#include "utility/oskar_device_utils.h"
#include <cuda_runtime.h>

// Indices into the visibility/baseline matrix.
#define SP blockIdx.x /* Column index. */
#define SQ blockIdx.y /* Row index. */

enum {
    VER_UNKNOWN    = -1,  // Not checked.
    VER_NONE       =  0,  // Not specified.
    // Actual versions:
    VER_OLD        =  1,
    VER_NON_SM     =  2,
    VER_SM         =  3
};
static int ver_specified_ = VER_UNKNOWN;
static int ver_cc_ = VER_UNKNOWN;
static int correlate_version(bool prec_double,
        double frac_bandwidth, double time_int_sec);

template
<
// Compile-time parameters.
bool BANDWIDTH_SMEARING, bool TIME_SMEARING, bool GAUSSIAN,
typename REAL, typename REAL2
>
__global__
void oskar_xcorr_scalar_cudak(
        const int                   num_sources,
        const int                   num_stations,
        const REAL2* const restrict jones,
        const REAL*  const restrict source_I,
        const REAL*  const restrict source_l,
        const REAL*  const restrict source_m,
        const REAL*  const restrict source_n,
        const REAL*  const restrict source_a,
        const REAL*  const restrict source_b,
        const REAL*  const restrict source_c,
        const REAL*  const restrict station_u,
        const REAL*  const restrict station_v,
        const REAL*  const restrict station_w,
        const REAL*  const restrict station_x,
        const REAL*  const restrict station_y,
        const REAL                  uv_min_lambda,
        const REAL                  uv_max_lambda,
        const REAL                  inv_wavelength,
        const REAL                  frac_bandwidth,
        const REAL                  time_int_sec,
        const REAL                  gha0_rad,
        const REAL                  dec0_rad,
        REAL2*             restrict vis)
{
    extern __shared__ __align__(sizeof(double2)) unsigned char my_smem[];
    __shared__ REAL uv_len, uu, vv, ww, uu2, vv2, uuvv, du, dv, dw;
    REAL2 t1, t2, sum; // Partial sum per thread.
    REAL2* smem = reinterpret_cast<REAL2*>(my_smem); // Allows template.

    // Return immediately if in the wrong half of the visibility matrix.
    if (SQ >= SP) return;

    // Get common baseline values per thread block.
    if (threadIdx.x == 0)
    {
        OSKAR_BASELINE_TERMS(REAL, station_u[SP], station_u[SQ],
                station_v[SP], station_v[SQ], station_w[SP], station_w[SQ],
                uu, vv, ww, uu2, vv2, uuvv, uv_len);

        if (TIME_SMEARING)
            OSKAR_BASELINE_DELTAS(REAL, station_x[SP], station_x[SQ],
                    station_y[SP], station_y[SQ], du, dv, dw);
    }
    __syncthreads();

    // Apply the baseline length filter.
    if (uv_len < uv_min_lambda || uv_len > uv_max_lambda) return;

    // Get pointers to source vectors for both stations.
    const REAL2* const restrict station_p = &jones[num_sources * SP];
    const REAL2* const restrict station_q = &jones[num_sources * SQ];

    // Each thread loops over a subset of the sources.
    sum.x = sum.y = (REAL) 0;
    for (int i = threadIdx.x; i < num_sources; i += blockDim.x)
    {
        REAL smearing;
        if (GAUSSIAN)
        {
            const REAL t = source_a[i] * uu2 + source_b[i] * uuvv +
                    source_c[i] * vv2;
            smearing = exp((REAL) -t);
        }
        else
        {
            smearing = (REAL) 1;
        }
        smearing *= source_I[i];
        if (BANDWIDTH_SMEARING || TIME_SMEARING)
        {
            const REAL l = source_l[i];
            const REAL m = source_m[i];
            const REAL n = source_n[i] - (REAL) 1;
            if (BANDWIDTH_SMEARING)
            {
                const REAL t = uu * l + vv * m + ww * n;
                smearing *= oskar_sinc<REAL>(t);
            }
            if (TIME_SMEARING)
            {
                const REAL t = du * l + dv * m + dw * n;
                smearing *= oskar_sinc<REAL>(t);
            }
        }

        // Multiply Jones scalars.
        t1 = station_p[i];
        t2 = station_q[i];
        OSKAR_MUL_COMPLEX_CONJUGATE_IN_PLACE(REAL2, t1, t2)

        // Multiply result by smearing term and accumulate.
        sum.x += t1.x * smearing; sum.y += t1.y * smearing;
    }

    // Store partial sum for the thread in shared memory.
    smem[threadIdx.x] = sum;
    __syncthreads();

    // Accumulate contents of shared memory.
    if (threadIdx.x == 0)
    {
        // Sum over all sources for this baseline.
        for (int i = 1; i < blockDim.x; ++i)
        {
            sum.x += smem[i].x; sum.y += smem[i].y;
        }

        // Add result of this thread block to the baseline visibility.
        int i = oskar_evaluate_baseline_index_inline(num_stations, SP, SQ);
        vis[i].x += sum.x; vis[i].y += sum.y;
    }
}

#define OKN_NSOURCES 32
#define OKN_BPK 4 /* baselines per kernel */
#define WARP 32

template
<
// Compile-time parameters.
bool BANDWIDTH_SMEARING, bool TIME_SMEARING, bool GAUSSIAN,
typename REAL, typename REAL2
>
__global__
void oskar_xcorr_scalar_NON_SM_cudak(
        const int                   num_sources,
        const int                   num_stations,
        const REAL2* const restrict jones,
        const REAL*  const restrict source_I,
        const REAL*  const restrict source_l,
        const REAL*  const restrict source_m,
        const REAL*  const restrict source_n,
        const REAL*  const restrict source_a,
        const REAL*  const restrict source_b,
        const REAL*  const restrict source_c,
        const REAL*  const restrict station_u,
        const REAL*  const restrict station_v,
        const REAL*  const restrict station_w,
        const REAL*  const restrict station_x,
        const REAL*  const restrict station_y,
        const REAL                  uv_min_lambda,
        const REAL                  uv_max_lambda,
        const REAL                  inv_wavelength,
        const REAL                  frac_bandwidth,
        const REAL                  time_int_sec,
        const REAL                  gha0_rad,
        const REAL                  dec0_rad,
        REAL2*             restrict vis)
{
    __shared__ REAL uv_len[OKN_BPK], uu[OKN_BPK], vv[OKN_BPK], ww[OKN_BPK];
    __shared__ REAL uu2[OKN_BPK], vv2[OKN_BPK], uuvv[OKN_BPK];
    __shared__ REAL du[OKN_BPK], dv[OKN_BPK], dw[OKN_BPK];
    __shared__ const REAL2 *station_q[OKN_BPK];
    REAL2 t1, t2, sum;

    const int w = (threadIdx.x >> 5); // Warp ID.
    const int i = (threadIdx.x & 31); // ID within warp (local ID).

    // Return immediately if in the wrong half of the visibility matrix.
    if (OKN_BPK * SQ >= SP) return;

    // Get baseline values per warp.
    if (i == 0)
    {
        const int i_sq = OKN_BPK * SQ + w;

        // Set pointer to source vector for station q to safe position
        // so non-existence SQ >= SP does not cause problems.
        station_q[w] = &jones[0];

        if (i_sq < num_stations)
        {
            OSKAR_BASELINE_TERMS(REAL,
                    station_u[SP], station_u[i_sq],
                    station_v[SP], station_v[i_sq],
                    station_w[SP], station_w[i_sq],
                    uu[w], vv[w], ww[w], uu2[w], vv2[w], uuvv[w], uv_len[w]);

            if (TIME_SMEARING)
                OSKAR_BASELINE_DELTAS(REAL,
                        station_x[SP], station_x[i_sq],
                        station_y[SP], station_y[i_sq],
                        du[w], dv[w], dw[w]);

            // Get valid pointer to source vector for station q.
            station_q[w] = &jones[num_sources * i_sq];
        }
    }
    __syncthreads();

    // Get pointer to source vector for station p.
    const REAL2* const restrict station_p = &jones[num_sources * SP];

    // Each thread from given warp loops over a subset of the sources,
    // and each warp works with a different station q.
    sum.x = sum.y = (REAL)0;
    int itemp = (num_sources >> 5) * WARP;
    for (int outer = i; outer < itemp; outer += WARP)
    {
        REAL smearing;
        if (GAUSSIAN)
        {
            const REAL t = source_a[outer] * uu2[w] +
                    source_b[outer] * uuvv[w] + source_c[outer] * vv2[w];
            smearing = exp((REAL) -t);
        }
        else
        {
            smearing = (REAL) 1;
        }
        smearing *= source_I[outer];
        if (BANDWIDTH_SMEARING || TIME_SMEARING)
        {
            const REAL l = source_l[outer];
            const REAL m = source_m[outer];
            const REAL n = source_n[outer] - (REAL) 1;
            if (BANDWIDTH_SMEARING)
            {
                const REAL t = uu[w] * l + vv[w] * m + ww[w] * n;
                smearing *= oskar_sinc<REAL>(t);
            }
            if (TIME_SMEARING)
            {
                const REAL t = du[w] * l + dv[w] * m + dw[w] * n;
                smearing *= oskar_sinc<REAL>(t);
            }
        }

        // Multiply Jones scalars.
        t1 = station_p[outer];
        t2 = (station_q[w])[outer];
        OSKAR_MUL_COMPLEX_CONJUGATE_IN_PLACE(REAL2, t1, t2)

        // Multiply result by smearing term and accumulate.
        sum.x += t1.x * smearing; sum.y += t1.y * smearing;
        __syncthreads();
    }
    if ((num_sources & 31) > 0)
    {
        int outer = (num_sources >> 5) * WARP + i;
        if (outer < num_sources)
        {
            REAL smearing;
            if (GAUSSIAN)
            {
                const REAL t = source_a[outer] * uu2[w] +
                        source_b[outer] * uuvv[w] + source_c[outer] * vv2[w];
                smearing = exp((REAL) -t);
            }
            else
            {
                smearing = (REAL) 1;
            }
            smearing *= source_I[outer];
            if (BANDWIDTH_SMEARING || TIME_SMEARING)
            {
                const REAL l = source_l[outer];
                const REAL m = source_m[outer];
                const REAL n = source_n[outer] - (REAL) 1;
                if (BANDWIDTH_SMEARING)
                {
                    const REAL t = uu[w] * l + vv[w] * m + ww[w] * n;
                    smearing *= oskar_sinc<REAL>(t);
                }
                if (TIME_SMEARING)
                {
                    const REAL t = du[w] * l + dv[w] * m + dw[w] * n;
                    smearing *= oskar_sinc<REAL>(t);
                }
            }

            // Multiply Jones scalars.
            t1 = station_p[outer];
            t2 = (station_q[w])[outer];
            OSKAR_MUL_COMPLEX_CONJUGATE_IN_PLACE(REAL2, t1, t2)

            // Multiply result by smearing term and accumulate.
            sum.x += t1.x * smearing; sum.y += t1.y * smearing;
        }
    }

    // Reduce results within warp.
    OSKAR_WARP_REDUCE(sum.x);
    OSKAR_WARP_REDUCE(sum.y);

    // Add result of this warp to the baseline visibility.
    if (i == 0 && (OKN_BPK * SQ + w) < SP)
    {
        if (uv_len[w] < uv_min_lambda || uv_len[w] > uv_max_lambda) return;
        const int j = oskar_evaluate_baseline_index_inline(num_stations,
                SP, OKN_BPK * SQ + w);
        vis[j].x += sum.x; vis[j].y += sum.y;
    }
}

template
<
// Compile-time parameters.
bool BANDWIDTH_SMEARING, bool TIME_SMEARING, bool GAUSSIAN,
typename REAL, typename REAL2
>
__global__
void oskar_xcorr_scalar_SM_cudak(
        const int                   num_sources,
        const int                   num_stations,
        const REAL2* const restrict jones,
        const REAL*  const restrict source_I,
        const REAL*  const restrict source_l,
        const REAL*  const restrict source_m,
        const REAL*  const restrict source_n,
        const REAL*  const restrict source_a,
        const REAL*  const restrict source_b,
        const REAL*  const restrict source_c,
        const REAL*  const restrict station_u,
        const REAL*  const restrict station_v,
        const REAL*  const restrict station_w,
        const REAL*  const restrict station_x,
        const REAL*  const restrict station_y,
        const REAL                  uv_min_lambda,
        const REAL                  uv_max_lambda,
        const REAL                  inv_wavelength,
        const REAL                  frac_bandwidth,
        const REAL                  time_int_sec,
        const REAL                  gha0_rad,
        const REAL                  dec0_rad,
        REAL2*             restrict vis)
{
    __shared__ REAL uv_len[OKN_BPK], uu[OKN_BPK], vv[OKN_BPK], ww[OKN_BPK];
    __shared__ REAL uu2[OKN_BPK], vv2[OKN_BPK], uuvv[OKN_BPK];
    __shared__ REAL du[OKN_BPK], dv[OKN_BPK], dw[OKN_BPK];
    __shared__ const REAL2 *station_q[OKN_BPK];
    __shared__ REAL   s_I[OKN_NSOURCES];
    __shared__ REAL   s_l[OKN_NSOURCES];
    __shared__ REAL   s_m[OKN_NSOURCES];
    __shared__ REAL   s_n[OKN_NSOURCES];
    __shared__ REAL   s_a[OKN_NSOURCES];
    __shared__ REAL   s_b[OKN_NSOURCES];
    __shared__ REAL   s_c[OKN_NSOURCES];
    __shared__ REAL2 s_sp[OKN_NSOURCES];
    REAL2 t1, t2, sum;

    const int w = (threadIdx.x >> 5); // Warp ID.
    const int i = (threadIdx.x & 31); // ID within warp (local ID).

    // Return immediately if in the wrong half of the visibility matrix.
    if (OKN_BPK * SQ >= SP) return;

    // Get baseline values per warp.
    if (i == 0)
    {
        const int i_sq = OKN_BPK * SQ + w;

        // Set pointer to source vector for station q to safe position
        // so non-existence SQ >= SP does not cause problems.
        station_q[w] = &jones[0];

        if (i_sq < num_stations)
        {
            OSKAR_BASELINE_TERMS(REAL,
                    station_u[SP], station_u[i_sq],
                    station_v[SP], station_v[i_sq],
                    station_w[SP], station_w[i_sq],
                    uu[w], vv[w], ww[w], uu2[w], vv2[w], uuvv[w], uv_len[w]);

            if (TIME_SMEARING)
                OSKAR_BASELINE_DELTAS(REAL,
                        station_x[SP], station_x[i_sq],
                        station_y[SP], station_y[i_sq],
                        du[w], dv[w], dw[w]);

            // Get valid pointer to source vector for station q.
            station_q[w] = &jones[num_sources * i_sq];
        }
    }
    __syncthreads();

    // Get pointer to source vector for station p.
    const REAL2* const restrict station_p = &jones[num_sources * SP];

    // Each thread from given warp loops over a subset of the sources,
    // and each warp works with a different station q.
    sum.x = sum.y = (REAL)0;
    int itemp = (num_sources >> 5) * WARP;
    for (int outer = i; outer < itemp; outer += WARP)
    {
        if (w == 0)
        {
            s_I[i] = source_I[outer];
            if (BANDWIDTH_SMEARING || TIME_SMEARING)
                s_l[i] = source_l[outer];
            if (GAUSSIAN)
            {
                s_a[i] = source_a[outer];
                s_b[i] = source_b[outer];
            }
        }
        if (w == 1)
        {
            if (BANDWIDTH_SMEARING || TIME_SMEARING)
                s_m[i] = source_m[outer];
            if (GAUSSIAN)
                s_c[i] = source_c[outer];
        }
        if (w == 2)
        {
            if (BANDWIDTH_SMEARING || TIME_SMEARING)
                s_n[i] = source_n[outer];
        }
        if (w == 3)
        {
            s_sp[i] = station_p[outer];
        }
        __syncthreads();

        REAL smearing;
        if (GAUSSIAN)
        {
            const REAL t = s_a[i] * uu2[w] +
                    s_b[i] * uuvv[w] + s_c[i] * vv2[w];
            smearing = exp((REAL) -t);
        }
        else
        {
            smearing = (REAL) 1;
        }
        smearing *= s_I[i];
        if (BANDWIDTH_SMEARING || TIME_SMEARING)
        {
            const REAL l = s_l[i];
            const REAL m = s_m[i];
            const REAL n = s_n[i] - (REAL) 1;
            if (BANDWIDTH_SMEARING)
            {
                const REAL t = uu[w] * l + vv[w] * m + ww[w] * n;
                smearing *= oskar_sinc<REAL>(t);
            }
            if (TIME_SMEARING)
            {
                const REAL t = du[w] * l + dv[w] * m + dw[w] * n;
                smearing *= oskar_sinc<REAL>(t);
            }
        }

        // Multiply Jones scalars.
        t1 = s_sp[i];
        t2 = (station_q[w])[outer];
        OSKAR_MUL_COMPLEX_CONJUGATE_IN_PLACE(REAL2, t1, t2)

        // Multiply result by smearing term and accumulate.
        sum.x += t1.x * smearing; sum.y += t1.y * smearing;
        __syncthreads();
    }
    if ((num_sources & 31) > 0)
    {
        int outer = (num_sources >> 5) * WARP + i;
        if (outer < num_sources)
        {
            if (w == 0)
            {
                s_I[i] = source_I[outer];
                if (BANDWIDTH_SMEARING || TIME_SMEARING)
                    s_l[i] = source_l[outer];
                if (GAUSSIAN)
                {
                    s_a[i] = source_a[outer];
                    s_b[i] = source_b[outer];
                }
            }
            if (w == 1)
            {
                if (BANDWIDTH_SMEARING || TIME_SMEARING)
                    s_m[i] = source_m[outer];
                if (GAUSSIAN)
                    s_c[i] = source_c[outer];
            }
            if (w == 2)
            {
                if (BANDWIDTH_SMEARING || TIME_SMEARING)
                    s_n[i] = source_n[outer];
            }
            if (w == 3)
            {
                s_sp[i] = station_p[outer];
            }
        }
        __syncthreads();
        if (outer < num_sources)
        {
            REAL smearing;
            if (GAUSSIAN)
            {
                const REAL t = s_a[i] * uu2[w] +
                        s_b[i] * uuvv[w] + s_c[i] * vv2[w];
                smearing = exp((REAL) -t);
            }
            else
            {
                smearing = (REAL) 1;
            }
            smearing *= s_I[i];
            if (BANDWIDTH_SMEARING || TIME_SMEARING)
            {
                const REAL l = s_l[i];
                const REAL m = s_m[i];
                const REAL n = s_n[i] - (REAL) 1;
                if (BANDWIDTH_SMEARING)
                {
                    const REAL t = uu[w] * l + vv[w] * m + ww[w] * n;
                    smearing *= oskar_sinc<REAL>(t);
                }
                if (TIME_SMEARING)
                {
                    const REAL t = du[w] * l + dv[w] * m + dw[w] * n;
                    smearing *= oskar_sinc<REAL>(t);
                }
            }

            // Multiply Jones scalars.
            t1 = s_sp[i];
            t2 = (station_q[w])[outer];
            OSKAR_MUL_COMPLEX_CONJUGATE_IN_PLACE(REAL2, t1, t2)

            // Multiply result by smearing term and accumulate.
            sum.x += t1.x * smearing; sum.y += t1.y * smearing;
        }
    }

    // Reduce results within warp.
    OSKAR_WARP_REDUCE(sum.x);
    OSKAR_WARP_REDUCE(sum.y);

    // Add result of this warp to the baseline visibility.
    if (i == 0 && (OKN_BPK * SQ + w) < SP)
    {
        if (uv_len[w] < uv_min_lambda || uv_len[w] > uv_max_lambda) return;
        const int j = oskar_evaluate_baseline_index_inline(num_stations,
                SP, OKN_BPK * SQ + w);
        vis[j].x += sum.x; vis[j].y += sum.y;
    }
}

#define XCORR_KERNEL(NAME, BS, TS, GAUSSIAN, REAL, REAL2)                   \
        NAME<BS, TS, GAUSSIAN, REAL, REAL2>                                 \
        OSKAR_CUDAK_CONF(num_blocks, num_threads, shared_mem)               \
        (num_sources, num_stations, d_jones, d_I, d_l, d_m, d_n,            \
                d_a, d_b, d_c, d_station_u, d_station_v, d_station_w,       \
                d_station_x, d_station_y, uv_min_lambda, uv_max_lambda,     \
                inv_wavelength, frac_bandwidth, time_int_sec,               \
                gha0_rad, dec0_rad, d_vis);

#define XCORR_SELECT(NAME, GAUSSIAN, REAL, REAL2)                           \
        if (frac_bandwidth == (REAL)0 && time_int_sec == (REAL)0)           \
            XCORR_KERNEL(NAME, false, false, GAUSSIAN, REAL, REAL2)         \
        else if (frac_bandwidth != (REAL)0 && time_int_sec == (REAL)0)      \
            XCORR_KERNEL(NAME, true, false, GAUSSIAN, REAL, REAL2)          \
        else if (frac_bandwidth == (REAL)0 && time_int_sec != (REAL)0)      \
            XCORR_KERNEL(NAME, false, true, GAUSSIAN, REAL, REAL2)          \
        else if (frac_bandwidth != (REAL)0 && time_int_sec != (REAL)0)      \
            XCORR_KERNEL(NAME, true, true, GAUSSIAN, REAL, REAL2)

void oskar_cross_correlate_scalar_point_cuda_f(
        int num_sources, int num_stations, const float2* d_jones,
        const float* d_I, const float* d_l,
        const float* d_m, const float* d_n,
        const float* d_station_u, const float* d_station_v,
        const float* d_station_w, const float* d_station_x,
        const float* d_station_y, float uv_min_lambda, float uv_max_lambda,
        float inv_wavelength, float frac_bandwidth, const float time_int_sec,
        const float gha0_rad, const float dec0_rad, float2* d_vis)
{
    const dim3 num_threads(128, 1);
    const float *d_a = 0, *d_b = 0, *d_c = 0;
    const int ver = correlate_version(false, frac_bandwidth, time_int_sec);
    if (ver == VER_NON_SM)
    {
        dim3 num_blocks(num_stations, (num_stations + OKN_BPK - 1) / OKN_BPK);
        const size_t shared_mem = 0;
        XCORR_SELECT(oskar_xcorr_scalar_NON_SM_cudak, false, float, float2)
    }
    else if (ver == VER_SM)
    {
        dim3 num_blocks(num_stations, (num_stations + OKN_BPK - 1) / OKN_BPK);
        const size_t shared_mem = 0;
        XCORR_SELECT(oskar_xcorr_scalar_SM_cudak, false, float, float2)
    }
    else
    {
        dim3 num_blocks(num_stations, num_stations);
        const size_t shared_mem = num_threads.x * sizeof(float2);
        XCORR_SELECT(oskar_xcorr_scalar_cudak, false, float, float2)
    }
}

void oskar_cross_correlate_scalar_point_cuda_d(
        int num_sources, int num_stations, const double2* d_jones,
        const double* d_I, const double* d_l,
        const double* d_m, const double* d_n,
        const double* d_station_u, const double* d_station_v,
        const double* d_station_w, const double* d_station_x,
        const double* d_station_y, double uv_min_lambda, double uv_max_lambda,
        double inv_wavelength, double frac_bandwidth, const double time_int_sec,
        const double gha0_rad, const double dec0_rad, double2* d_vis)
{
    const dim3 num_threads(128, 1);
    const double *d_a = 0, *d_b = 0, *d_c = 0;
    const int ver = correlate_version(true, frac_bandwidth, time_int_sec);
    if (ver == VER_NON_SM)
    {
        dim3 num_blocks(num_stations, (num_stations + OKN_BPK - 1) / OKN_BPK);
        const size_t shared_mem = 0;
        XCORR_SELECT(oskar_xcorr_scalar_NON_SM_cudak, false, double, double2)
    }
    else if (ver == VER_SM)
    {
        dim3 num_blocks(num_stations, (num_stations + OKN_BPK - 1) / OKN_BPK);
        const size_t shared_mem = 0;
        XCORR_SELECT(oskar_xcorr_scalar_SM_cudak, false, double, double2)
    }
    else
    {
        dim3 num_blocks(num_stations, num_stations);
        const size_t shared_mem = num_threads.x * sizeof(double2);
        XCORR_SELECT(oskar_xcorr_scalar_cudak, false, double, double2)
    }
}

void oskar_cross_correlate_scalar_gaussian_cuda_f(
        int num_sources, int num_stations, const float2* d_jones,
        const float* d_I, const float* d_l,
        const float* d_m, const float* d_n,
        const float* d_a, const float* d_b,
        const float* d_c, const float* d_station_u,
        const float* d_station_v, const float* d_station_w,
        const float* d_station_x, const float* d_station_y,
        float uv_min_lambda, float uv_max_lambda, float inv_wavelength,
        float frac_bandwidth, float time_int_sec, float gha0_rad,
        float dec0_rad, float2* d_vis)
{
    const dim3 num_threads(128, 1);
    const int ver = correlate_version(false, frac_bandwidth, time_int_sec);
    if (ver == VER_NON_SM)
    {
        dim3 num_blocks(num_stations, (num_stations + OKN_BPK - 1) / OKN_BPK);
        const size_t shared_mem = 0;
        XCORR_SELECT(oskar_xcorr_scalar_NON_SM_cudak, true, float, float2)
    }
    else if (ver == VER_SM)
    {
        dim3 num_blocks(num_stations, (num_stations + OKN_BPK - 1) / OKN_BPK);
        const size_t shared_mem = 0;
        XCORR_SELECT(oskar_xcorr_scalar_SM_cudak, true, float, float2)
    }
    else
    {
        dim3 num_blocks(num_stations, num_stations);
        const size_t shared_mem = num_threads.x * sizeof(float2);
        XCORR_SELECT(oskar_xcorr_scalar_cudak, true, float, float2)
    }
}

void oskar_cross_correlate_scalar_gaussian_cuda_d(
        int num_sources, int num_stations, const double2* d_jones,
        const double* d_I, const double* d_l,
        const double* d_m, const double* d_n,
        const double* d_a, const double* d_b,
        const double* d_c, const double* d_station_u,
        const double* d_station_v, const double* d_station_w,
        const double* d_station_x, const double* d_station_y,
        double uv_min_lambda, double uv_max_lambda, double inv_wavelength,
        double frac_bandwidth, double time_int_sec, double gha0_rad,
        double dec0_rad, double2* d_vis)
{
    const dim3 num_threads(128, 1);
    const int ver = correlate_version(true, frac_bandwidth, time_int_sec);
    if (ver == VER_NON_SM)
    {
        dim3 num_blocks(num_stations, (num_stations + OKN_BPK - 1) / OKN_BPK);
        const size_t shared_mem = 0;
        XCORR_SELECT(oskar_xcorr_scalar_NON_SM_cudak, true, double, double2)
    }
    else if (ver == VER_SM)
    {
        dim3 num_blocks(num_stations, (num_stations + OKN_BPK - 1) / OKN_BPK);
        const size_t shared_mem = 0;
        XCORR_SELECT(oskar_xcorr_scalar_SM_cudak, true, double, double2)
    }
    else
    {
        dim3 num_blocks(num_stations, num_stations);
        const size_t shared_mem = num_threads.x * sizeof(double2);
        XCORR_SELECT(oskar_xcorr_scalar_cudak, true, double, double2)
    }
}

int correlate_version(bool prec_double,
        double frac_bandwidth, double time_int_sec)
{
    // Check the environment variable if necessary
    // and use the specified version if it has been set.
    if (ver_specified_ == VER_UNKNOWN)
    {
        const char* v = getenv("OSKAR_CORRELATE");
        if (v)
        {
            if (!strcmp(v, "OLD") || !strcmp(v, "old"))
                ver_specified_ = VER_OLD;
            else if (!strcmp(v, "SM") || !strcmp(v, "sm"))
                ver_specified_ = VER_SM;
            else if (strstr(v, "NO") || strstr(v, "no"))
                ver_specified_ = VER_NON_SM;
        }
        if (ver_specified_ == VER_UNKNOWN)
            ver_specified_ = VER_NONE;
    }
    if (ver_specified_ > VER_NONE) return ver_specified_;

    // Check the device compute capability if required.
    if (ver_cc_ == VER_UNKNOWN)
        ver_cc_ = oskar_device_compute_capability();

    // Use non-shared-memory version on Volta.
    if (ver_cc_ >= 70) return VER_NON_SM;

    // Decide which is the best version to use on pre-Volta architectures.
    if (ver_cc_ >= 30)
    {
        const bool smearing = (frac_bandwidth != 0.0 || time_int_sec != 0.0);
        if (prec_double && smearing) return VER_NON_SM;
        if (prec_double && !smearing) return VER_SM;
        if (!prec_double && !smearing) return VER_NON_SM;
    }

    // Otherwise, use the old version.
    return VER_OLD;
}
