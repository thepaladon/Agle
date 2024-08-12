#include "PBR.hlsl"
#include "ReSTIR.hlsl"
#include "ShaderHeaders/CameraGPU.h"

ConstantBuffer<ShadeSeedData> shadeSeedData : register(b0);
ConstantBuffer<GameplaySkyMat> skyMat : register(b1);
ConstantBuffer<ShadeSettings> shadeSettings : register(b2);

StructuredBuffer<uint> rayCount : register(t0);
StructuredBuffer<ExtendResult> extendedBatch : register(t1);

RWStructuredBuffer<Ray> rayBatch : register(u0);
RWStructuredBuffer<uint> atomicNewRays : register(u1);
RWStructuredBuffer<uint> atomicShadowRays : register(u2);
RWStructuredBuffer<Ray> newRaysBatch : register(u3);
RWStructuredBuffer<float4> output : register(u4);
RWStructuredBuffer<float4> worldSpaceIntersections : register(u5);
RWStructuredBuffer<float> currentDepthBuffer : register(u6);
RWStructuredBuffer<float> prevDepthBuffer : register(u7);
RWStructuredBuffer<float3> currentModelNormal : register(u8);
RWStructuredBuffer<uint> currentModelPrimID : register(u9);
RWStructuredBuffer<float4> primaryAlbedo : register(u10);
RWStructuredBuffer<float4> emission : register(u11);
RWStructuredBuffer<uint> instanceIDs : register(u12);
RWStructuredBuffer<MaterialHitData> materialHits : register(u13);

float3 ApplyThreshold(float3 color)
{
    float colorVal = length(color); ///Todo optimize to squared length
    float3 clampedEnergy = color;
    if (colorVal > shadeSettings.m_Threshold)
    {
				// The max energy, we can add = threshold
        clampedEnergy = normalize(clampedEnergy) * shadeSettings.m_Threshold;
    }
    return clampedEnergy;
}

float4 SampleSky(float3 rayDir, SamplerState samp)
{
    if (skyMat.m_UseSkyMat == 1)
    {
        rayDir = normalize(mul(skyMat.m_RotMat, float4(rayDir, 0.0f)).xyz);
    }
    float u = 0.5 + atan2(rayDir.z, rayDir.x) / (2 * PI);
    float v = 0.5 - asin(rayDir.y) / PI;

    Texture2D<float4> skyboxTex = ResourceDescriptorHeap[RDH_SKYBOX];
	return float4(skyboxTex.SampleLevel(samp, float2(u, v), 0).rgb, 1.0);
}

// 1 - survival chance in Russian Rullette after hitting a surface
float SurviveProbRR(float3 albedo)
{
    return clamp(max(max(albedo.r, albedo.g), albedo.b), 0.f, 1.f);
}

