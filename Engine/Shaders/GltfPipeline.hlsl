

#include "Common.hlsl"
#include "ShaderHeaders/WavefrontStructsGPU.h"

// Inspired by https://github.com/nvpro-samples/vk_raytrace/tree/master
// Compact data, from a primitive reused a lot of times
// All the data is in world space

struct GeomIntersectData
{
    float3 m_Normal; // Hit normal
    float3 m_GeomNormal; // Outwards facing normal
    float3 m_ModelSpaceNormal;
    float3 m_Position;
    float2 m_UV;
    float3 m_TangentU;
    float3 m_TangentV;
    float3 m_Color;
    int m_TextureStart;
    int m_ModelStart;
    int m_MaterialIndex;
    float m_TexelAreaDouble;
    float m_TriangleAreaDouble;
};

float GetDoubleTriangleArea(float3 v0, float3 v1, float3 v2)
{
    float3 v01 = v1 - v0;
    float3 v02 = v2 - v0;
    float3 crossProduct = cross(v01, v02);
    
    float area = length(crossProduct); // no * 0.5
    return area;
}

float GetDoubleTriangleArea(float2 v0, float2 v1, float2 v2)
{
    float2 v01x = v1.x - v0.x;
    float2 v02x = v2.x - v0.x;
    float2 v01y = v1.y - v0.y;
    float2 v02y = v2.y - v0.y;
    
    float area = abs(v01x * v02y - v02x * v01y);
    return area;
}

// Referenced from RT Gems Chapter 20.6
void PropagateRayCone(inout Ray ray, float hitT, float coneSpreadAngle)
{
    ray.m_ConeWidth = coneSpreadAngle * hitT + ray.m_ConeWidth;
}

// Referenced from RT Gems Chapter 20.6
float GetTriangleLODConstant(float texelArea, float triangleArea, float texDim)
{
    float tA = texDim * texDim * texelArea;
    float pA = triangleArea;
    return 0.5 * log2(tA / pA);
}

// Referenced from RT Gems Chapter 20.6
float GetMaterialLOD(Ray ray, MaterialGPU materialInfo, GeomIntersectData geoData)
{
    float lambda = GetTriangleLODConstant(geoData.m_TexelAreaDouble, geoData.m_TriangleAreaDouble, materialInfo.m_TextureDim);
    lambda += log2(abs(ray.m_ConeWidth) + 0.00001);
    //lambda += 0.5 * log2(materialInfo.m_TextureDim * materialInfo.m_TextureDim); This is in boox example code, but not in formula???
    lambda -= log2(abs(dot(ray.m_Direction.xyz, geoData.m_Normal)));
    return lambda;
    
}

