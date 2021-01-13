#ifndef _SHADER_DEFINITIONS_H_
#define _SHADER_DEFINITIONS_H_

#define PER_DRAW_CBUFFER_SLOT 0
#define PER_MATERIAL_CBUFFER_SLOT 1
#define PER_VIEW_CBUFFER_SLOT 2
#define PER_FRAME_CBUFFER_SLOT 3
#define POST_PROCESS_CBUFFER_SLOT 5

#define RENDERER_VARIABLES_CBUFFER_SLOT 7

#define UTILITY_CBUFFER_SLOT 10

// Texture slots
#define SHADOW_MAP_TEXTURE2D_SLOT 0
#define POINT_LIGHT_SHADOW_MAP_TEXTURECUBEARRAY_SLOT 1
#define SPOT_LIGHT_SHADOW_MAP_TEXTURE2DARRAY_SLOT 2

#define ALBEDO_TEXTURE2D_SLOT 3
#define ROUGHNESS_TEXTURE2D_SLOT 4
#define METALLIC_TEXTURE2D_SLOT 5
#define AO_TEXTURE2D_SLOT 6
#define METALLIC_ROUGHNESS_TEXTURE2D_SLOT 7
#define NORMAL_TEXTURE2D_SLOT 8
#define EMISSIVE_TEXTURE2D_SLOT 9

#define RADIANCE_TEXTURECUBE_SLOT 10
#define IRRADIANCE_TEXTURECUBE_SLOT 11
#define BRDF_LUT_TEXTURE_SLOT 12

#define SKYBOX_TEXTURECUBE_SLOT 5

#define POST_PROCESS_TEXTURE0_SLOT 10

// Deferred shading textures
#define GBUFFER0_TEXTURE2D_SLOT 25
#define GBUFFER1_TEXTURE2D_SLOT 26
#define GBUFFER2_TEXTURE2D_SLOT 27
#define GBUFFER3_TEXTURE2D_SLOT 28
#define GBUFFER4_TEXTURE2D_SLOT 29

// Samler state slots
#define SHADOW_SAMPLER_COMPARISON_STATE_SLOT 1
#define POINT_CLAMP_SAMPLER_STATE_SLOT 2
#define LINEAR_CLAMP_SAMPLER_STATE_SLOT 3
#define LINEAR_WRAP_SAMPLER_STATE_SLOT 4
#define POINT_WRAP_SAMPLER_STATE_SLOT 5
#define OBJECT_SAMPLER_STATE_SLOT 6


// Variable defines
#define MAX_SHADOW_CASCADE_COUNT 5

#ifdef __cplusplus
#define CBUFFER(name, registerNumber) struct name
#define TEXTURE2D(name, format, slotNumber)
#define RWTEXTURE2D(name, format, slotNumber)
#define TEXTURE2DARRAY(name, format, slotNumber)
#define RWTEXTURE2DARRAY(name, format, slotNumber)
#define TEXTURECUBE(name, format, slotNumber)
#define TEXTURECUBEARRAY(name, format, slotNumber)

#define SAMPLER_STATE(name, slotNumber)
#define SAMPLER_COMPARISON_STATE(name, slotNumber)
#else
#define Matrix4 row_major matrix
#define Vector4 float4
#define Vector3 float3
#define Vector2 float2
#define uint32_t uint
#define CBUFFER(name, slotNumber) cbuffer name : register(b ## slotNumber)
#define TEXTURE2D(name, format, slotNumber) Texture2D<format> name : register(t ## slotNumber)
#define RWTEXTURE2D(name, format, slotNumber) RWTexture2D<format> name : register(t ## slotNumber)
#define TEXTURE2DARRAY(name, format, slotNumber) Texture2DArray<format> name : register(t ## slotNumber)
#define RWTEXTURE2DARRAY(name, format, slotNumber) RWTexture2DArray<format> name : register(u ## slotNumber)
#define TEXTURECUBE(name, format, slotNumber) TextureCube<format> name : register(t ## slotNumber)
#define TEXTURECUBEARRAY(name, format, slotNumber) TextureCubeArray<format> name : register(t ## slotNumber)

#define SAMPLER_STATE(name, slotNumber) SamplerState name : register(s ## slotNumber)
#define SAMPLER_COMPARISON_STATE(name, slotNumber) SamplerComparisonState name : register(s ## slotNumber)
#endif

struct DrawMaterial {
    Vector4 ambientColor;
    Vector4 emissiveColor;
    Vector4 diffuseColor;

    float roughness;
    float metallic;

    uint32_t hasAlbedoTexture;
    uint32_t hasRoughnessTexture;
    uint32_t hasMetallicTexture;
    uint32_t hasAOTexture;

    uint32_t hasMetallicRoughnessTexture;
    uint32_t hasEmissiveTexture;
};

///////////// Constant buffers
CBUFFER(PerDrawGlobalConstantBuffer, PER_DRAW_CBUFFER_SLOT) {
    Matrix4 g_ModelMatrix;

    uint32_t g_PerDrawExtraData0; // Light index
    uint32_t g_PerDrawExtraData1;
    uint32_t g_PerDrawExtraData2;
    uint32_t g_PerDrawExtraData3;
};

CBUFFER(PerMaterialGlobalConstantBuffer, PER_MATERIAL_CBUFFER_SLOT) {
    DrawMaterial g_Material;
};