[numthreads(256, 1, 1)]
void main(uint3 idx : SV_DispatchThreadID)
{
	// Work only with active rays
    if (idx.x < rayCount[0])
    {
        ExtendResult hitResult = extendedBatch[idx.x];
        Ray ray = rayBatch[idx.x];
		uint pixelIdx = ray.m_PixelIdx; 
        
        // Find the world space intersection point
        float3 intersection = ray.m_Origin + ray.m_Direction * hitResult.m_DistanceT;
        if (shadeSeedData.m_WavefronLoopIdx == 0)
        {
            prevDepthBuffer[pixelIdx] = currentDepthBuffer[pixelIdx];
            currentDepthBuffer[pixelIdx] = hitResult.m_DistanceT;
            worldSpaceIntersections[pixelIdx] = float4(intersection, 1.f);
        }
        
        float3 rayThroughput = ray.m_Throughput;
        float3 absorption = ray.m_Absorption;
		// If ray didn't hit anything, it samples sky and dies
        if (hitResult.m_DistanceT < 0.f)
        {
            SamplerState mirror = SamplerDescriptorHeap[SDH_SKYBOX]; 
            float3 skyColor = rayThroughput * SampleSky(normalize(ray.m_Direction).xyz, mirror).rgb ;
			output[pixelIdx] += float4(ApplyThreshold(skyColor * skyMat.m_LightingStrength), 0.f);

            // No geometry intersected
            if (shadeSeedData.m_WavefronLoopIdx == 0)
            {
                currentModelPrimID[pixelIdx] = PackUints(0, 0);
				emission[pixelIdx] += float4(skyColor * skyMat.m_BackgroundStrength, 0.f);
                instanceIDs[pixelIdx] = 0;
            }
            return;
        }
        else
        {
            if (shadeSeedData.m_WavefronLoopIdx == 0)
            {
            // Store intersected mesh
                uint modelID = (hitResult.m_ModelAndInstanceID >> 16) & 0xFFFF;
                uint instanceID = hitResult.m_ModelAndInstanceID & 0xFFFF;
                uint primitiveID = hitResult.m_PrimitiveId;
            // We add 1, as we use uints and 0 will be no model intersection
                currentModelPrimID[pixelIdx] = PackUints(modelID + 1, primitiveID + 1);
                instanceIDs[pixelIdx] = instanceID + 1;
            }
        }
        // Get vertex attributes
        GeomIntersectData intersectData = GetIntersectionData(hitResult);
        PropagateRayCone(ray, hitResult.m_DistanceT, shadeSettings.m_ConeSpreadAngle);
        
        if (shadeSeedData.m_WavefronLoopIdx == 0)
        {
            currentModelNormal[pixelIdx] = intersectData.m_ModelSpaceNormal;
        }
        
        MaterialGPU materialInfo =
        StructuredBuffer<MaterialGPU>( ResourceDescriptorHeap[intersectData.m_ModelStart + MATERIAL_OFFSET])[intersectData.m_MaterialIndex];

        materialInfo.m_MaterialLOD = GetMaterialLOD(ray, materialInfo, intersectData);
        // EMISSION
        {
            float3 lightColor = GetEmissiveColor(materialInfo, SamplerDescriptorHeap[materialInfo.m_EmissiveSamplerIndex], intersectData);

            if (materialInfo.m_EmissiveStrength > 1.f)
            {
                // Drawing lights from a direct sample (from camera or from mirror)
                if (ray.m_LastSpecular == 1)
                {
					// Only Shade the surface of the emissive triangle as only that emits light
                    float4 lightEnergy = 0.f;
                    if (shadeSeedData.m_WavefronLoopIdx == 0)
                    {
                         // Draw lights directly from the camera
                        lightEnergy = float4(lightColor * materialInfo.m_EmissiveStrength, 0.0);
                        output[pixelIdx] += lightEnergy;
                        emission[pixelIdx] += lightEnergy;
                    }
                    else
                    {
                        // Draw lights from specular bounces
                        lightEnergy = float4(ApplyThreshold(rayThroughput * lightColor * materialInfo.m_EmissiveStrength), 0.0);
                        output[pixelIdx] += lightEnergy;
                    }
                }
                // We stop upon hitting a light
                return;
            }

            // Else We don't consider the triangle a light and we shade as unlit emissive
			if (length(lightColor) > 0.f)
			{
				output[pixelIdx] += float4(rayThroughput * lightColor * materialInfo.m_EmissiveStrength, 0.f);
                if (shadeSeedData.m_WavefronLoopIdx == 0)
                {
                    emission[pixelIdx] += float4(rayThroughput * lightColor, 0.f);
                }
				return;
			}
		}
        
		// Seeding
        uint seed = CombineIntoSeed(pixelIdx, shadeSeedData.m_FrameIdx, shadeSeedData.m_WavefronLoopIdx);
        seed = GetWangHashSeed(seed);

        // Read all textures and factors
        MaterialHitData materialHitData = FillInMaterialData(materialInfo, intersectData);
        
		// Normal Map
        ApplyNormalMap(materialInfo, SamplerDescriptorHeap[materialInfo.m_NormalSamplerIndex], intersectData);

        bool inside = false;
        if (dot(intersectData.m_Normal, ray.m_Direction.xyz) > 0.f)
        {
            inside = true;
            intersectData.m_Normal = -intersectData.m_Normal;
            materialHitData.m_Eta = materialInfo.m_Ior;
            materialHitData.m_F0 = pow((1.0 - materialInfo.m_Ior) / (materialInfo.m_Ior + 1.0), 2.0);
            
        }
        else
        {
            materialHitData.m_Eta = 1.f / materialInfo.m_Ior;
        }
      
        // INDIRECT LIGHTING
       
        // Get albedo from the base color
        float4 albedo = float4(materialHitData.m_BaseColor, 0.f);
        if (shadeSeedData.m_WavefronLoopIdx == 0)
        {
            primaryAlbedo[pixelIdx] = albedo;
        }
       
        // Draw UNLIT objects
        if (materialInfo.m_Unlit == 1)
        {
            if (ray.m_LastSpecular == 1)
            {
                output[pixelIdx] += float4(rayThroughput * albedo.rgb, 0.f);
            }
            // No point of further shading
            return;
        }

        // Imposrtance sampling hemisphere with cosine
        float3 newRayDir = float3(0.f, 0.f, 0.f);
         
        // Probability density function based on irradiance
        float irradPdf = 0.f;
        uint isSpecular = 1;
        bool refracted = false;

    	float3 BRDF = PbrSample(materialHitData,
								intersectData,
								inside,
								-ray.m_Direction,
								isSpecular,
								refracted,
								newRayDir,
								irradPdf,
								seed);
      
        if (irradPdf <= 0.f || BRDF.x < 0.f || BRDF.y < 0.f || BRDF.z < 0.f)
        {
            return;
        }
        
        //ABSORPTION
        rayThroughput *= exp(-absorption * hitResult.m_DistanceT);
        if (refracted == true)
        {   
            if (inside == true)
                absorption = float3(0.f, 0.f, 0.f);
            else
                absorption = -log(materialInfo.m_AttenuationColor) / (materialInfo.m_AttenuationDistance);
        }

        if (isSpecular == 0)
        {
            // Fill in data to pass to the DI shader
            // These aren't actual rays - the real bounce rays are generated after
            ray.m_Origin = intersection; // Starts from intersection point
            ray.m_Absorption = intersectData.m_Normal; // We write normal here to save space in material hit struct
            ray.m_Throughput = rayThroughput;
            rayBatch[idx.x] = ray;
            
            materialHitData.m_DiffuseRayID = idx.x;
            // Increment the atomic for direct hits and "push" to the rays array
            uint prevAtom = 0;
            InterlockedAdd(atomicShadowRays[0], 1, prevAtom);
            materialHits[prevAtom] = materialHitData;
        }
        
		// Russian Roulette
        if (isSpecular == 0)
        {
            
            float probability = SurviveProbRR(albedo.rgb);
            float randVal = rand(seed);
			// Kill random rays based on the max albedo color
            if (probability < randVal)
                return;
			// If the ray survives, it contributes more to the final image,
			// as it would, on average, survive less often
            else
                rayThroughput *= (1.f / probability);
        }
        
        // Tracing Distance Selection - https://www.gdcvault.com/play/1026723/Ray-Traced-Reflections-in-Wolfenstein
        // The values are from the Wolfenstein presentation
        float smoothness = clamp(1.f - materialHitData.m_AlphaRoughness, 0.001, 1.0);
        float range = shadeSettings.m_TracingDistanceMultiplier * pow(smoothness * 0.9 + 0.1, 4.0);
        
		// Generate a new ray
		{
			Ray newRay;
			newRay.m_Origin = intersection;
			newRay.m_Direction = newRayDir;
			// Clamp is a temporal fix of black pixels
			rayThroughput *= max(abs(dot(intersectData.m_Normal, newRayDir) / irradPdf * BRDF), 0.001f);
			newRay.m_Throughput = rayThroughput;
			newRay.m_PixelIdx = pixelIdx;
			newRay.m_LastSpecular = isSpecular * ray.m_LastSpecular;
            newRay.m_Absorption = absorption;
            newRay.m_ConeWidth = ray.m_ConeWidth;
            newRay.m_MaxT = range;
			// "Push" a new ray to the batch
			uint prevAtom = 0;
			InterlockedAdd(atomicNewRays[0], 1, prevAtom);
			newRaysBatch[prevAtom] = newRay;
		}
    }
}