GeomIntersectData GetIntersectionData(ExtendResult hitResult)
{
    GeomIntersectData intersection;
    intersection.m_Normal = float3(1.f, 0.f, 0.f);
    intersection.m_GeomNormal = float3(1.f, 0.f, 0.f);
    intersection.m_Position = float3(0.f, 0.f, 0.f);
    intersection.m_UV = float2(0.f, 0.f);
    intersection.m_TangentU = float3(1.f, 0.f, 0.f);
    intersection.m_TangentV = float3(1.f, 0.f, 0.f);
    intersection.m_Color = float3(1.f, 1.f, 1.f);
    intersection.m_TextureStart = 0;
    intersection.m_ModelStart = 0;
    intersection.m_MaterialIndex = 0;
    // --------------- Get Ids -------------------------------------
    uint modelID = (hitResult.m_ModelAndInstanceID >> 16) & 0xFFFF;
    uint instanceID = hitResult.m_ModelAndInstanceID & 0xFFFF;
    uint primitiveID = hitResult.m_PrimitiveId;
    uint triangleID = hitResult.m_TriangleId;

    // Get reqired buffers from bindless heap
	StructuredBuffer<ModelHeapLocation> ModelBuffer = ResourceDescriptorHeap[RDH_MODEL_DATA];
    ModelHeapLocation modelInfo = ModelBuffer[modelID];
    PrimitiveGPU primitiveInfo = 
    StructuredBuffer<PrimitiveGPU>( ResourceDescriptorHeap[modelInfo.m_ModelStart])[primitiveID];
 
    
     // Get Indices of Triangle
    uint indicesIndex = primitiveInfo.m_IndexBufferId;
    StructuredBuffer<uint> indexBuffer =
        ResourceDescriptorHeap[modelInfo.m_ModelStart + BUFFER_OFFSET + indicesIndex];
    
    uint3 vertIdx = uint3(indexBuffer[(triangleID * 3) + 0], indexBuffer[(triangleID * 3) + 1], indexBuffer[(triangleID * 3) + 2]);
    // -------------------------------------------------------------
    
    // Get Barycentrics to find normals and sample textures
    float3 barycentrics = float3(1.f - hitResult.m_BarycentricUV.x - hitResult.m_BarycentricUV.y,
			hitResult.m_BarycentricUV.x, hitResult.m_BarycentricUV.y);
    
    // Get model to world transform
	StructuredBuffer<float4x4> transforms = ResourceDescriptorHeap[RDH_TRANSFORMS];
    float4x4 transformToWorld = transforms[instanceID];
    transformToWorld = mul(transformToWorld, primitiveInfo.m_Model);
    
    // Vertex positions
    float3 pos0;
    float3 pos1;
    float3 pos2;
    if (primitiveInfo.m_PositionIndex != -1)
    {
        StructuredBuffer<float3> posBuffer = ResourceDescriptorHeap[modelInfo.m_ModelStart + BUFFER_OFFSET + primitiveInfo.m_PositionIndex];
        // primitive space pos
        pos0 = mul(transformToWorld, float4(posBuffer[vertIdx.x], 1.f)).xyz;
        pos1 = mul(transformToWorld, float4(posBuffer[vertIdx.y], 1.f)).xyz;
        pos2 = mul(transformToWorld, float4(posBuffer[vertIdx.z], 1.f)).xyz;
        float3 pos = pos0 * barycentrics.x + pos1 * barycentrics.y + pos2 * barycentrics.z;
        // World position of the intersection
        intersection.m_Position = pos;
        intersection.m_TriangleAreaDouble = GetDoubleTriangleArea(pos0, pos1, pos2);
    }
    
    // Get texture coordinates
    float2 uv0;
    float2 uv1;
    float2 uv2;
    
    if (primitiveInfo.m_TexCoordIndex != -1)
    {
        StructuredBuffer<float2> UvBuffer =
				ResourceDescriptorHeap[modelInfo.m_ModelStart + BUFFER_OFFSET + primitiveInfo.m_TexCoordIndex];
        uv0 = UvBuffer[vertIdx.x];
        uv1 = UvBuffer[vertIdx.y];
        uv2 = UvBuffer[vertIdx.z];
        float2 textureUVs = uv0 * barycentrics.x + uv1 * barycentrics.y +
				uv2 * barycentrics.z;
        intersection.m_UV = textureUVs;
        intersection.m_TexelAreaDouble = GetDoubleTriangleArea(uv0, uv1, uv2);
    }
    // Intersection normal
    if (primitiveInfo.m_NormalIndex != -1)
    {
        StructuredBuffer<float3> normalBuffer =
				ResourceDescriptorHeap[modelInfo.m_ModelStart + BUFFER_OFFSET + primitiveInfo.m_NormalIndex];
        // Primitive space normal
        float3 normal = normalBuffer[vertIdx.x] * barycentrics.x + normalBuffer[vertIdx.y] * barycentrics.y +
				normalBuffer[vertIdx.z] * barycentrics.z;
		// From primitive to world space
        intersection.m_Normal = normalize(mul((float3x3) transformToWorld, normal));
        intersection.m_GeomNormal = intersection.m_Normal;
        intersection.m_ModelSpaceNormal = normalize(mul((float3x3) primitiveInfo.m_Model, normal));

    }
    else
    {
        intersection.m_GeomNormal = cross(pos1 - pos0, pos2 - pos0);
        intersection.m_ModelSpaceNormal = normalize(mul((float3x3) primitiveInfo.m_Model, intersection.m_GeomNormal));
        intersection.m_GeomNormal = normalize(mul((float3x3) transformToWorld, intersection.m_GeomNormal));
        intersection.m_Normal = intersection.m_GeomNormal;
    }
    
    // Tangents
    if (primitiveInfo.m_TangentIndex != -1)
    {
        StructuredBuffer<float4> tangentBuffer =
				ResourceDescriptorHeap[modelInfo.m_ModelStart + BUFFER_OFFSET + primitiveInfo.m_TangentIndex];
        // Primitive space tangent
        float4 tangentV = tangentBuffer[vertIdx.x] * barycentrics.x + tangentBuffer[vertIdx.y] * barycentrics.y +
				tangentBuffer[vertIdx.z] * barycentrics.z;
        
        float tangentSpace = sign(tangentV.w);
        float3 tangent = tangentV.xyz;
		// World space tangent
        tangent = normalize(mul(transformToWorld, float4(tangent, 0.f)).xyz);
        tangent = normalize(tangent - dot(intersection.m_Normal, tangent) * intersection.m_Normal);
        float3 bitangent = normalize(cross(intersection.m_Normal, tangent) + 0.0001f) * tangentSpace; // do we need *tangentSpace?
        intersection.m_TangentU = tangent;
        intersection.m_TangentV = bitangent;
    }
    else
    {
        // Generate tangent and bitangent
        float3 N = intersection.m_Normal;
        if ((abs(N.z) > 0.99999f))
            intersection.m_TangentU = normalize(float3(-N.x * N.y, 1.0f - N.y * N.y, -N.y * N.z));
        else
            intersection.m_TangentU = normalize(float3(-N.x * N.z, -N.y * N.z, 1.0f - N.z * N.z));
        intersection.m_TangentV = cross(intersection.m_TangentU, N);
    }
    
    // Get vertex color
    if (primitiveInfo.m_ColorIndex != -1)
    {
        StructuredBuffer<float3> colorBuffer =
				ResourceDescriptorHeap[modelInfo.m_ModelStart + BUFFER_OFFSET + primitiveInfo.m_ColorIndex];

        float3 color = colorBuffer[vertIdx.x] * barycentrics.x + colorBuffer[vertIdx.y] * barycentrics.y +
				colorBuffer[vertIdx.z] * barycentrics.z;
        intersection.m_Color = color;
        
    }
    intersection.m_ModelStart = modelInfo.m_ModelStart;
    intersection.m_TextureStart = modelInfo.m_TextureStart;
    intersection.m_MaterialIndex = primitiveInfo.m_MaterialIndex;
    return intersection;

}

