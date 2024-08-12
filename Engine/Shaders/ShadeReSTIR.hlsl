#include "NEE.hlsl"
#include "ReSTIR.hlsl"

ConstantBuffer<DISeedData> shadeSeedData : register(b0);
ConstantBuffer<ThresholdValue> threshold : register(b1);
ConstantBuffer<ReStirSettings> restirSettings : register(b2);

StructuredBuffer<Ray> rayBatch : register(t0);
StructuredBuffer<uint> atomicShadowRays : register(t1);
StructuredBuffer<LightPickData> lightData : register(t2);
StructuredBuffer<MaterialHitData> materialHits : register(t3);

RWStructuredBuffer<float4> output : register(u0);
RWStructuredBuffer<Reservoir> currentReservoirs : register(u1);
RWStructuredBuffer<Reservoir> previousReservoirs : register(u2);

[numthreads(256, 1, 1)]
void main(uint3 idx : SV_DispatchThreadID)
{
	// Work only with active rays
    if (idx.x < atomicShadowRays[0])
    {
        MaterialHitData materialHitData = materialHits[idx.x];
        Ray ray = rayBatch[materialHitData.m_DiffuseRayID];
		uint pixelIdx = ray.m_PixelIdx; 
        
		// Seeding
        uint seed = CombineIntoSeed(pixelIdx, shadeSeedData.m_FrameIdx, shadeSeedData.m_WavefronLoopIdx);
        seed = GetWangHashSeed(seed);
		
		// We stored normal data in absorption
        float3 normal = ray.m_Absorption;
		
        // DIRECT LIGHTING
        // Output Data:
		LightDataRaw lightDataRaw;
		float3 lightContribution = float3(0.0, 0.0, 0.0);
		bool isLightContributing = false;

		float2 blueNoise = SampleBlueNoiseTexture(pixelIdx, shadeSeedData.m_FrameIdx).xy;
            
		if (restirSettings.m_UseReSTIR != 0)
        {
            Reservoir finalReservoir = previousReservoirs[pixelIdx];
			// Get relevant light data
            if (isReservoirValid(finalReservoir))
            {
				lightDataRaw = GetLightData(ray.m_Origin,
											finalReservoir.m_PickedLightIdx,
											lightData[finalReservoir.m_PickedLightIdx],
											blueNoise);

                isLightContributing = EvalLightContribution(materialHitData,
															 normal,
															 ray.m_Direction,
															 lightDataRaw,
															 lightContribution,
															 seed);

				// Add contributing weight
                lightContribution *= finalReservoir.m_Weight;
            }
            
		    // Shade Pixel
            if (isLightContributing)
            {
                lightContribution *= ray.m_Throughput;
			    // ToDo, reimplement this or add a flag for it
			    // Remove fireflies, using the threshold
                float colorVal = length(lightContribution); ///Todo optimize to squared length
                float3 clampedEnergy = lightContribution;
                if (colorVal > threshold.m_Value)
                {
				    // The max energy, we can add = threshold
                    clampedEnergy = normalize(clampedEnergy) * threshold.m_Value;
                }
            
			    // Add energy of a pixel if it is not shaded
                output[pixelIdx] += float4(clampedEnergy, 1.0);
        
            }
        }
    }
    
    // Reset all current reservoirs
    currentReservoirs[idx.x] = InitEmptyReservoir();
}