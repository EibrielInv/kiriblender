/* 
 * Adapted from Open Shading Language with this license: 
 * 
 * Copyright (c) 2009-2010 Sony Pictures Imageworks Inc., et al. 
 * All Rights Reserved. 
 * 
 * Modifications Copyright 2011, Blender Foundation. 
 *  
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are 
 * met: 
 * * Redistributions of source code must retain the above copyright 
 *   notice, this list of conditions and the following disclaimer. 
 * * Redistributions in binary form must reproduce the above copyright 
 *   notice, this list of conditions and the following disclaimer in the 
 *   documentation and/or other materials provided with the distribution. 
 * * Neither the name of Sony Pictures Imageworks nor the names of its 
 *   contributors may be used to endorse or promote products derived from 
 *   this software without specific prior written permission. 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
*/

#ifndef __BSDF_REFLECTION_H__
#define __BSDF_REFLECTION_H__

CCL_NAMESPACE_BEGIN

/* REFLECTION */

typedef struct BsdfReflectionClosure {
	//float3 m_N;
} BsdfReflectionClosure;

__device void bsdf_reflection_setup(ShaderData *sd, float3 N)
{
	//BsdfReflectionClosure *self = (BsdfReflectionClosure*)sd->svm_closure_data;
	//self->m_N = N;

	sd->svm_closure = CLOSURE_BSDF_REFLECTION_ID;
}

__device void bsdf_reflection_blur(ShaderData *sd, float roughness)
{
}

__device float3 bsdf_reflection_eval_reflect(const ShaderData *sd, const float3 I, const float3 omega_in, float *pdf)
{
	return make_float3(0.0f, 0.0f, 0.0f);
}

__device float3 bsdf_reflection_eval_transmit(const ShaderData *sd, const float3 I, const float3 omega_in, float *pdf)
{
	return make_float3(0.0f, 0.0f, 0.0f);
}

__device float bsdf_reflection_albedo(const ShaderData *sd, const float3 I)
{
	return 1.0f;
}

__device int bsdf_reflection_sample(const ShaderData *sd, float randu, float randv, float3 *eval, float3 *omega_in, float3 *domega_in_dx, float3 *domega_in_dy, float *pdf)
{
	//const BsdfReflectionClosure *self = (const BsdfReflectionClosure*)sd->svm_closure_data;
	float3 m_N = sd->N;

	// only one direction is possible
	float cosNO = dot(m_N, sd->I);
	if(cosNO > 0) {
		*omega_in = (2 * cosNO) * m_N - sd->I;
		if(dot(sd->Ng, *omega_in) > 0) {
#ifdef __RAY_DIFFERENTIALS__
			*domega_in_dx = 2 * dot(m_N, sd->dI.dx) * m_N - sd->dI.dx;
			*domega_in_dy = 2 * dot(m_N, sd->dI.dy) * m_N - sd->dI.dy;
#endif
			*pdf = 1;
			*eval = make_float3(1, 1, 1);
		}
	}
	return LABEL_REFLECT|LABEL_SINGULAR;
}

CCL_NAMESPACE_END

#endif /* __BSDF_REFLECTION_H__ */