CBUFFER(PerViewGlobalConstantBuffer, PER_VIEW_CBUFFER_SLOT) {
    Matrix4 g_ViewMatrix;
    Matrix4 g_ProjMatrix;
    Matrix4 g_ProjViewMatrix;
    Vector4 g_CameraPos;

    Matrix4 g_InverseViewProjMatrix;
};

struct SpotLightData {
    Vector3 position;
    float minConeAngleCos;
    Vector3 direction;
    float maxConeAngleCos;
    Vector3 color;
    float intensity;

    float radius;
    Vector3 pad00302;
};

struct PointLightData {
    Vector3 position;
    float radius;
    Vector3 color;
    float intensity;
};

#define MAX_POINT_LIGHT_COUNT 8
#define MAX_SPOT_LIGHT_COUNT 8

CBUFFER(PerFrameGlobalConstantBuffer, PER_FRAME_CBUFFER_SLOT) {
    Vector4 g_DirectionLightDirection;
    Vector4 g_DirectionLightColor; // xyz color, w intensity
    Matrix4 g_DirectionLightProjViewMatrices[MAX_SHADOW_CASCADE_COUNT];

    PointLightData g_PointLights[MAX_POINT_LIGHT_COUNT];

    SpotLightData g_SpotLights[MAX_SPOT_LIGHT_COUNT];
    Matrix4 g_SpotLightProjViewMatrices[MAX_SPOT_LIGHT_COUNT];

    uint32_t g_PointLightCount;
    uint32_t g_SpotLightCount;
    float g_PointLightProjNearPlane;
    float g_PointLightProjFarPlane;

    Vector4 g_ScreenDimensions;
};


CBUFFER(PostProcessConstantBuffer, POST_PROCESS_CBUFFER_SLOT) {
    float g_PPExposure;
    float g_PPGamma;
    Vector2 pad0003;
};

CBUFFER(RendererVariablesConstantBuffer, RENDERER_VARIABLES_CBUFFER_SLOT) {
    uint32_t g_ShadowCascadeCount;
    uint32_t g_ShadowMapSize;

    // Debug
    uint32_t g_VisualizeCascades;
    uint32_t pad0000002;
};

/////////// Textures
TEXTURE2DARRAY(g_ShadowMapTexture, float, SHADOW_MAP_TEXTURE2D_SLOT);
TEXTURECUBEARRAY(g_PointLightShadowMapArray, float, POINT_LIGHT_SHADOW_MAP_TEXTURECUBEARRAY_SLOT);
TEXTURE2DARRAY(g_SpotLightShadowMapArray, float, SPOT_LIGHT_SHADOW_MAP_TEXTURE2DARRAY_SLOT);

// PBR textures
TEXTURE2D(g_AlbedoTexture, float4, ALBEDO_TEXTURE2D_SLOT);
TEXTURE2D(g_RoughnessTexture, float4, ROUGHNESS_TEXTURE2D_SLOT);
TEXTURE2D(g_MetallicTexture, float4, METALLIC_TEXTURE2D_SLOT);
TEXTURE2D(g_AOTexture, float4, AO_TEXTURE2D_SLOT);
TEXTURE2D(g_MetallicRoughnessTexture, float4, METALLIC_ROUGHNESS_TEXTURE2D_SLOT);
TEXTURE2D(g_NormalTexture, float4, NORMAL_TEXTURE2D_SLOT);
TEXTURE2D(g_EmissiveTexture, float4, EMISSIVE_TEXTURE2D_SLOT);

TEXTURECUBE(g_RadianceTexture, float4, RADIANCE_TEXTURECUBE_SLOT);
TEXTURECUBE(g_IrradianceTexture, float4, IRRADIANCE_TEXTURECUBE_SLOT);
TEXTURE2D(g_EnvBrdfTexture, float2, BRDF_LUT_TEXTURE_SLOT);

// Skybox
TEXTURECUBE(g_SkyboxTexture, float4, SKYBOX_TEXTURECUBE_SLOT);

// Post process textures
TEXTURE2D(g_PostProcessTexture0, float4, POST_PROCESS_TEXTURE0_SLOT);

// Deferred shading textures
TEXTURE2D(g_GBuffer0Texture, float4, GBUFFER0_TEXTURE2D_SLOT);
TEXTURE2D(g_GBuffer1Texture, float4, GBUFFER1_TEXTURE2D_SLOT);
TEXTURE2D(g_GBuffer2Texture, float4, GBUFFER2_TEXTURE2D_SLOT);
TEXTURE2D(g_GBuffer3Texture, float4, GBUFFER3_TEXTURE2D_SLOT);
TEXTURE2D(g_GBuffer4Texture, float4, GBUFFER4_TEXTURE2D_SLOT);


/////////// Samplers
SAMPLER_COMPARISON_STATE(g_ShadowMapSampler, SHADOW_SAMPLER_COMPARISON_STATE_SLOT);

SAMPLER_STATE(g_PointClampSampler,   POINT_CLAMP_SAMPLER_STATE_SLOT);
SAMPLER_STATE(g_PointWrapSampler,    POINT_WRAP_SAMPLER_STATE_SLOT);
SAMPLER_STATE(g_LinearWrapSampler,   LINEAR_WRAP_SAMPLER_STATE_SLOT);
SAMPLER_STATE(g_LinearClampSampler,  LINEAR_CLAMP_SAMPLER_STATE_SLOT);
SAMPLER_STATE(g_ObjectSampler,       OBJECT_SAMPLER_STATE_SLOT);

#endif
