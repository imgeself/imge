#include "ShaderDefinitions.h"
#include "ShadowUtils.hlsli"
#include "BRDF.hlsli"


///////////////////////////////////////////////////////////////////////////
/////// VERTEX SHADER
///////////////////////////////////////////////////////////////////////////
struct VSInput {
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
#ifdef NORMAL_MAPPING
    float4 tangent : TANGENT;
#endif
};

struct VSOut {
    float3 normal : NORMAL;
    float3 worldPos : WORLDPOS;
    float2 texCoord : TEXCOORD;
#ifdef NORMAL_MAPPING
    float3x3 tbn : TANGENTMATRIX;
#endif
    float4 pos : SV_Position;
};

VSOut VSMain(VSInput input) {
    float4 transformedPosition = mul(g_ProjViewMatrix, mul(g_ModelMatrix, float4(input.pos, 1.0f)));

    float3 normalVector = mul((float3x3) g_ModelMatrix, input.normal);
    normalVector = normalize(normalVector);

    VSOut vertexOut;
    vertexOut.normal = normalVector;
    vertexOut.pos = transformedPosition;
    vertexOut.worldPos = (float3) mul(g_ModelMatrix, float4(input.pos, 1.0f));
    vertexOut.texCoord = input.texCoord;

#ifdef NORMAL_MAPPING
    float3 tangentVector = mul((float3x3) g_ModelMatrix, input.tangent.xyz);
    tangentVector = normalize(tangentVector);

    float3 bitangentVector = normalize(cross(normalVector, tangentVector) * input.tangent.w);

    float3x3 tbnMatrix = float3x3(tangentVector.x, bitangentVector.x, normalVector.x,
                                  tangentVector.y, bitangentVector.y, normalVector.y,
                                  tangentVector.z, bitangentVector.z, normalVector.z);

    vertexOut.tbn = tbnMatrix;
#endif

    return vertexOut;
}


