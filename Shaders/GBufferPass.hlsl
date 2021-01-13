#include "ShaderDefinitions.h"
#include "DeferredCommon.hlsli"


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
GBufferPassPixelShaderOutput PSMain(VSOut input) {
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

    GBuffer gbuffer;
    gbuffer.albedo = albedoColor;
    gbuffer.normal = normalVector;
    gbuffer.roughness = roughness;
    gbuffer.metallic = metallic;
    gbuffer.occlusion = ao;

    return FillGBuffer(gbuffer);
}