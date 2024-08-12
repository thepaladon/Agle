#include "Random.hlsl"
#include "BrdfFuncs.hlsl"
#include "GltfPipeline.hlsl"

// HEMISPHERE -----------------------------------
// Hemisphere importance sampling (Diffuse)
float3 CosineWeightedDiffuseReflection(inout uint seed)
{
	float r0 = rand(seed);
	float r1 = rand(seed);
    float r = sqrt(r0);
    float theta = 2 * PI * r1;
    float x = r * cos(theta);
    float y = r * sin(theta);
    float3 ray = float3(x, y, sqrt(1.f - r0));
    
    return ray;
}

// Hemisphere GGX sampling (Specular)
float3 GGXSampling(float specularAlpha, inout uint seed)
{
	float r0 = rand(seed);
	float r1 = rand(seed);
    float phi = r0 * 2.f * PI;
 
    float cosTheta = sqrt((1.f - r1) / (1.f + (specularAlpha * specularAlpha - 1.0) * r1));
    float sinTheta = clamp(sqrt(1.0 - (cosTheta * cosTheta)), 0.0, 1.0);
    float sinPhi = sin(phi);
    float cosPhi = cos(phi);
    float3 ray =  float3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
    
    return ray;
}

float3 EvalDiffuseGltf(MaterialHitData mat, float3 V, float3 N, float3 L, inout float pdf)
{
    pdf = 0;
    float NdotV = dot(N, V);
    float NdotL = dot(N, L);

    if (NdotL < 0.0 || NdotV < 0.0)
        return float3(0.f, 0.f, 0.f);

    NdotL = clamp(NdotL, 0.001, 1.0);

    pdf = NdotL * invPI;
    
    return BRDF_LambertianSimple(lerp(mat.m_BaseColor, float3(0.f, 0.f, 0.f), mat.m_Metallic));
}

float3 BSDF_GGX(MaterialHitData mat, float3 V, float3 N, float3 L, float3 H, float NdotL, float3 F, inout float pdf)
{
    pdf = 0;
    
    if (NdotL < 0.0)
        return 0.f;

    float NdotV = dot(N, V);
    float NdotH = clamp(dot(N, H), 0.f, 1.f);
    float LdotH = clamp(dot(L, H), 0.f, 1.f);
    float VdotH = clamp(dot(V, H), 0.f, 1.f);

    NdotL = clamp(NdotL, 0.001, 1.0);
    NdotV = clamp(abs(NdotV), 0.001, 1.0);
    
    float G = V_GGX(NdotL, NdotV, mat.m_AlphaRoughness);
    float D = D_GGX(NdotH, mat.m_AlphaRoughness);
    
    pdf = D * NdotH / (4.0 * VdotH);
    
    return F * D * G;
}

float3 EvalBSDF(MaterialHitData mat, float3 V, float3 N, float3 L, float3 H, bool specBounce, float specChance, bool transmissBounce, float transmissChance, inout float pdf)
{
    pdf = 0.f;
    float3 brdf = float3(0.f, 0.f, 0.f);
    if (transmissBounce == true)
    {
        if (specBounce == true)
        {
            // Reflection
            float NdotL = dot(N, L);
            brdf = BSDF_GGX(mat, V, N, L, H, NdotL, specChance, pdf) * mat.m_BaseColor.rgb;
            pdf *= specChance * transmissChance;
        }
        else
        {
            // Transmission
            float NdotL = abs(dot(N, L));
            brdf = BSDF_GGX(mat, V, N, L, H, NdotL, (1.0 - specChance), pdf) * mat.m_BaseColor.rgb;
            pdf *= (1.0 - specChance) * transmissChance;
        }
    }
    else
    {
    
        if (specBounce == true)
        {
            float NdotL = dot(N, L);
            float VdotH = clamp(dot(V, H), 0.f, 1.f);
            float3 f90 = 1.0;
            float3 F = F_Schlick(mat.m_F0, f90, VdotH);
            brdf = BSDF_GGX(mat, V, N, L, H, NdotL, F, pdf);
            pdf *= specChance * (1.f - transmissChance);
        }
        else
        {
            brdf = EvalDiffuseGltf(mat, V, N, L, pdf);
            pdf *= (1.f - specChance) * (1.f - transmissChance);
        }
    }
    
    return brdf;
}