///////////////////////////////////////////////////////////////////////////
/////// FRAGMENT SHADER
///////////////////////////////////////////////////////////////////////////
float4 PSMain(VSOut input) : SV_Target {
    float3 cameraPos = g_CameraPos.xyz;

    float3 viewVector = normalize(cameraPos - input.worldPos);

    float3 albedoColor = g_Material.diffuseColor.rgb;
    float alpha = g_Material.diffuseColor.a;
    if (g_Material.hasAlbedoTexture) {
        float4 color = g_AlbedoTexture.Sample(g_ObjectSampler, input.texCoord);
        albedoColor = color.rgb;
        alpha = color.a;
    }

#ifdef ALPHA_TEST
    clip(alpha - 0.1f);
#endif

    float3 emissiveColor = g_Material.emissiveColor.rgb;
    if (g_Material.hasEmissiveTexture) {
        emissiveColor = g_EmissiveTexture.Sample(g_ObjectSampler, input.texCoord).rgb;
    }

    float roughness = g_Material.roughness;
    if (g_Material.hasRoughnessTexture) {
        roughness = g_RoughnessTexture.Sample(g_ObjectSampler, input.texCoord).r;
    }

    float metallic = g_Material.metallic;
    if (g_Material.hasMetallicTexture) {
        metallic = g_MetallicTexture.Sample(g_ObjectSampler, input.texCoord).r;
    }

    if (g_Material.hasMetallicRoughnessTexture) {
        float4 t = g_MetallicRoughnessTexture.Sample(g_ObjectSampler, input.texCoord);
        // https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
        metallic = t.b;
        roughness = t.g;
    }

    float ao = 1.0f;
    if (g_Material.hasAOTexture) {
        ao = g_AOTexture.Sample(g_ObjectSampler, input.texCoord).r;
    }

#ifdef NORMAL_MAPPING
    float3 normalMap = g_NormalTexture.Sample(g_ObjectSampler, input.texCoord).xyz;
    normalMap = normalMap * 2.0f - 1.0f;

    float3 normalVector = normalize(normalMap);
    normalVector = normalize(mul(input.tbn, normalVector));
#else
    float3 normalVector = normalize(input.normal);
#endif

    float3 surfaceColor = float3(0.0f, 0.0f, 0.0f);
    float3 cascadeColor = float3(1.0f, 1.0f, 1.0f);
    // Directional Light
    {
        float3 lightVector = normalize(-g_DirectionLightDirection.xyz);
        float3 Lo = BRDF(lightVector, normalVector, viewVector, albedoColor, roughness, metallic);

        uint hitCascadeIndex;
        float shadowFactor = CalculateShadowValue(input.worldPos, hitCascadeIndex);

        // Visualize cascades
        float3 cascadeVisualizeColors[4] = {
                    1.0f, 0.0f, 0.0f,
                    1.0f, 0.0f, 1.0f,
                    0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 1.0f,
        };

        float3 lightColor = g_DirectionLightColor.rgb;
        float intensity = g_DirectionLightColor.a;
        cascadeColor = cascadeVisualizeColors[hitCascadeIndex];

        float3 color = Lo * lightColor * intensity * shadowFactor;
        surfaceColor += color;
    }

    // Point lights
    for (uint pointLightIndex = 0; pointLightIndex < g_PointLightCount; pointLightIndex++) {
        PointLightData light = g_PointLights[pointLightIndex];

        float3 unnormalizedLightVector = light.position.xyz - input.worldPos;
        float distanceSq = dot(unnormalizedLightVector, unnormalizedLightVector);
        float attenuation = 1 / distanceSq;

        float3 lightVector = normalize(unnormalizedLightVector);
        float3 Lo = BRDF(lightVector, normalVector, viewVector, albedoColor, roughness, metallic);

        float3 lightColor = light.color;
        float intensity = light.intensity;

        float shadowFactor = CalculatePointLightShadow(unnormalizedLightVector, pointLightIndex);

        float3 color = Lo * lightColor * intensity * attenuation * shadowFactor;
        surfaceColor += color;
    }

    // Spot lights
    for (uint spotLightIndex = 0; spotLightIndex < g_SpotLightCount; spotLightIndex++) {
        SpotLightData light = g_SpotLights[spotLightIndex];

        float3 lightVector = light.position - input.worldPos;
        lightVector = normalize(lightVector);

        float minConeAngleCos = light.minConeAngleCos;
        float maxConeAngleCos = light.maxConeAngleCos;

        float coneAngle = dot(lightVector, normalize(-light.direction));
        float smooth = smoothstep(maxConeAngleCos, minConeAngleCos, coneAngle);
        float3 Lo = BRDF(lightVector, normalVector, viewVector, albedoColor, roughness, metallic);

        float3 lightColor = light.color;
        float intensity = light.intensity;

        float shadowFactor = CalculateSpotLightShadow(input.worldPos, spotLightIndex);

        float3 color = Lo * lightColor * intensity * smooth * shadowFactor;
        surfaceColor += color;
    }

    float3 f0 = lerp((float3) 0.04f, albedoColor, metallic);
    float NdotV = saturate(dot(normalVector, viewVector));
    float3 kS = FresnelSchlickRoughness(NdotV, f0, roughness);
    float3 kD = 1.0f - kS;
    kD *= 1.0 - metallic;

    float3 eyeVector = -viewVector;
    float3 reflectedVector = reflect(eyeVector, normalVector);
    float3 radiance = g_RadianceTexture.SampleLevel(g_LinearWrapSampler, reflectedVector, roughness * 6.0f).rgb;
    float3 irradiance = g_IrradianceTexture.Sample(g_LinearWrapSampler, normalVector).rgb;
    
    float2 lutVector = float2(NdotV, roughness);
    float2 envBRDF = g_EnvBrdfTexture.Sample(g_LinearClampSampler, lutVector).rg;
    
    float3 diffuseAmbient = kD * (irradiance * albedoColor);
    float3 ambient = diffuseAmbient + radiance * (kS * envBRDF.x + envBRDF.y);
    ambient *= ao * 0.5f;

    float3 color = surfaceColor + ambient + emissiveColor;

    if (g_VisualizeCascades) {
        return float4(color * cascadeColor, alpha);
    } else {
        return float4(color, alpha);
    }
}
