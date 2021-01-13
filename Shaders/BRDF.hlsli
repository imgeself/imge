#define PI 3.14159265f

// Diffuse
inline float3 DiffuseLambert() {
    return 1.0f / PI;
}

// Specular distrubution function
inline float DistributionGGX(float3 normalVector, float3 halfwayVector, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = saturate(dot(normalVector, halfwayVector));
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0f) + 1;
    denom = PI * denom * denom;

    return a2 / denom;
}

inline float GeometryGGX(float3 normalVector, float3 v, float k) {
    float NdotV = saturate(dot(normalVector, v));

    float denom = NdotV * (1.0f - k) + k;
    return NdotV / denom;
}

inline float GeometrySmith(float3 viewVector, float3 normalVector, float3 lightVector, float roughness) {
    float a = roughness * roughness;
    float k = pow((a + 1), 2) / 8.0f;

    float ggxL = GeometryGGX(normalVector, lightVector, k);
    float ggxV = GeometryGGX(normalVector, viewVector, k);

    return ggxL * ggxV;
}

inline float3 FresnelSchlick(float angle, float3 f0, float f90) {
    return f0 + (f90 - f0) * pow(1.0 - angle, 5.0f);
}

inline float3 FresnelSchlickRoughness(float angle, float3 F0, float roughness) {
    return F0 + (max((float3)(1.0 - roughness), F0) - F0) * pow(1.0 - angle, 5.0);
}

inline float DisneyDiffuse(float3 lightVector, float3 halfwayVector, float3 normalVector, float3 viewVector, float roughness) {
    float LdotH = saturate(dot(lightVector, halfwayVector));
    float NdotL = saturate(dot(lightVector, normalVector));
    float NdotV = saturate(dot(viewVector, normalVector));

    float energyBias = lerp(0, 0.5, roughness);
    float energyFactor = lerp(1.0, 1.0 / 1.51, roughness);
    float3 f0 = float3(1.0f, 1.0f, 1.0f);
    float f90 = energyBias + 2.0 * roughness * LdotH * LdotH;
    float lightScatter = FresnelSchlick(NdotL, f0, f90).r;
    float viewScatter = FresnelSchlick(NdotV, f0, f90).r;
    return lightScatter * viewScatter * energyFactor * (1.0 / PI);
}

float3 BRDF(float3 lightVector, float3 normalVector, float3 viewVector, float3 albedo, float roughness, float metallic) {
    float3 halfwayVector = normalize(viewVector + lightVector);

    float D = DistributionGGX(normalVector, halfwayVector, roughness);
    float G = GeometrySmith(viewVector, normalVector, lightVector, roughness);

    float3 f0 = lerp((float3) 0.04f, albedo, metallic);

    float VdotH = saturate(dot(viewVector, halfwayVector));
    float3 F = FresnelSchlick(VdotH, f0, 1.0f);

    float3 kS = F;
    float3 kD = 1.0f - kS;
    kD *= 1.0 - metallic;

    float NdotL = saturate(dot(normalVector, lightVector));

    float3 numerator = D * G * F;
    float3 denom = 4.0f * saturate(dot(normalVector, viewVector)) * NdotL;
    float3 specularBRDF = numerator / max(denom, 0.001f);

    float3 diffuseBRDF = kD * albedo * DisneyDiffuse(lightVector, halfwayVector, normalVector, viewVector, roughness);

    float3 brdf = diffuseBRDF + specularBRDF;
    float3 Lo = brdf * NdotL;

    return Lo;
}
