#include "Common.hlsl"
#include "ShaderHeaders/WavefrontStructsGPU.h"

RaytracingAccelerationStructure sceneBVH : register(t0);
StructuredBuffer<Ray> rayBatch : register(t1);
StructuredBuffer<uint> rayCount : register(t2);
RWStructuredBuffer<ExtendResult> extendBatch : register(u0);
RWStructuredBuffer<uint> atomicShadowRays : register(u1);
RWStructuredBuffer<uint> atomicNewRays : register(u2);

[numthreads(256, 1, 1)]
void main(uint3 idx : SV_DispatchThreadID)
{
	// We work only with the active rays
    if (idx.x < rayCount[0])
    {
        RayQuery<RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> query;

        RayDesc ray;
        ray.Origin = rayBatch[idx.x].m_Origin;
        ray.Direction = rayBatch[idx.x].m_Direction;
        ray.TMin = 0.001f;
        ray.TMax = rayBatch[idx.x].m_MaxT;
		extendBatch[idx.x].m_DistanceT = -1.f;

        query.TraceRayInline(
            sceneBVH,
            RAY_FLAG_NONE,
            0xff,
            ray);

        query.Proceed();

        if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
        {
            extendBatch[idx.x].m_BarycentricUV = query.CommittedTriangleBarycentrics();
            extendBatch[idx.x].m_DistanceT = query.CommittedRayT();
            extendBatch[idx.x].m_ModelAndInstanceID = PackUints(query.CommittedInstanceID(), query.CommittedInstanceIndex());
            extendBatch[idx.x].m_PrimitiveId = query.CommittedGeometryIndex();
            extendBatch[idx.x].m_TriangleId = query.CommittedPrimitiveIndex();
            extendBatch[idx.x].m_Padding = float2(0.f, 0.f);
        }
    }
    // Reset shadow rays and bounced rays count
    atomicShadowRays[0] = 0;
    atomicShadowRays[1] = 0;
    atomicNewRays[0] = 0;
}
