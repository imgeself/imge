#include "pch.h"
#include "AssetLoading.h"
#include "Log.h"
#include "ApiRegistry.h"
#include "Platform.h"
#include "RHI.h"
#include "ecs.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

static APIRegistry* gAPIRegistry = nullptr;

struct AssetAPIState
{
    ILinearAllocator* tempAllocator;
};

static AssetAPIState* gState = nullptr;

static void LoadNode(RHIAPI* rhiAPI, EntityAPI* entityAPI, const cgltf_data* data, const cgltf_node* node, Component components[2],
                     MaterialComponentData* materials, GPUBuffer* buffers, EntityContext* entityContext, bool leftHandedNormalMap)
{

    // TODO: Parent transformation!!
    TransformComponentData transformComponentData = {};
    transformComponentData.translation = Vector3(0.0f, 0.0f, 0.0f);
    transformComponentData.scale = Vector3(1.0f, 1.0f, 1.0f);
    transformComponentData.orientation = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
    if (node->has_scale) {
        transformComponentData.scale *= Vector3(node->scale[0], node->scale[1], node->scale[2]);
    }
    if (node->has_translation) {
        transformComponentData.translation += Vector3(node->translation[0], node->translation[1], node->translation[2]);
    }
    if (node->has_rotation) {
        transformComponentData.orientation = Vector4(node->rotation[0], node->rotation[1], node->rotation[2], node->rotation[3]);
    }

    if (node->mesh)
    {
        const cgltf_mesh* gltfMesh = node->mesh;

        u32 numPrimitives = gltfMesh->primitives_count;
        for (u32 primitiveIndex = 0; primitiveIndex < numPrimitives; ++primitiveIndex)
        {
            const cgltf_primitive* prim = gltfMesh->primitives + primitiveIndex;
            const cgltf_accessor* indices = prim->indices;
            const cgltf_buffer_view* indexBufferView = indices->buffer_view;
            const cgltf_buffer* buffer = indexBufferView->buffer;

            MeshComponentData mesh = {};
            u64 materialIndex = 0;
            if (prim->material)
            {
                materialIndex = (u64) (prim->material - data->materials);
            }

            if (indices)
            {
                u64 bufferViewIndex =  (u64) (indices->buffer_view - data->buffer_views);
                mesh.indexBuffer = buffers[bufferViewIndex];
                mesh.indexBufferStride = (u32) indices->stride;
                mesh.indexStart = (u32) indices->offset / indices->stride;
                mesh.indexCount = (u32) indices->count;
            }

            bool hasTangents = false;
            cgltf_buffer_view* positionBufferView;
            cgltf_buffer_view* normalBufferView;
            cgltf_buffer_view* texCoordBufferView;

            u32 numAttributes = prim->attributes_count;
            for (u32 attributeIndex = 0; attributeIndex < numAttributes; ++attributeIndex)
            {
                const cgltf_attribute* attribute = prim->attributes + attributeIndex;
                const cgltf_attribute_type type = attribute->type;

                const cgltf_accessor* accessor = attribute->data;
                cgltf_buffer_view* bufferView = accessor->buffer_view;
                u64 bufferViewIndex = (u64) (bufferView - data->buffer_views);

                u32 stride = accessor->stride;
                u32 offset = accessor->offset;

                if (type == cgltf_attribute_type_position)
                {
                    ASSERT(stride == 12, "Unsupported position stride");
                    mesh.vertexBuffers[VertexBuffers::VERTEX_BUFFER_POSITIONS] = buffers[bufferViewIndex];
                    mesh.vertexStrides[VertexBuffers::VERTEX_BUFFER_POSITIONS] = stride;
                    mesh.vertexOffsets[VertexBuffers::VERTEX_BUFFER_POSITIONS] = offset;
                    positionBufferView = bufferView;

                }
                else if (type == cgltf_attribute_type_normal)
                {
                    ASSERT(stride == 12, "Unsupported normal stride");
                    mesh.vertexBuffers[VertexBuffers::VERTEX_BUFFER_NORMAL] = buffers[bufferViewIndex];
                    mesh.vertexStrides[VertexBuffers::VERTEX_BUFFER_NORMAL] = stride;
                    mesh.vertexOffsets[VertexBuffers::VERTEX_BUFFER_NORMAL] = offset;
                    normalBufferView = bufferView;
                }
                else if (type == cgltf_attribute_type_texcoord)
                {
                    ASSERT(stride == 8, "Unsupported texcoord stride");
                    mesh.vertexBuffers[VertexBuffers::VERTEX_BUFFER_TEXCOORD] = buffers[bufferViewIndex];
                    mesh.vertexStrides[VertexBuffers::VERTEX_BUFFER_TEXCOORD] = stride;
                    mesh.vertexOffsets[VertexBuffers::VERTEX_BUFFER_TEXCOORD] = offset;
                    texCoordBufferView = bufferView;
                }
                else if (type == cgltf_attribute_type_tangent)
                {
                    ASSERT(stride == 16, "Unsupported tangent stride");
                    mesh.vertexBuffers[VertexBuffers::VERTEX_BUFFER_TANGENT] = buffers[bufferViewIndex];
                    mesh.vertexStrides[VertexBuffers::VERTEX_BUFFER_TANGENT] = stride;
                    mesh.vertexOffsets[VertexBuffers::VERTEX_BUFFER_TANGENT] = offset;
                    hasTangents = true;
                }
            }

            void* componentDatas[3] = { &mesh, (materials + materialIndex), &transformComponentData };
            entityAPI->CreateEntityWithComponents(entityContext, components, componentDatas, 3);
        }
    }

    u32 numChildren = node->children_count;
    for (u32 childrenIndex = 0; childrenIndex < numChildren; ++childrenIndex)
    {
        const cgltf_node* children = data->nodes + childrenIndex;
        LoadNode(rhiAPI, entityAPI, data, children, components, materials, buffers, entityContext, leftHandedNormalMap);
    }
}