// Inspired by https://github.com/nvpro-samples/vk_raytrace/tree/master
float3 PbrSample(MaterialHitData mat, GeomIntersectData intersectData, bool inside, float3 V, inout uint isSpecular, out bool refracted, inout float3 newRayDir, inout float pdf, inout uint seed)
{
    pdf = 0.f;
    float3 N = intersectData.m_Normal;
    float3 T = intersectData.m_TangentU;
    float3 B = intersectData.m_TangentV;
    float3 H = 0.f;
    float specChance = mat.m_Metallic;
    bool specBounce = rand(seed) < specChance;
    refracted = false;
    // Transmission weight
    float transChance = (1.f - mat.m_Metallic) * mat.m_TransmissionFactor;
    bool transBounce = rand(seed) < transChance;
    // Transmission
    if (transBounce == true)
    {
        isSpecular = true;
        
        H = GGXSampling(mat.m_AlphaRoughness, seed);
        H = T * H.x + B * H.y + N * H.z;
        newRayDir = normalize(reflect(-V, H));
        
        float f0 = mat.m_F0.r;
        // Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to "Real-Time-Rendering" 4th editon on page 325.
        float f90 = clamp(f0 * 50.0, 0.0, 1.0);
        float VdotH = abs(dot(V, H));
        float F = F_Schlick(f0, f90, abs(dot(newRayDir, H)));
        specChance = F;
        float discriminat = mat.m_Eta * mat.m_Eta * (1.f - VdotH * VdotH); // (Total internal reflection)
        // Reflection/Total internal reflection
        if (discriminat > 1.f || rand(seed) < specChance)
        {
            specBounce = true;
        }
        else
        {
            refracted = true;
            specBounce = false;
            // Find the pure refractive ray
            newRayDir = normalize(refract(-V, H, mat.m_Eta));
            // Catch rays perpendicular to surface, and simply continue
            if (isnan(newRayDir.x) || isnan(newRayDir.y) || isnan(newRayDir.z))
            {
                newRayDir = -V;
            }
        }

    }
    else
    {
        // Calculate new direction
        if (specBounce == true)
        {
            H = GGXSampling(mat.m_AlphaRoughness, seed);
            H = T * H.x + B * H.y + N * H.z;
            newRayDir = reflect(-V, H);
            isSpecular = 1;
        }
        else
        {
            float newSpecChance = mat.m_SpecularWeight;
            
            if (rand(seed) < newSpecChance)
            {
                specBounce = true;
                specChance = (1.f - specChance) * newSpecChance;
                H = GGXSampling(mat.m_AlphaRoughness, seed);
                H = T * H.x + B * H.y + N * H.z;
                newRayDir = reflect(-V, H);
                isSpecular = 1;
            }
            else
            {
                // Diffuse
                specChance = specChance + newSpecChance - specChance * newSpecChance;
                float3 L = CosineWeightedDiffuseReflection(seed);
                L = T * L.x + B * L.y + N * L.z;
                newRayDir = L;
                isSpecular = 0;
            }
        }
    }
    
    // Evaluate a full BSDF
    float3 brdf = EvalBSDF(mat, V, N, newRayDir, H, specBounce, specChance, transBounce, transChance, pdf);
    return brdf;
}

float3 PbrDirectSample(MaterialHitData mat, float3 V, float3 N, float3 L, inout float pdf, inout uint seed)
{
    // Calculate the half vector
    float3 H;
    if (dot(N, L) < 0.0)
        H = normalize(L * (1.0 / mat.m_Eta) + V);
    else
        H = normalize(L + V);
    if (dot(N, H) < 0.0)
        H = -H;
    
    pdf = 0.f;
    float specChance = mat.m_Metallic;
    bool specBounce = rand(seed) < specChance;
    float transChance = (1.f - mat.m_Metallic) * mat.m_TransmissionFactor;
    bool transBounce = rand(seed) < transChance;
    if (transBounce == true)
    {
        float f0 = mat.m_F0.r;
        // Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to "Real-Time-Rendering" 4th editon on page 325.
        float f90 = clamp(f0 * 50.0, 0.0, 1.0);
        float VdotH = abs(dot(V, H));
        float F = F_Schlick(f0, f90, abs(dot(L, H)));
        specChance = F;
        float discriminat = mat.m_Eta * mat.m_Eta * (1.f - VdotH * VdotH); // (Total internal reflection)
        // Reflection/Total internal reflection
        if (discriminat > 1.f || rand(seed) < F)
        {
            specBounce = true;
        }
    }
    else
    {
        float newSpecChance = mat.m_SpecularWeight;
            
        if (rand(seed) < newSpecChance)
        {
            specBounce = true;
            specChance = (1.f - specChance) * newSpecChance;
        }
        else
        {
            // Diffuse
            specChance = specChance + newSpecChance - specChance * newSpecChance;
        }
    }
     // Evaluate a full BSDF
    return EvalBSDF(mat, V, N, L, H, specBounce, specChance, transBounce, transChance, pdf);
}