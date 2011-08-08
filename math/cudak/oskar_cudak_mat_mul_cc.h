/*
 * Copyright (c) 2011, The University of Oxford
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

#ifndef OSKAR_MATH_CUDAK_MAT_MUL_CC_H_
#define OSKAR_MATH_CUDAK_MAT_MUL_CC_H_

/**
 * @file oskar_math_cudak_mat_mul_cc.h
 */

#include "utility/oskar_cuda_eclipse.h"

/**
 * @brief
 * CUDA kernel to multiply two complex matrices together, element-wise
 * (single precision).
 *
 * @details
 * This CUDA kernel multiplies the elements of two complex matrices together
 * using the graphics card.
 *
 * All matrices must be of the same size.
 *
 * @param[in] n1 Size of the fastest varying dimension.
 * @param[in] n2 Size of the slowest varying dimension.
 * @param[in] a First input matrix.
 * @param[in] b Second input matrix.
 * @param[out] c Output matrix.
 */
__global__
void oskar_cudak_mat_mul_cc_f(int n1, int n2, const float2* a,
        const float2* b, float2* c);

/**
 * @brief
 * CUDA kernel to multiply two complex matrices together, element-wise
 * (double precision).
 *
 * @details
 * This CUDA kernel multiplies the elements of two complex matrices together
 * using the graphics card.
 *
 * All matrices must be of the same size.
 *
 * @param[in] n1 Size of the fastest varying dimension.
 * @param[in] n2 Size of the slowest varying dimension.
 * @param[in] a First input matrix.
 * @param[in] b Second input matrix.
 * @param[out] c Output matrix.
 */
__global__
void oskar_cudak_mat_mul_cc_d(int n1, int n2, const double2* a,
        const double2* b, double2* c);

#endif // OSKAR_MATH_CUDAK_MAT_MUL_CC_H_