GPUTexture2D LoadGLTFImage(const cgltf_image* gltfImage, RHIAPI* rhiAPI, const char* directory, bool generateMips = true, bool srgb = false) {

    TempAllocator tempAllocator;
    tempAllocator.Printf("%s/%s", directory, gltfImage->uri);

    i32 width = 0;
    i32 height = 0;
    i32 comp = 0;
    u8* imageData = stbi_load(tempAllocator.buffer, &width, &height, &comp, 4);

    // Subresources
    SubresourceData subresource;
    subresource.data = imageData;
    subresource.memPitch = width * 4;
    subresource.memSlicePitch = 0;

    // TODO: Determine the format using image's pixel informations!
    Texture2DDesc initParams = {};
    initParams.arraySize = 1;
    initParams.format = srgb ? FORMAT_R8G8B8A8_UNORM_SRGB : FORMAT_R8G8B8A8_UNORM;
    initParams.width = width;
    initParams.height = height;
    initParams.sampleCount = 1;
    initParams.mipCount = generateMips ? 0 : 1;
    initParams.flags = GPUResourceFlags::BIND_SHADER_RESOURCE;
    if (generateMips) {
        initParams.flags |= (GPUResourceFlags::MISC_GENERATE_MIPS | GPUResourceFlags::BIND_RENDER_TARGET);
    } else {
        initParams.flags |= GPUResourceFlags::USAGE_IMMUTABLE;
    }

    return rhiAPI->CreateTexture2D(&initParams, &subresource, 0);
}


