// Referenced from https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/6546208a521eebff504929ce5bfa43da7c43eaee/source/Renderer/shaders/brdf.glsl
#define PI 3.141592653589f
#define invPI 0.318309886183f
// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
float3 F_Schlick(float3 f0, float3 f90, float VdotH)
{
    return f0 + (f90 - f0) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);
}
float F_Schlick(float f0, float f90, float VdotH)
{
    return f0 + (f90 - f0) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);
}
float3 F_SchlickDiel(float f0, float VdotH)
{
    float sinSq = f0 * f0 * (1.0 - VdotH * VdotH);
 
    // Total internal reflection
    if (sinSq > 1.0)
            return 1.0;
 
    float cosT = sqrt(max(1.0 - sinSq, 0.0));
 
    float rs = (f0 * cosT - VdotH) / (f0 * cosT + VdotH);
    float rp = (f0 * VdotH - cosT) / (f0 * VdotH + cosT);
 
    return 0.5f * (rs * rs + rp * rp);
}
// Smith Joint GGX
// Note: Vis = G / (4 * NdotL * NdotV)
// see Eric Heitz. 2014. Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs. Journal of Computer Graphics Techniques, 3
// see Real-Time Rendering. Page 331 to 336.
// see https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf/geometricshadowing(specularg)
float V_GGX(float NdotL, float NdotV, float alphaRoughness)
{
    float alphaRoughnessSq = alphaRoughness * alphaRoughness;

    float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);
    float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);

    float GGX = GGXV + GGXL;
    if (GGX > 0.0)
    {
        return 0.5 / GGX;
    }
    return 0.0;
}

float Smith_G_GGX(float NdotV, float alphaRoughness)
{
    float alphaRoughnessSq = alphaRoughness * alphaRoughness;
    float dotSq = NdotV * NdotV;
    return 1.f / (NdotV + sqrt(alphaRoughnessSq + dotSq - alphaRoughnessSq * dotSq));

}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float D_GGX(float NdotH, float alphaRoughness)
{
    float alphaRoughnessSq = alphaRoughness * alphaRoughness;
    float f = (NdotH * NdotH) * (alphaRoughnessSq - 1.0) + 1.0;
    return (alphaRoughnessSq / (f * f)) * invPI;
}

float3 BRDF_LambertianSimple(float3 diffuseColor)
{
    return diffuseColor * invPI;
}