// Inspired by https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/6546208a521eebff504929ce5bfa43da7c43eaee/source/Renderer/shaders/material_info.glsl
// Reads Textures and compiles the material values for the current hit

// Modify normal based on the normal map
void ApplyNormalMap(MaterialGPU materialInfo, SamplerState texSampler, inout GeomIntersectData data)
{
    if (materialInfo.m_NormalTextureIndex != -1)
    {
        Texture2D<float3> normalTexture = ResourceDescriptorHeap[data.m_TextureStart + materialInfo.m_NormalTextureIndex];
        float3 normTex = normalTexture.SampleLevel(texSampler, data.m_UV, materialInfo.m_MaterialLOD).rgb;
        normTex = normalize(normTex * 2.f - 1.f);
        //normTex *= float3(materialInfo.m_NormalTextureScale, materialInfo.m_NormalTextureScale, 1.0);
        //normTex.g = -normTex.g;
        float3x3 TBN = float3x3(data.m_TangentU, data.m_TangentV, data.m_Normal);
        data.m_Normal = normalize(mul(normTex, TBN));
    }
}

float3 GetEmissiveColor(MaterialGPU materialInfo, SamplerState texSampler, GeomIntersectData data)
{
    float3 emissiveColor = materialInfo.m_EmissiveFactor;
    if (materialInfo.m_EmissiveTextureIndex != -1)
    {
        Texture2D<float4> emissiveTexture = ResourceDescriptorHeap[data.m_TextureStart + materialInfo.m_EmissiveTextureIndex];
        emissiveColor *= SRGBToLinear(emissiveTexture.SampleLevel(texSampler, data.m_UV, materialInfo.m_MaterialLOD)).xyz;
    }
    return emissiveColor;
}

void GetBaseColor(inout MaterialHitData matData, MaterialGPU materialInfo, SamplerState texSampler, GeomIntersectData data)
{
    matData.m_BaseColor = materialInfo.m_BaseColorFactor.rgb;
    if (materialInfo.m_BaseColorTextureIndex != -1)
    {
        Texture2D<float4> baseColorTexture =ResourceDescriptorHeap[data.m_TextureStart + materialInfo.m_BaseColorTextureIndex];
        matData.m_BaseColor *= SRGBToLinear(baseColorTexture.SampleLevel(texSampler, data.m_UV, materialInfo.m_MaterialLOD)).rgb;
    }

    matData.m_BaseColor *= data.m_Color;
}