void LoadGLTFFile(const char* filepath, RHIAPI* rhiAPI, EntityAPI* entityAPI, EntityContext* entityContext)
{

    const char* filename = PathGetFilename(filepath);
    const char* extension = PathGetExtension(filepath);
    char fileDirectory[1024];
    strcpy(fileDirectory, filepath);
    PathRemoveFilename(fileDirectory);

    cgltf_options options = {};
    cgltf_data* data = NULL;
    cgltf_result result = cgltf_parse_file(&options, filepath, &data);

    if (result != cgltf_result_success) {
        return;
    }

    // Load textures
    u32 numGltfImages = data->images_count;
    GPUShaderResourceView* textures = (GPUShaderResourceView*) alloca(sizeof(GPUShaderResourceView) * numGltfImages);
    for (u32 imageIndex = 0; imageIndex < numGltfImages; ++imageIndex)
    {
        cgltf_image* gltfImage = data->images + imageIndex;

        GPUTexture2D texture = LoadGLTFImage(gltfImage, rhiAPI, fileDirectory, false, false);
        GPUResourceViewDesc resourceDesc = {};
        resourceDesc.firstArraySlice = 0;
        resourceDesc.firstMip = 0;
        resourceDesc.sliceArrayCount = 1;
        resourceDesc.mipCount = 1;
        GPUShaderResourceView resourceView = rhiAPI->CreateShaderResourceView(texture, resourceDesc, 0);
        textures[imageIndex] = resourceView;
    }

    // Load materials
    u32 numGltfMaterials = data->materials_count;
    MaterialComponentData* materials = (MaterialComponentData*) alloca(sizeof(MaterialComponentData) * numGltfMaterials);
    for (u32 materialIndex = 0; materialIndex < numGltfMaterials; ++materialIndex) {
        cgltf_material* gltfMaterial = data->materials + materialIndex;
        MaterialComponentData material = {};

        cgltf_pbr_metallic_roughness pbr = gltfMaterial->pbr_metallic_roughness;

        material.baseColor = Vector4((float)pbr.base_color_factor[0], (float)pbr.base_color_factor[1], (float)pbr.base_color_factor[2], (float)pbr.base_color_factor[3]);
        material.emissiveColor = Vector4((float)gltfMaterial->emissive_factor[0], (float)gltfMaterial->emissive_factor[1], (float)gltfMaterial->emissive_factor[2], 1.0f);
        material.roughness = (float) pbr.roughness_factor;
        material.metallic = (float) pbr.metallic_factor;

        cgltf_texture_view baseColor = pbr.base_color_texture;
        if (baseColor.texture) {
            const cgltf_texture* baseColorTexture = baseColor.texture;
            const cgltf_image* baseColorImage = baseColorTexture->image;
            u64 imageIndex = (u64) (baseColorImage - data->images);
            material.baseColorTexture = textures[imageIndex];
        }

        cgltf_texture_view metallicRoughness = pbr.metallic_roughness_texture;
        if (metallicRoughness.texture) {
            const cgltf_texture* metallicRoughnessTexture = metallicRoughness.texture;
            const cgltf_image* metallicRoughnessImage = metallicRoughnessTexture->image;
            u64 imageIndex = (u64) (metallicRoughnessImage - data->images);
            material.roughnessMetallicTexture = textures[imageIndex];
        }

        cgltf_texture_view emissive = gltfMaterial->emissive_texture;
        if (emissive.texture) {
            const cgltf_texture* emissiveTexture = emissive.texture;
            const cgltf_image* emissiveImage = emissiveTexture->image;
            u64 imageIndex = (u64) (emissiveImage - data->images);
            material.emissiveTexture = textures[imageIndex];
        }

        cgltf_texture_view occlusion = gltfMaterial->occlusion_texture;
        if (occlusion.texture) {
            const cgltf_texture* occlusionTexture = occlusion.texture;
            const cgltf_image* occlusionImage = occlusionTexture->image;
            u64 imageIndex = (u64) (occlusionImage - data->images);
            material.occlusionTexture = textures[imageIndex];
        }

        cgltf_texture_view normals = gltfMaterial->normal_texture;
        if (normals.texture) {
            const cgltf_texture* normalTexture = normals.texture;
            const cgltf_image* normalImage = normalTexture->image;
            u64 imageIndex = (u64) (normalImage - data->images);
            material.normalMapTexture = textures[imageIndex];
        }

        *(materials + materialIndex) = material;
    }


    // Load meshes
    cgltf_load_buffers(&options, data, filepath);
    u32 numBuffers = data->buffer_views_count;
    GPUBuffer* buffers = (GPUBuffer*) alloca(sizeof(GPUBuffer) * numBuffers);
    for (size_t bufferIndex = 0; bufferIndex < numBuffers; ++bufferIndex) {
        const cgltf_buffer_view* gltfBufferView = data->buffer_views + bufferIndex;
        const cgltf_buffer* gltfBuffer = gltfBufferView->buffer;

        uint32_t bufferFlags = GPUResourceFlags::USAGE_IMMUTABLE;
        if (gltfBufferView->type == cgltf_buffer_view_type_invalid) {
            continue;
        }
        switch (gltfBufferView->type) {
            case cgltf_buffer_view_type_vertices:
            {
                bufferFlags |= GPUResourceFlags::BIND_VERTEX_BUFFER;
            } break;
            case cgltf_buffer_view_type_indices:
            {
                bufferFlags |= GPUResourceFlags::BIND_INDEX_BUFFER;
            } break;
            default:
            {
                ASSERT(false, "Unsupported bufferView target");
            } break;
        }

        SubresourceData bufferSubresource = {};
        bufferSubresource.data = ((u8*) gltfBuffer->data) + gltfBufferView->offset;
        GPUBuffer buffer = rhiAPI->CreateBuffer(&bufferSubresource, gltfBufferView->size, bufferFlags, 0);

        *(buffers + bufferIndex) = buffer;
    }

    Component meshComponent = entityAPI->RegisterComponent(entityContext, MESH_COMPONENT_NAME, sizeof(MeshComponentData));
    Component materialComponent = entityAPI->RegisterComponent(entityContext, MATERIAL_COMPONENT_NAME, sizeof(MaterialComponentData));
    Component transformComponent = entityAPI->RegisterComponent(entityContext, TRANSFORM_COMPONENT_NAME, sizeof(TransformComponentData));
    Component components[3] = { meshComponent, materialComponent, transformComponent };

    const cgltf_scene* gltfScene = data->scene;
    u32 numNodes = gltfScene->nodes_count;
    for (u32 nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex) {
        const cgltf_node* node = data->nodes + nodeIndex;
        LoadNode(rhiAPI, entityAPI, data, node, components, materials, buffers, entityContext, false);
    }
    

}

void LoadAsset(EntityContext* context, const char* filename)
{
    RHIAPI* rhiAPI = (RHIAPI*) gAPIRegistry->Get(RHI_API_NAME);
    EntityAPI* entityAPI = (EntityAPI*) gAPIRegistry->Get(ENTITY_API_NAME);

    LoadGLTFFile(filename, rhiAPI, entityAPI, context);
}

extern "C"
{
    MODULE_EXPORT void LoadPlugin(APIRegistry* registry, bool reload)
    {
        gAPIRegistry = registry;
        AssetAPI assetAPI = {};
        assetAPI.LoadAsset = LoadAsset;

        registry->Set(ASSET_API_NAME, &assetAPI, sizeof(AssetAPI));
    }

    MODULE_EXPORT void UnloadPlugin(APIRegistry* registry, bool reload)
    {
        registry->Remove(ASSET_API_NAME);
    }
}