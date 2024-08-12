#include "NEE.hlsl"
#include "ReSTIR.hlsl"

ConstantBuffer<DISeedData> shadeSeedData : register(b0);
ConstantBuffer<ReStirSettings> restirSettings : register(b1);

StructuredBuffer<Ray> rayBatch : register(t0);
StructuredBuffer<LightPickData> lightData : register(t1);
StructuredBuffer<MaterialHitData> materialHits : register(t2);

RWStructuredBuffer<uint> atomicShadowRays : register(u0);
RWStructuredBuffer<ShadowRay> shadowRayBatch : register(u1);
RWStructuredBuffer<Reservoir> currentReservoirs : register(u2);

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

		// We only do ReSTIR on the FIRST iteration as after that we don't have valid spatio-temporal data
		if (restirSettings.m_UseReSTIR != 0 && shadeSeedData.m_WavefronLoopIdx == 0)
		{
			// Input Data:
			LightDataRaw pickedLightData;
			float3 pickedLightContribution = float3(0.0, 0.0, 0.0);
			bool pickedisLightContributing = false;
			float pdf = 1.0 / shadeSeedData.m_NumLights; // Uniform Distribution at first
			Reservoir risReservoir = InitEmptyReservoir();

			// Create the Candidate Lights and add to the Reservoir
			for (int i = 0; i < restirSettings.m_RISRandomLights; i++)
			{
				float3 resLightContribution = float3(0.0, 0.0, 0.0);

				// ToDo, optimize this so all threads use the same lights.
				// https://www.youtube.com/watch?v=kI5uEMXvreY&t=10217s&ab_channel=High-PerformanceGraphics
				uint lightID = rand(seed) * shadeSeedData.m_NumLights - 1;

                LightDataRaw resLightData = GetLightData(ray.m_Origin, lightID, lightData[lightID], blueNoise);
				bool resIsLightContributing = EvalLightContribution(materialHitData,
																	normal,
																	ray.m_Direction, 
																	resLightData,
																	resLightContribution,
																	seed);

				float rawWeight = length(resLightContribution);

                if (UpdateReservoir(risReservoir, lightID, rawWeight, pdf, 1.0, seed))
				{
					pickedLightData = resLightData;
                    pickedLightContribution = resLightContribution;
					pickedisLightContributing = resIsLightContributing;
				}
			}

			// Calculate the weight of current Reservoir
			CalculateReservoirWeight(risReservoir);
            pickedLightContribution *= risReservoir.m_Weight;
                
			lightDataRaw = pickedLightData;
            lightContribution = pickedLightContribution;
			isLightContributing = pickedisLightContributing;
            currentReservoirs[pixelIdx] = risReservoir;
        }
		else
		{
			float3 randomlightContribution = float3(0.0, 0.0, 0.0);

			// Sample a random light source
			uint randomLightID = rand(seed) * shadeSeedData.m_NumLights - 1;
            LightDataRaw randomLightData = GetLightData(ray.m_Origin, randomLightID, lightData[randomLightID], blueNoise);

            // I don't think this should get accounted in lightColor.
			// IMO, lightColor var should be constant and should represent
			// all the data embedded in the glTF / Model / Material - Angel [08.03.24]
			float lightPickChanceMultiplier = shadeSeedData.m_NumLights;
				
			// Output of Randomly Picking a light
			bool randIsLightContrib = EvalLightContribution(materialHitData,
																normal,
																ray.m_Direction, 
																randomLightData,
																randomlightContribution,
																seed);

			randomlightContribution *= lightPickChanceMultiplier;
            // Out Random:
			lightDataRaw = randomLightData;
			isLightContributing = randIsLightContrib;
			lightContribution = randomlightContribution;
		}

		// We generate shadow ray only if the irradiace coming to a point is positive
		// And the light triangle is facing the intersection
		if (isLightContributing)
		{
			// Generated shadow ray
			ShadowRay sRay;
            sRay.m_Origin = ray.m_Origin; // Starts from intersection point
			sRay.m_Direction = lightDataRaw.lightDir; // Is directed towards random point on the light source
			sRay.m_DistanceT = lightDataRaw.distToLight; // Can't extend further than the point on the light

            sRay.m_Energy = ray.m_Throughput * lightContribution;
			sRay.m_PixelIdx = pixelIdx; // We will need to write to the corresponding pixel
			sRay.m_Padding = 0.f;

			// Increment the atomic and "push" to the shadow rays array
			uint prevAtom = 0;
			InterlockedAdd(atomicShadowRays[1], 1, prevAtom);
			shadowRayBatch[prevAtom] = sRay;
		}
    }
}