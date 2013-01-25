/*
 * Copyright (c) 2013, The University of Oxford
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

#ifndef OSKAR_EVALUATE_ARRAY_PATTERN_H_
#define OSKAR_EVALUATE_ARRAY_PATTERN_H_

/**
 * @file oskar_evaluate_array_pattern.h
 */

#include "oskar_global.h"
#include "utility/oskar_Mem.h"
#include "station/oskar_StationModel.h"
#include "utility/oskar_CurandState.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Evaluates the value of a station beam at a number of discrete
 * positions for the given station and beam direction. This is equivalent
 * to the Array Factor or scalar E-Jones.
 *
 * @details
 * The station beam amplitudes are evaluated using a DFT on the GPU, so
 * all memory passed to and returned from this function must be allocated
 * on the device.
 *
 * Note:
 * - Station x,y,z coordinates used by this function are assumed to be in
 * radians (i.e. pre-multiplied by the wavenumber).
 * - The \p weights buffer must be allocated on the GPU of complex type
 * matching the same floating point precision as the rest of the memory
 * passed to the function.
 *
 * @param[out] beam          Array of station complex beam amplitudes returned.
 * @param[in]  station       Station model structure.
 * @param[in]  beam_x        Beam phase centre component along x.
 * @param[in]  beam_y        Beam phase centre component along y.
 * @param[in]  beam_z        Beam phase centre component along z.
 * @param[in]  num_points    Number of points at which to evaluate beam.
 * @param[in]  x             Array of horizontal x direction components at
 *                           which the beam should be evaluated.
 * @param[in]  y             Array of horizontal y direction components at
 *                           which the beam should be evaluated.
 * @param[in]  z             Array of horizontal z direction components at
 *                           which the beam should be evaluated.
 * @param[in]  weights       Work buffer used to evaluate DFT weights.
 * @param[in]  weights_error Work buffer used to evaluate DFT weights errors.
 * @param[in]  curand_state  Structure holding a set of CURAND states.
 * @param[in,out] status     Status return code.
 */
OSKAR_EXPORT
void oskar_evaluate_array_pattern(oskar_Mem* beam,
        const oskar_StationModel* station, double beam_x, double beam_y,
        double beam_z, int num_points, const oskar_Mem* x, const oskar_Mem* y,
        const oskar_Mem* z, oskar_Mem* weights, oskar_Mem* weights_error,
        oskar_CurandState* curand_state, int* status);

#ifdef __cplusplus
}
#endif

#endif /* OSKAR_EVALUATE_ARRAY_PATTERN_H_ */
