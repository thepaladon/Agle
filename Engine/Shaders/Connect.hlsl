#define SHADER_STRUCT 1

#include "ShaderHeaders/WavefrontStructsGPU.h" 

StructuredBuffer<ShadowRay> shadowRayBatch : register(t0);
RaytracingAccelerationStructure sceneBVH : register(t1);
StructuredBuffer<uint> atomicShadowRays : register(t2);
StructuredBuffer<uint> atomicNewRays : register(t3);

RWStructuredBuffer<float4> output : register(u0);
RWStructuredBuffer<uint> rayCount : register(u1);
RWStructuredBuffer<Reservoir> currentReservoirs : register(u2);

ConstantBuffer<ConnectSettings> settings : register(b0);

[numthreads(256, 1, 1)]
void main(uint3 idx : SV_DispatchThreadID)
{
    // In the next extend we will evaluate only the number of bounced rays
    rayCount[0] = atomicNewRays[0];

    
	// Shade only active shadow rays
    if (idx.x < atomicShadowRays[1])
    {
        RayQuery<RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> query;

		// Setup a shadow ray
        RayDesc ray;
        ray.Origin = shadowRayBatch[idx.x].m_Origin.xyz;
        ray.Direction = shadowRayBatch[idx.x].m_Direction.xyz;
        ray.TMin = 0.001f;
        ray.TMax = shadowRayBatch[idx.x].m_DistanceT - 0.001f;
        
        query.TraceRayInline(
            sceneBVH,
            RAY_FLAG_NONE,
            0xff,
            ray);

        query.Proceed();
        
        if (query.CommittedStatus() == COMMITTED_NOTHING)
        {
            if (settings.m_WavefronLoopIdx != 0 || settings.m_UseReSTIR == 0)
            {
                // ToDo, reimplement this or add a flag for it
			    // Remove fireflies, using the threshold
                float colorVal = length(shadowRayBatch[idx.x].m_Energy); ///Todo optimize to squared length
                float3 clampedEnergy = shadowRayBatch[idx.x].m_Energy;
                if (colorVal > settings.m_Threshold)
                {
				// The max energy, we can add = threshold
                    clampedEnergy = normalize(clampedEnergy) * settings.m_Threshold;
                }
            
			    // Add energy of a pixel if it is not shaded
                output[shadowRayBatch[idx.x].m_PixelIdx] += float4(clampedEnergy, 1.0);
            }
        }
        else
        {
            if (settings.m_WavefronLoopIdx == 0)
            {
                // ToDo: Research reset reservoir or set weight to 0?
                currentReservoirs[shadowRayBatch[idx.x].m_PixelIdx].m_Weight = 0.f;
            }
        }
    }
}
