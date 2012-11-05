/*
 * Copyright (c) 2012, The University of Oxford
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

#ifndef OSKAR_DFT_C2R_2D_CUDA_H_
#define OSKAR_DFT_C2R_2D_CUDA_H_

/**
 * @file oskar_dft_c2r_2d_cuda.h
 */

#include "oskar_global.h"
#include "utility/oskar_vector_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief
 * CUDA wrapper to perform a 2D complex-to-real single-precision DFT.
 *
 * @details
 * Computes a real output from a set of complex input data, using CUDA to
 * evaluate a 2D Direct Fourier Transform (DFT).
 *
 * Note that all pointers are device pointers, and must not be dereferenced
 * in host code.
 *
 * This function must be supplied with the input x- and y-positions, and the
 * output x- and y-positions. The input positions must be pre-multiplied by a
 * factor k (= 2pi / lambda), and the output positions are direction cosines.
 *
 * The fastest-varying dimension in the output array is along x. The output is
 * assumed to be completely real, so the conjugate copy of the input data
 * should not be supplied.
 *
 * @param[in] n_in         Number of input points.
 * @param[in] x_in         Array of input x positions.
 * @param[in] y_in         Array of input y positions.
 * @param[in] data_in      Array of complex input data (length 2 * n_in).
 * @param[in] n_out        Number of output points.
 * @param[in] x_out        Array of output 1/x positions.
 * @param[in] y_out        Array of output 1/y positions.
 * @param[out] output      Array of computed output points.
 */
OSKAR_EXPORT
void oskar_dft_c2r_2d_cuda_f(int n_in, const float* x_in, const float* y_in,
        const float2* data_in, int n_out, const float* x_out,
        const float* y_out, float* output);

/**
 * @brief
 * CUDA wrapper to perform a 2D complex-to-real double-precision DFT.
 *
 * @details
 * Computes a real output from a set of complex input data, using CUDA to
 * evaluate a 2D Direct Fourier Transform (DFT).
 *
 * Note that all pointers are device pointers, and must not be dereferenced
 * in host code.
 *
 * This function must be supplied with the input x- and y-positions, and the
 * output x- and y-positions. The input positions must be pre-multiplied by a
 * factor k (= 2 pi / lambda), and the output positions are direction cosines.
 *
 * The fastest-varying dimension in the output array is along x. The output is
 * assumed to be completely real, so the conjugate copy of the input data
 * should not be supplied.
 *
 * @param[in] n_in         Number of input points.
 * @param[in] x_in         Array of input x positions.
 * @param[in] y_in         Array of input y positions.
 * @param[in] data_in      Array of complex input data (length 2 * n_in).
 * @param[in] n_out        Number of output points.
 * @param[in] x_out        Array of output 1/x positions.
 * @param[in] y_out        Array of output 1/y positions.
 * @param[out] output      Array of computed output points.
 */
OSKAR_EXPORT
void oskar_dft_c2r_2d_cuda_d(int n_in, const double* x_in, const double* y_in,
        const double2* data_in, int n_out, const double* x_out,
        const double* y_out, double* output);

#ifdef __CUDACC__

/**
 * @brief
 * CUDA kernel to perform a 2D complex-to-real single-precision DFT.
 *
 * @details
 * This CUDA kernel performs a 2D complex-to-real DFT.
 *
 * Each thread evaluates a single output point, looping over all the input
 * points while performing a complex multiply-accumulate with the input
 * DFT weights. The output values are assumed to be completely real,
 * so the Hermitian copies should not be passed in the input data, and the
 * imaginary part of the output is not evaluated.
 *
 * The input positions must be pre-multiplied by a factor k (= 2pi / lambda).
 *
 * The computed points are returned in the \p output array, which must be
 * pre-sized to length n_out. The returned values are not normalised to the
 * number of input points.
 *
 * The kernel requires max_in_chunk * sizeof(float4) bytes of shared memory.
 *
 * ============================================================================
 * WARNING: Changed the sign of the DFT to negative for the 2.0.0-beta release
 * to resolve image ordering problem when writing FITS files.
 * This should be thought of as a hack as we find no clear justification for
 * the phase of the DFT to have a negative sign.
 * ============================================================================
 *
 * @param[in] n_in         Number of input points.
 * @param[in] x_in         Array of input x positions.
 * @param[in] y_in         Array of input y positions.
 * @param[in] data_in      Array of complex input data.
 * @param[in] n_out        Number of output points.
 * @param[in] x_out        Array of output 1/x positions.
 * @param[in] y_out        Array of output 1/y positions.
 * @param[in] max_in_chunk Maximum input points per chunk.
 * @param[out] output      Array of computed output points.
 */
OSKAR_EXPORT
__global__
void oskar_dft_c2r_2d_cudak_f(int n_in, const float* x_in,
        const float* y_in, const float2* data_in, const int n_out,
        const float* x_out, const float* y_out, const int max_in_chunk,
        float* output);

/**
 * @brief
 * CUDA kernel to perform a 2D complex-to-real double-precision DFT.
 *
 * @details
 * This CUDA kernel performs a 2D complex-to-real DFT.
 *
 * Each thread evaluates a single output point, looping over all the input
 * points while performing a complex multiply-accumulate with the input
 * DFT weights. The output values are assumed to be completely real,
 * so the Hermitian copies should not be passed in the input data, and the
 * imaginary part of the output is not evaluated.
 *
 * The input positions must be pre-multiplied by a factor k (= 2pi / lambda).
 *
 * The computed points are returned in the \p output array, which must be
 * pre-sized to length n_out. The returned values are not normalised to the
 * number of input points.
 *
 * The kernel requires max_in_chunk * sizeof(double4) bytes of shared memory.
 *
 * ============================================================================
 * WARNING: Changed the sign of the DFT to negative for the 2.0.0-beta release
 * to resolve image ordering problem when writing FITS files.
 * This should be thought of as a hack as we find no clear justification for
 * the phase of the DFT to have a negative sign.
 * ============================================================================
 *
 * @param[in] n_in         Number of input points.
 * @param[in] x_in         Array of input x positions.
 * @param[in] y_in         Array of input y positions.
 * @param[in] data_in      Array of complex input data.
 * @param[in] n_out        Number of output points.
 * @param[in] x_out        Array of output 1/x positions.
 * @param[in] y_out        Array of output 1/y positions.
 * @param[in] max_in_chunk Maximum input points per chunk.
 * @param[out] output      Array of computed output points.
 */
OSKAR_EXPORT
__global__
void oskar_dft_c2r_2d_cudak_d(int n_in, const double* x_in,
        const double* y_in, const double2* data_in, const int n_out,
        const double* x_out, const double* y_out, const int max_in_chunk,
        double* output);

#endif /* __CUDACC__ */

#ifdef __cplusplus
}
#endif

#endif /* OSKAR_DFT_C2R_2D_CUDA_H_ */