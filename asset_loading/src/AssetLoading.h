#pragma once

enum VertexBuffers : uint8_t {
    VERTEX_BUFFER_POSITIONS,
    VERTEX_BUFFER_NORMAL,
    VERTEX_BUFFER_TEXCOORD,
    VERTEX_BUFFER_TANGENT,

    VERTEX_BUFFER_COUNT
};

#define MATERIAL_COMPONENT_NAME "MaterialComponent"
struct MaterialComponentData
{
    Vector4 baseColor;
    Vector4 emissiveColor;
    f32 roughness;
    f32 metallic;

    GPUShaderResourceView baseColorTexture;
    GPUShaderResourceView roughnessTexture;
    GPUShaderResourceView metallicTexture;
    GPUShaderResourceView roughnessMetallicTexture;
    GPUShaderResourceView occlusionTexture;
    GPUShaderResourceView normalMapTexture;
    GPUShaderResourceView emissiveTexture;
};

#define MESH_COMPONENT_NAME "MeshComponent"
struct MeshComponentData
{
    GPUBuffer vertexBuffers[VertexBuffers::VERTEX_BUFFER_COUNT];
    uint32_t vertexStrides[VertexBuffers::VERTEX_BUFFER_COUNT] = {0};
    uint32_t vertexOffsets[VertexBuffers::VERTEX_BUFFER_COUNT] = {0};


    GPUBuffer indexBuffer;
    uint32_t indexBufferStride = 0;
    uint32_t indexBufferOffset = 0;

    uint32_t indexStart = 0;
    uint32_t indexCount = 0;
};

#define TRANSFORM_COMPONENT_NAME "TransformComponent"
struct TransformComponentData
{
    Vector3 scale;
    Vector3 translation;
    Vector4 orientation;
};

struct EntityContext;

#define ASSET_API_NAME "AssetAPI"

struct AssetAPI
{
    void (*LoadAsset)(EntityContext* entityContext, const char* filename);
};