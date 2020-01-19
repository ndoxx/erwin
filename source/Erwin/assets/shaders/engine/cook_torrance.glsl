const float PI    = 3.14159265359f;
const float invPI = 0.318309886f;

float TrowbridgeReitzGGX(float NdotH, float roughness)
{
    float r2    = roughness*roughness;
    float r4    = r2*r2;
    float denom = (NdotH * NdotH * (r4 - 1.0f) + 1.0f);
    denom = PI * (denom * denom);

    return r4 / denom;
}

float SchlickGGX(float NdotV, float roughness)
{
    float r     = roughness + 1.0f;
    float k     = (r*r) * 0.125f;
    float denom = NdotV * (1.0f - k) + k;

    return NdotV / denom;
}

float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    float ggx2 = SchlickGGX(NdotV, roughness);
    float ggx1 = SchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Fresnel-Schlick approx.
vec3 FresnelSchlick(float VdotH, vec3 F0)
{
    float flipVdotH = 1.0f - VdotH;
    return ((1.0f - F0) * (flipVdotH*flipVdotH*flipVdotH*flipVdotH*flipVdotH)) + F0;
}

// Gaussian Spherical approx.
vec3 FresnelGS(float VdotH, vec3 F0)
{
    return ((1.0f - F0) * pow(2.0f, (-5.55473f*VdotH - 6.98316f)*VdotH)) + F0;
}

// BRDF
vec3 CookTorrance(vec3 radiance, // Light color
                  vec3 lightDir,
                  vec3 normal,
                  vec3 viewDir,
                  vec3 albedo,
                  float metallic,
                  float roughness)
{
    // Calculate reflectance at normal incidence; if dielectric (like plastic) use F0
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
    // Actual formula for F0 is: pow(abs((1.0-ior) / (1.0+ior)), 2.0) with ior= index of refraction
    vec3 F0 = mix(vec3(0.04f), albedo, metallic);

    vec3 halfwayDir = normalize(viewDir + lightDir);
    float NdotL = max(dot(normal, lightDir),    0.0000001f); // small number to prevent division by zero.
    float NdotV = max(dot(normal, viewDir),     0.0000001f);
    float NdotH = max(dot(normal, halfwayDir),  0.f);
    float VdotH = max(dot(viewDir, halfwayDir), 0.f);

    // Cook-Torrance BRDF
    float D = TrowbridgeReitzGGX(NdotH, roughness);
    float G = GeometrySmith(NdotV, NdotL, roughness);
    vec3  F = FresnelGS(VdotH, F0);

    vec3 specular = (D * G) * F;
    specular /= 4.0f * NdotV * NdotL;

    // kS is equal to Fresnel
    vec3 kS = F;
    // Energy conservation -> diffuse = 1 - specular
    vec3 kD = vec3(1.0f) - kS;
    // Metals have no diffuse component. Linear blend quasi-metals.
    kD = -metallic*kD + kD;
    kD *= invPI;

    // Outgoing radiance Lo = kD*f_Lambert + kS*f_Cook-Torrance
    // Specular term already multiplied by kS==F
    vec3 Lo = (kD * albedo) + specular;

    // scale light by NdotL
    return Lo * (NdotL * radiance);
}
