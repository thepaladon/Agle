#include "PBR.hlsl"
#include "ShaderHeaders/WavefrontStructsGPU.h" 

float GetTriangleSurface(float3 v0, float3 v1, float3 v2)
{
    float3 v01 = v1 - v0;
    float3 v02 = v2 - v0;
    float3 crossProduct = cross(v01, v02);
    // Calculate the magnitude of the cross product
    float area = 0.5 * length(crossProduct);
    return area;
}

bool EvalLightContribution(
    MaterialHitData materialHitData,
    float3 intersectionNormal,
    float3 rayDir,
	LightDataRaw lightIn,
    out float3 lightContribution,
    inout uint seed
)
{
    if (dot(intersectionNormal, lightIn.lightDir) > 0.f)
    {
        float lightPDF = 1.f;
        float3 lightBRDF = PbrDirectSample(materialHitData, -rayDir, intersectionNormal, lightIn.lightDir, lightPDF, seed);
       
        if (lightPDF > 0.f && lightBRDF.x >= 0.f && lightBRDF.y >= 0.f && lightBRDF.z >= 0.f)
        {
            // We take abs of dot(lightIn.lightNormal, -lightIn.lightDir), so every light is considered double sided
            const float solidAngle = (abs(dot(lightIn.lightNormal, -lightIn.lightDir)) * lightIn.lightArea) / (lightIn.distToLight * lightIn.distToLight);
            lightPDF *= 1.f / solidAngle;
            lightContribution = (dot(intersectionNormal, lightIn.lightDir) / lightPDF) * lightBRDF * lightIn.lightColor;
            return true;
        }
    }
    lightContribution = float3(0.0, 0.0, 0.0);
    return false;
}
 
LightDataRaw GetLightData(
    float3 origin,
	uint lightID,
    LightPickData lightPick,
    float2 blueNoise)
{
    LightDataRaw rawLightData;

    uint modelID = lightPick.m_ModelId;
    uint instanceID = lightPick.m_InstanceId;

    // From all light triangles get specific triangle in specific model in specific primitive
    uint primitiveID = lightPick.m_PrimitiveId;
    uint triangleID = lightID - lightPick.m_LightsInPrim;

    float2 uv = float2(blueNoise.x, blueNoise.y);
    
    // Get Barycentrics to find normals and sample textures
    float3 barycentrics = float3(1.f - uv.x - uv.y, uv.x, uv.y);
	
    ModelHeapLocation modelInfo = 
    StructuredBuffer<ModelHeapLocation>( ResourceDescriptorHeap[RDH_MODEL_DATA])[modelID];
    
    PrimitiveGPU primitiveInfo =

    StructuredBuffer<PrimitiveGPU>( ResourceDescriptorHeap[modelInfo.m_ModelStart])[primitiveID];
    
    MaterialGPU materialInfo =

    StructuredBuffer<MaterialGPU>( ResourceDescriptorHeap[modelInfo.m_ModelStart + MATERIAL_OFFSET])[primitiveInfo.m_MaterialIndex]; // Can  we even have something without material?

	// Get Indices of Triangle
    uint indicesIndex = primitiveInfo.m_IndexBufferId;
    StructuredBuffer<uint> indexBuffer =
        ResourceDescriptorHeap[modelInfo.m_ModelStart + BUFFER_OFFSET + indicesIndex];
    uint3 vertIdx = uint3(indexBuffer[(triangleID * 3) + 0], indexBuffer[(triangleID * 3) + 1], indexBuffer[(triangleID * 3) + 2]);
    
	// Get model to world transform
    StructuredBuffer<float4x4> transforms = ResourceDescriptorHeap[RDH_TRANSFORMS];
    float4x4 transformToWorld = transforms[instanceID];
    transformToWorld = mul(transformToWorld, primitiveInfo.m_Model);
    
    // Get light normal
    if (primitiveInfo.m_NormalIndex != -1)
    {
        StructuredBuffer<float3> normalBuffer =
				ResourceDescriptorHeap[modelInfo.m_ModelStart + BUFFER_OFFSET + primitiveInfo.m_NormalIndex];
        // Primitive space normal
        rawLightData.lightNormal = normalBuffer[vertIdx.x] * barycentrics.x + normalBuffer[vertIdx.y] * barycentrics.y +
				normalBuffer[vertIdx.z] * barycentrics.z;
		// From primitive to world space
        rawLightData.lightNormal = normalize(mul((float3x3) transformToWorld, rawLightData.lightNormal));
    }
  
	// Get Barycentrics to find normals and sample textures
    float2 textureUVs = float2(0.f, 0.f);
    if (primitiveInfo.m_TexCoordIndex != -1)
    {
        StructuredBuffer<float2> UvBuffer =
				    ResourceDescriptorHeap[modelInfo.m_ModelStart + BUFFER_OFFSET + primitiveInfo.m_TexCoordIndex];
        textureUVs = UvBuffer[vertIdx.x] * barycentrics.x + UvBuffer[vertIdx.y] * barycentrics.y +
				    UvBuffer[vertIdx.z] * barycentrics.z;
    }

    // Get Light Color
    {
        rawLightData.lightColor = materialInfo.m_EmissiveFactor;
        if (materialInfo.m_EmissiveTextureIndex != -1)
        {
            SamplerState samplerE = SamplerDescriptorHeap[materialInfo.m_EmissiveSamplerIndex];
            Texture2D<float4> emissiveTexture = ResourceDescriptorHeap[modelInfo.m_TextureStart + materialInfo.m_EmissiveTextureIndex];
            rawLightData.lightColor *= SRGBToLinear(emissiveTexture.SampleLevel(samplerE, textureUVs, 0.0)).xyz;
        }
        rawLightData.lightColor *= materialInfo.m_EmissiveStrength;
    }

    // Get intersection pos
    float3 intersection = float3(0.f, 0.f, 0.f);
    if (primitiveInfo.m_PositionIndex != -1)
    {
        StructuredBuffer<float3> posBuffer = ResourceDescriptorHeap[modelInfo.m_ModelStart + BUFFER_OFFSET + primitiveInfo.m_PositionIndex];
        // World space positions of the triangle verts
        float3 vPosWorld0 = mul(transformToWorld, float4(posBuffer[vertIdx.x], 1.f)).xyz;
        float3 vPosWorld1 = mul(transformToWorld, float4(posBuffer[vertIdx.y], 1.f)).xyz;
        float3 vPosWorld2 = mul(transformToWorld, float4(posBuffer[vertIdx.z], 1.f)).xyz;
        // World space intersection
        intersection = vPosWorld0 * barycentrics.x + vPosWorld1 * barycentrics.y + vPosWorld2 * barycentrics.z;
        // Calculate triangle area
        rawLightData.lightArea = GetTriangleSurface(vPosWorld0, vPosWorld1, vPosWorld2);
    }

    float epsilon = 0.001;
    float3 lightVec = intersection - origin;
    rawLightData.lightDir = normalize(lightVec);
    rawLightData.distToLight = length(lightVec) - epsilon;

    return rawLightData;
}