void GetMetallicRoughnessInfo(inout MaterialHitData matData, MaterialGPU materialInfo, SamplerState texSampler, GeomIntersectData data)
{
    matData.m_Metallic = materialInfo.m_MetallicFactor;
    matData.m_AlphaRoughness = materialInfo.m_RoughnessFactor;

    if (materialInfo.m_MetallicRoughnessTextureIndex != -1)
    {
        // Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
        // This layout intentionally reserves the 'r' channel for (optional) occlusion map data
        Texture2D<float4> metallicRoughnessTexture = ResourceDescriptorHeap[data.m_TextureStart + materialInfo.m_MetallicRoughnessTextureIndex];
        
        float4 metRough = metallicRoughnessTexture.SampleLevel(texSampler, data.m_UV, materialInfo.m_MaterialLOD);
        matData.m_AlphaRoughness *= metRough.g;
        matData.m_Metallic *= metRough.b;
    }
    
    matData.m_F0 = lerp(matData.m_F0, matData.m_BaseColor, matData.m_Metallic);
}

// SPECULAR KHR
void GetSpecularInfo(inout MaterialHitData matData, MaterialGPU materialInfo, SamplerState texSampler, SamplerState texColSampler, GeomIntersectData data)
{   
    float3 specularTextureSample = float3(1.f, 1.f, 1.f);
    matData.m_SpecularWeight = materialInfo.m_SpecularFactor;
    
    if (materialInfo.m_SpecularTextureIndex != -1)
    {
        Texture2D<float4> specularTexture = ResourceDescriptorHeap[data.m_TextureStart + materialInfo.m_SpecularTextureIndex];
        
        matData.m_SpecularWeight *= specularTexture.SampleLevel(texSampler, data.m_UV, materialInfo.m_MaterialLOD).a;
    }

    if (materialInfo.m_SpecularColorTextureIndex != -1)
    {
        Texture2D<float4> specularColorTexture = ResourceDescriptorHeap[data.m_TextureStart + materialInfo.m_SpecularColorTextureIndex];

        specularTextureSample = SRGBToLinear(specularColorTexture.SampleLevel(texColSampler, data.m_UV, materialInfo.m_MaterialLOD).rgb);
      
    }

    float3 dielectricSpecularF0 = min(matData.m_F0 * materialInfo.m_SpecularColorFactor * specularTextureSample.rgb, float3(1.f, 1.f, 1.f));
    matData.m_F0 = lerp(dielectricSpecularF0, matData.m_BaseColor, matData.m_Metallic);
}

// TRANSMISSSION KHR
void GetTransmissionInfo(inout MaterialHitData matData, MaterialGPU materialInfo, SamplerState texSampler, GeomIntersectData data)
{
    matData.m_TransmissionFactor = materialInfo.m_TransmissionFactor;

    if (materialInfo.m_TransmissionTextureIndex != -1)
    {
        Texture2D<float4> transmissionTexture = ResourceDescriptorHeap[data.m_TextureStart + materialInfo.m_TransmissionTextureIndex];
        
        float4 transmissionSample = transmissionTexture.SampleLevel(texSampler, data.m_UV, materialInfo.m_MaterialLOD);
        matData.m_TransmissionFactor *= transmissionSample.r;
    }
}

// IOR KHR
void GetIorInfo(inout MaterialHitData matData, MaterialGPU materialInfo)
{
    matData.m_F0 = pow((materialInfo.m_Ior - 1.0) / (materialInfo.m_Ior + 1.0), 2.0);
}

MaterialHitData FillInMaterialData(MaterialGPU materialInfo, GeomIntersectData geomIntersect)
{
    MaterialHitData matData;
    matData.m_F0 = 0.04f;
    matData.m_SpecularWeight = 0.f;
    matData.m_DiffuseRayID = 0;
    // Read data
    GetBaseColor(matData, materialInfo, SamplerDescriptorHeap[materialInfo.m_BaseColorSamplerIndex], geomIntersect);
    GetIorInfo(matData, materialInfo);
    GetMetallicRoughnessInfo(matData, materialInfo, SamplerDescriptorHeap[materialInfo.m_MetallicRoughnessSamplerIndex], geomIntersect);
    GetSpecularInfo(matData, materialInfo, SamplerDescriptorHeap[materialInfo.m_SpecularSamplerIndex], SamplerDescriptorHeap[materialInfo.m_SpecularColorSamplerIndex], geomIntersect);
    GetTransmissionInfo(matData, materialInfo, SamplerDescriptorHeap[materialInfo.m_TransmissionSamplerIndex], geomIntersect);
   
    // Pre-calculate values for PBR
    matData.m_Metallic = clamp(matData.m_Metallic, 0.0, 1.0);
    
    // Perceptual to alpha roughness
    matData.m_AlphaRoughness = clamp(matData.m_AlphaRoughness * matData.m_AlphaRoughness, 0.001, 1.0);
    
    return matData;
}