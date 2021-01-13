#include "pch.h"
#include "System.h"
#include "RHI.h"
#include "Platform.h"
#include "ApiRegistry.h"
#include "Allocator.h"
#include "imgui.h"
#include "imgui_api.h"
#include "Log.h"
#include "Profiler.h"
#include "ecs.h"
#include "AssetLoading.h"
#include "ShaderDefinitions.h"

#include <d3dcompiler.h>

struct SystemState
{
    ILinearAllocator* appAllocator;
    ILinearAllocator* frameAllocator;
    PlatformWindow* mainWindow;

    // NOTE: All of this for demo rendering
    GPUViewport viewport;
    GPUGraphicsPipelineState graphicsPSO;
    GPUBuffer perFrameGlobalConstantBuffer;
    GPUBuffer perMaterialGlobalConstantBuffer;
    GPUBuffer perDrawGlobalConstantBuffer;
    GPUBuffer perViewGlobalConstantBuffer;
    GPUSamplerState defaultSamplerStates[8];

    Vector3 cameraPos;
    Matrix4 cameraProjection;
    Matrix4 cameraView;

    EntityContext* context;
};

static APIRegistry* gAPIRegistry = nullptr;
static SystemState* gState = nullptr;
static ProfilerAPI* gProfilerAPI = nullptr;

struct FooComponent
{
    float x, y;
} foo;

struct BooComponent
{
    float x, y, z;
} boo;

// Demo rendering with ECS
// TODO: This is just for demo. I will be implementing a proper scene rendering system
void RenderSystemUpdate(EntityContext* context, EntitySystemUpdateSet* updateData, void* userData)
{
    RHIAPI* rhiAPI = (RHIAPI*) gAPIRegistry->Get(RHI_API_NAME);
    SystemState* systemState = (SystemState*) userData;

    {
        GPUBuffer vsConstantBuffers[8] = {};
        GPUBuffer psConstantBuffers[8] = {};

        vsConstantBuffers[PER_DRAW_CBUFFER_SLOT] = systemState->perDrawGlobalConstantBuffer;
        vsConstantBuffers[PER_VIEW_CBUFFER_SLOT] = systemState->perViewGlobalConstantBuffer;
        vsConstantBuffers[PER_FRAME_CBUFFER_SLOT] = systemState->perFrameGlobalConstantBuffer;

        psConstantBuffers[PER_MATERIAL_CBUFFER_SLOT] = systemState->perMaterialGlobalConstantBuffer;
        psConstantBuffers[PER_VIEW_CBUFFER_SLOT] = systemState->perViewGlobalConstantBuffer;
        psConstantBuffers[PER_FRAME_CBUFFER_SLOT] = systemState->perFrameGlobalConstantBuffer;

        rhiAPI->SetConstantBuffersVS(0, vsConstantBuffers, 8);
        rhiAPI->SetConstantBuffersPS(0, psConstantBuffers, 8);

        rhiAPI->SetSamplerStatesPS(0, gState->defaultSamplerStates, 8);
    }

    {
        PerFrameGlobalConstantBuffer perFrameData = {};
        perFrameData.g_DirectionLightColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        perFrameData.g_DirectionLightDirection = Vector4(-2.0f, 5.0f, -3.0f, 0.0f);
        void* perFramePointer = rhiAPI->MapBuffer(systemState->perFrameGlobalConstantBuffer);
        memcpy(perFramePointer, &perFrameData, sizeof(PerFrameGlobalConstantBuffer));
        rhiAPI->UnmapBuffer(systemState->perFrameGlobalConstantBuffer);
    }

    {
        PerViewGlobalConstantBuffer perViewData = {};
        perViewData.g_CameraPos = Vector4(systemState->cameraPos, 0.0f);
        perViewData.g_ViewMatrix = systemState->cameraView;
        perViewData.g_ProjMatrix = systemState->cameraProjection;
        perViewData.g_ProjViewMatrix = systemState->cameraProjection * systemState->cameraView;
        void* perViewPointer = rhiAPI->MapBuffer(systemState->perViewGlobalConstantBuffer);
        memcpy(perViewPointer, &perViewData, sizeof(PerViewGlobalConstantBuffer));
        rhiAPI->UnmapBuffer(systemState->perViewGlobalConstantBuffer);
    }

    for (u32 arrayIndex = 0; arrayIndex < updateData->numArrays; ++arrayIndex)
    {
        EntitySystemUpdateArray* updateArray = updateData->arrays + arrayIndex;
        for (u32 index = 0; index < updateArray->length; ++index)
        {

            MeshComponentData* mesh = (MeshComponentData*) updateArray->componentData[0];
            MaterialComponentData* material = (MaterialComponentData*) updateArray->componentData[1];
            TransformComponentData* transform = (TransformComponentData*) updateArray->componentData[2];

            Matrix4 translateMatrix = TranslateMatrix(transform->translation);
            Matrix4 scaleMatrix = ScaleMatrix(transform->scale);
            Matrix4 rotationMatrix;
            float w, x, y, z,
            xx, yy, zz,
            xy, yz, xz,
            wx, wy, wz, norm, s;

            norm = sqrtf(DotProduct(transform->orientation, transform->orientation));
            s = norm > 0.0f ? 2.0f / norm : 0.0f;

            x = transform->orientation[0];
            y = transform->orientation[1];
            z = transform->orientation[2];
            w = transform->orientation[3];

            xx = s * x * x;   xy = s * x * y;   wx = s * w * x;
            yy = s * y * y;   yz = s * y * z;   wy = s * w * y;
            zz = s * z * z;   xz = s * x * z;   wz = s * w * z;

            rotationMatrix[0][0] = 1.0f - yy - zz;
            rotationMatrix[1][1] = 1.0f - xx - zz;
            rotationMatrix[2][2] = 1.0f - xx - yy;

            rotationMatrix[0][1] = xy + wz;
            rotationMatrix[1][2] = yz + wx;
            rotationMatrix[2][0] = xz + wy;

            rotationMatrix[1][0] = xy - wz;
            rotationMatrix[2][1] = yz - wx;
            rotationMatrix[0][2] = xz - wy;

            rotationMatrix[0][3] = 0.0f;
            rotationMatrix[1][3] = 0.0f;
            rotationMatrix[2][3] = 0.0f;
            rotationMatrix[3][0] = 0.0f;
            rotationMatrix[3][1] = 0.0f;
            rotationMatrix[3][2] = 0.0f;
            rotationMatrix[3][3] = 1.0f;

            Matrix4 transformMatrix =  translateMatrix * rotationMatrix * scaleMatrix;

            {
                PerDrawGlobalConstantBuffer perDrawData = {};
                perDrawData.g_ModelMatrix = transformMatrix;
                void* perDrawPointer = rhiAPI->MapBuffer(systemState->perDrawGlobalConstantBuffer);
                memcpy(perDrawPointer, &perDrawData, sizeof(PerDrawGlobalConstantBuffer));
                rhiAPI->UnmapBuffer(systemState->perDrawGlobalConstantBuffer);
            }

            {
                DrawMaterial drawMaterial = {};
                drawMaterial.diffuseColor = material->baseColor;
                drawMaterial.emissiveColor = material->emissiveColor;
                drawMaterial.metallic = material->metallic;
                drawMaterial.roughness = material->roughness;
                drawMaterial.hasAlbedoTexture = material->baseColorTexture.handle != INVALID_HANDLE;
                drawMaterial.hasMetallicTexture = material->metallicTexture.handle != INVALID_HANDLE;
                drawMaterial.hasRoughnessTexture = material->roughnessTexture.handle != INVALID_HANDLE;
                drawMaterial.hasMetallicRoughnessTexture = material->roughnessMetallicTexture.handle != INVALID_HANDLE;
                drawMaterial.hasAOTexture = material->occlusionTexture.handle != INVALID_HANDLE;
                drawMaterial.hasEmissiveTexture = material->emissiveTexture.handle != INVALID_HANDLE;

                PerMaterialGlobalConstantBuffer perMaterialData = {};
                perMaterialData.g_Material = drawMaterial;
                void* perMaterialPointer = rhiAPI->MapBuffer(systemState->perMaterialGlobalConstantBuffer);
                memcpy(perMaterialPointer, &perMaterialData, sizeof(PerMaterialGlobalConstantBuffer));
                rhiAPI->UnmapBuffer(systemState->perMaterialGlobalConstantBuffer);
            }

            rhiAPI->SetVertexBuffers(mesh->vertexBuffers, 0, VERTEX_BUFFER_COUNT, mesh->vertexStrides, mesh->vertexOffsets);
            rhiAPI->SetIndexBuffer(mesh->indexBuffer, mesh->indexBufferStride, mesh->indexBufferOffset);

            GPUShaderResourceView resourceViews[16] = {};
            resourceViews[ALBEDO_TEXTURE2D_SLOT] = material->baseColorTexture;
            resourceViews[ROUGHNESS_TEXTURE2D_SLOT] = material->roughnessTexture;
            resourceViews[METALLIC_TEXTURE2D_SLOT] = material->metallicTexture;
            resourceViews[AO_TEXTURE2D_SLOT] = material->occlusionTexture;
            resourceViews[METALLIC_ROUGHNESS_TEXTURE2D_SLOT] = material->roughnessMetallicTexture;
            resourceViews[NORMAL_TEXTURE2D_SLOT] = material->normalMapTexture;
            resourceViews[EMISSIVE_TEXTURE2D_SLOT] = material->emissiveTexture;

            rhiAPI->SetShaderResourcesPS(0, resourceViews, 16);
            rhiAPI->DrawIndexed(mesh->indexCount, mesh->indexStart, 0);
        }
    }
}

bool RenderSystemFilter(EntityContext* context, Component* components, u32 numComponents, EntitySignature signature)
{
    return HasEntitySignatureComponent(&signature, components[0]) && HasEntitySignatureComponent(&signature, components[1]);
}

void SystemInitialize(ILinearAllocator* applicationAllocator)
{

    PlatformAPI* platformAPI = (PlatformAPI*) gAPIRegistry->Get(PLATFORM_API_NAME);

    platformAPI->LoadPlugin(gAPIRegistry, "RHI");
    RHIAPI* rhiAPI = (RHIAPI*) gAPIRegistry->Get(RHI_API_NAME);

    AllocatorAPI* allocatorAPI = (AllocatorAPI*) gAPIRegistry->Get(ALLOCATOR_API_NAME);
    gState = (SystemState*) applicationAllocator->Alloc(applicationAllocator->instance, sizeof(SystemState));

    gProfilerAPI->Init(allocatorAPI, applicationAllocator);

    // Update registry
    SystemAPI* api = (SystemAPI*) gAPIRegistry->Get(SYSTEM_API_NAME);
    api->state = (void*) gState;

    gState->appAllocator = applicationAllocator;
    gState->frameAllocator = allocatorAPI->CreateLinearAllocator(Megabyte(50), Megabyte(1));

    WindowAPI* windowAPI = platformAPI->windowAPI;

    WindowSize size = { 1280, 720 };
    gState->mainWindow = windowAPI->CreatePlatformWindow(applicationAllocator, "Imge", size);

    rhiAPI->Init(applicationAllocator, gState->mainWindow);

    platformAPI->LoadPlugin(gAPIRegistry, "ECS");
    EntityAPI* entityAPI = (EntityAPI*) gAPIRegistry->Get(ENTITY_API_NAME);

    EntityContext* context = entityAPI->CreateContext(applicationAllocator);
    gState->context = context;

    platformAPI->LoadPlugin(gAPIRegistry, "asset_loading");
    AssetAPI* assetAPI = (AssetAPI*) gAPIRegistry->Get(ASSET_API_NAME);
    assetAPI->LoadAsset(context, "DamagedHelmet/DamagedHelmet.gltf");

    FooComponent foo = { 31, 13};
    Component fooComponent = entityAPI->RegisterComponent(context, "FooComponent", sizeof(FooComponent));

    BooComponent boo = { 55, 66, 77};
    Component booComponent = entityAPI->RegisterComponent(context, "BooComponent", sizeof(BooComponent));


    Component components[2] = { fooComponent, booComponent };
    void* componentDatas[2] = { &foo, &boo };
    entityAPI->CreateEntityWithComponents(context, components, componentDatas, 2);

    FooComponent foo2 = { 11, 12};

    Component components2[1] = { fooComponent};
    void* componentDatas2[1] = { &foo2 };
    entityAPI->CreateEntityWithComponents(context, components2, componentDatas2, 1);


    Component meshComponent = entityAPI->RegisterComponent(context, MESH_COMPONENT_NAME, sizeof(MeshComponentData));
    Component materialComponent = entityAPI->RegisterComponent(context, MATERIAL_COMPONENT_NAME, sizeof(MaterialComponentData));
    Component transformComponent = entityAPI->RegisterComponent(context, TRANSFORM_COMPONENT_NAME, sizeof(TransformComponentData));

    // TODO: We create this system struct with Update and Filter function pointers. BUT these functions will be invalidated when we do a hotreload.
    // And this struct will be still pointing old Update function pointers.
    // Maybe use APIRegistry for this as well? When we do a hotreload update these functions?
    IEntitySystem demoSystem = {};
    demoSystem.components[0] = meshComponent;
    demoSystem.components[1] = materialComponent;
    demoSystem.components[2] = transformComponent;
    demoSystem.numComponent = 3;
    demoSystem.Filter = RenderSystemFilter;
    demoSystem.Update = RenderSystemUpdate;
    demoSystem.userData = (void*) gState;

    entityAPI->PushSystem(context, &demoSystem);


    platformAPI->LoadPlugin(gAPIRegistry, "imgui");
    ImguiAPI* imguiAPI = (ImguiAPI*) gAPIRegistry->Get(IMGUI_API_NAME);
    imguiAPI->Init(applicationAllocator, gState->mainWindow);

    LogAPI* logAPI = (LogAPI*) gAPIRegistry->Get(LOG_API_NAME);
    logAPI->Init(allocatorAPI, gState->appAllocator);

    // Demo rendering initialization
    // TODO: Implement proper shader API and shader system.
    ID3DBlob* shaderBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    D3DCompileFromFile(L"Shaders/PBRForward.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_0", D3DCOMPILE_DEBUG, 0, &shaderBlob, &errorBlob);
    if (errorBlob)
    {
        printf("Shader compilation errors: %s\n", (char*)errorBlob->GetBufferPointer());
        ASSERT(false, "Shader compile error");
        errorBlob->Release();
    }

    GPUShaderBytecode vertexByteCode = {};
    vertexByteCode.shaderBytecode = (u8*) shaderBlob->GetBufferPointer();
    vertexByteCode.bytecodeLength = shaderBlob->GetBufferSize();

    ID3DBlob* pixelShaderBlob = nullptr;
    ID3DBlob* pixelErrorBlob = nullptr;
    D3DCompileFromFile(L"Shaders/PBRForward.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_0", D3DCOMPILE_DEBUG, 0, &pixelShaderBlob, &pixelErrorBlob);
    if (pixelErrorBlob)
    {
        printf("Shader compilation errors: %s\n", (char*)pixelErrorBlob->GetBufferPointer());
        ASSERT(false, "Shader compile error");
        pixelErrorBlob->Release();
    }

    GPUShaderBytecode pixelByteCode = {};
    pixelByteCode.shaderBytecode = (u8*) pixelShaderBlob->GetBufferPointer();
    pixelByteCode.bytecodeLength = pixelShaderBlob->GetBufferSize();

    GPUVertexInputElement inputElement = {};
    inputElement.format = FORMAT_R32G32B32_FLOAT;
    inputElement.classification = GPUInputClassification::PER_VERTEX_DATA;
    inputElement.inputSlot = 0;
    inputElement.semanticIndex = 0;
    inputElement.semanticName = "POSITION";

    GPUVertexInputElement inputElementNormal = {};
    inputElementNormal.format = FORMAT_R32G32B32_FLOAT;
    inputElementNormal.classification = GPUInputClassification::PER_VERTEX_DATA;
    inputElementNormal.inputSlot = 1;
    inputElementNormal.semanticIndex = 0;
    inputElementNormal.semanticName = "NORMAL";

    GPUVertexInputElement inputElementTexCoord = {};
    inputElementTexCoord.format = FORMAT_R32G32_FLOAT;
    inputElementTexCoord.classification = GPUInputClassification::PER_VERTEX_DATA;
    inputElementTexCoord.inputSlot = 2;
    inputElementTexCoord.semanticIndex = 0;
    inputElementTexCoord.semanticName = "TEXCOORD";

    GPUVertexInputElement inputElements[3] = { inputElement, inputElementNormal, inputElementTexCoord };
    GPUInputLayoutDesc inputLayoutDesc = {};
    inputLayoutDesc.elements = inputElements;
    inputLayoutDesc.elementCount = 3;

    GPUGraphicsPipelineStateDesc pipelineDesc = {};
    pipelineDesc.vertexShader = vertexByteCode;
    pipelineDesc.pixelShader = pixelByteCode;
    pipelineDesc.inputLayoutDesc = inputLayoutDesc;
    pipelineDesc.primitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    gState->graphicsPSO = rhiAPI->CreateGraphicsPipelineState(pipelineDesc, 0);

    WindowSize clientSize = windowAPI->GetPlatformWindowClientSize(gState->mainWindow);
    GPUViewport viewport = {};
    viewport.width = clientSize.clientWidth;
    viewport.height = clientSize.clientHeight;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    gState->viewport = viewport;

    windowAPI->ShowPlatformWindow(gState->mainWindow);

    SamplerStateDesc shadowSamplerStateInitParams = {};
    shadowSamplerStateInitParams.filterMode = TextureFilterMode_MIN_MAG_LINEAR_MIP_POINT;
    shadowSamplerStateInitParams.addressModeU = TextureAddressMode_CLAMP;
    shadowSamplerStateInitParams.addressModeV = TextureAddressMode_CLAMP;
    shadowSamplerStateInitParams.addressModeW = TextureAddressMode_CLAMP;
    shadowSamplerStateInitParams.comparisonFunction = COMPARISON_LESS;
    gState->defaultSamplerStates[SHADOW_SAMPLER_COMPARISON_STATE_SLOT] = rhiAPI->CreateSamplerState(&shadowSamplerStateInitParams, 0);

    SamplerStateDesc samplerStateInitParams = {};
    samplerStateInitParams.filterMode = TextureFilterMode_MIN_MAG_MIP_POINT;
    samplerStateInitParams.addressModeU = TextureAddressMode_CLAMP;
    samplerStateInitParams.addressModeV = TextureAddressMode_CLAMP;
    samplerStateInitParams.addressModeW = TextureAddressMode_CLAMP;
    gState->defaultSamplerStates[POINT_CLAMP_SAMPLER_STATE_SLOT] = rhiAPI->CreateSamplerState(&samplerStateInitParams, 0);

    samplerStateInitParams.filterMode = TextureFilterMode_MIN_MAG_MIP_LINEAR;
    gState->defaultSamplerStates[LINEAR_CLAMP_SAMPLER_STATE_SLOT] = rhiAPI->CreateSamplerState(&samplerStateInitParams, 0);

    samplerStateInitParams.addressModeU = TextureAddressMode_WRAP;
    samplerStateInitParams.addressModeV = TextureAddressMode_WRAP;
    samplerStateInitParams.addressModeW = TextureAddressMode_WRAP;
    gState->defaultSamplerStates[LINEAR_WRAP_SAMPLER_STATE_SLOT] = rhiAPI->CreateSamplerState(&samplerStateInitParams, 0);

    samplerStateInitParams.filterMode = TextureFilterMode_MIN_MAG_MIP_POINT;
    gState->defaultSamplerStates[POINT_WRAP_SAMPLER_STATE_SLOT] = rhiAPI->CreateSamplerState(&samplerStateInitParams, 0);

    SamplerStateDesc objectSamplerStateInitParams = {};
    objectSamplerStateInitParams.filterMode = TextureFilterMode_ANISOTROPIC;
    objectSamplerStateInitParams.addressModeU = TextureAddressMode_WRAP;
    objectSamplerStateInitParams.addressModeV = TextureAddressMode_WRAP;
    objectSamplerStateInitParams.addressModeW = TextureAddressMode_WRAP;
    objectSamplerStateInitParams.maxAnisotropy = 16;
    gState->defaultSamplerStates[OBJECT_SAMPLER_STATE_SLOT] = rhiAPI->CreateSamplerState(&objectSamplerStateInitParams, 0);

    u32 constantBufferFlags = USAGE_DYNAMIC | CPU_ACCESS_WRITE | BIND_CONSTANT_BUFFER;
    gState->perFrameGlobalConstantBuffer = rhiAPI->CreateBuffer(nullptr, sizeof(PerFrameGlobalConstantBuffer), constantBufferFlags, 0);
    gState->perMaterialGlobalConstantBuffer = rhiAPI->CreateBuffer(nullptr, sizeof(PerMaterialGlobalConstantBuffer), constantBufferFlags, 0);
    gState->perDrawGlobalConstantBuffer = rhiAPI->CreateBuffer(nullptr, sizeof(PerDrawGlobalConstantBuffer), constantBufferFlags, 0);
    gState->perViewGlobalConstantBuffer = rhiAPI->CreateBuffer(nullptr, sizeof(PerViewGlobalConstantBuffer), constantBufferFlags, 0);

    gState->cameraPos = Vector3(0.0f, 2.0f, 5.0f);
    gState->cameraView = CameraLookAtLH(gState->cameraPos, Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
    gState->cameraProjection = PerspectiveMatrixLH(clientSize.clientWidth, clientSize.clientHeight, 0.1f, 100.0f, PIDIV4);
}


struct ProfileNode
{
    ProfileData* data;
    ProfileNode* childNodes;
    u32 childNodeCount;
};

static bool InsertProfileData(ProfileNode** profileList, u32* profileListCount, ProfileData* data, ILinearAllocator* frameAllocator)
{
    bool inserted = false;
    for (u32 i = 0; i < *profileListCount; ++i)
    {
        ProfileNode* node = *profileList + i;
        uint64_t nodeStartTime = node->data->startNanoseconds;
        uint64_t nodeEndTime = node->data->startNanoseconds + node->data->durationNanoseconds;

        if (data->startNanoseconds >= nodeStartTime && data->startNanoseconds < nodeEndTime)
        {
            if (node->childNodeCount > 0)
            {
                inserted = InsertProfileData(&node->childNodes, &node->childNodeCount, data, frameAllocator);
            }
            if (!inserted)
            {
                ProfileNode* newNode = (ProfileNode*) frameAllocator->Alloc(frameAllocator->instance, sizeof(ProfileNode));
                newNode->data = data;
                if (node->childNodeCount == 0)
                {
                    node->childNodes = newNode;
                }
                node->childNodeCount++;
                inserted = true;
            }
        }
    }
    if (!inserted)
    {
        ProfileNode* newNode = (ProfileNode*) frameAllocator->Alloc(frameAllocator->instance, sizeof(ProfileNode));
        newNode->data = data;
        if (*profileListCount == 0)
        {
            *profileList = newNode;
        }
        *profileListCount = *profileListCount + 1;
        inserted = true;
    }
    return inserted;
}

static void DrawChildNodes(ImguiAPI* imguiAPI, const ProfileNode* profileNodes, u32 profileNodeCount)
{
    for (u32 i = 0; i < profileNodeCount; ++i)
    {
        const ProfileNode* profileNode = profileNodes + i;
        const char* profileName = profileNode->data->name;
        double durationMilliseconds = profileNode->data->durationNanoseconds / 1000000.0;
        if (profileNode->childNodeCount == 0)
        {
            imguiAPI->Text("%s %.3f ms", profileName, durationMilliseconds);
        }
        else
        {
            //ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
            if (imguiAPI->TreeNode(profileName, "%s %.3f ms", profileName, durationMilliseconds))
            {
                DrawChildNodes(imguiAPI, profileNode->childNodes, profileNode->childNodeCount);
                imguiAPI->TreePop();
            }
        }
    }
}

static void DrawPerformanceWindow(ProfilerAPI* profilerAPI, ImguiAPI* imguiAPI, ILinearAllocator* frameAllocator)
{
    const ProfileData* profileDatas = nullptr;
    u32 profileDataCount = profilerAPI->GetProfileDatas(&profileDatas);

    ProfileData* sortedProfileData = (ProfileData*) frameAllocator->Alloc(frameAllocator->instance, profileDataCount * sizeof(ProfileData));
    memcpy(sortedProfileData, profileDatas, profileDataCount * sizeof(ProfileData));
    // Bubble sort
    for (u32 i = 0; i < profileDataCount; ++i)
    {
        for (u32 j = 0; j < (profileDataCount - 1); ++j)
        {
            ProfileData* firstItem = sortedProfileData + j;
            ProfileData* secondItem = sortedProfileData + j + 1;
            if (firstItem->startNanoseconds > secondItem->startNanoseconds)
            {
                SwapMemory(firstItem, secondItem, sizeof(ProfileData));
            }

        }
    }

    // Put profile data into a tree-like structure, so that we can look profile data in a hierarchical view
    ProfileNode* profileTree = nullptr;
    u32 profileNodeCount = 0;
    for (u32 i = 0; i < profileDataCount; ++i) {
        ProfileData* data = sortedProfileData + i;
        InsertProfileData(&profileTree, &profileNodeCount, data, frameAllocator);
    }

    if (imguiAPI->Begin("Performance", nullptr, 0)) {
        DrawChildNodes(imguiAPI, profileTree, profileNodeCount);
    }
    imguiAPI->End();
    profilerAPI->ClearData();

}

bool SystemTick()
{
    //PROFILE_FUNCTION(gProfilerAPI);
    gState->frameAllocator->ClearMemory(gState->frameAllocator->instance);

    RHIAPI* rhiAPI = (RHIAPI*) gAPIRegistry->Get(RHI_API_NAME);
    PlatformAPI* platformAPI = (PlatformAPI*) gAPIRegistry->Get(PLATFORM_API_NAME);
    ImguiAPI* imguiAPI = (ImguiAPI*) gAPIRegistry->Get(IMGUI_API_NAME);
    LogAPI* logAPI = (LogAPI*) gAPIRegistry->Get(LOG_API_NAME);
    EntityAPI* entityAPI = (EntityAPI*) gAPIRegistry->Get(ENTITY_API_NAME);
    WindowAPI* windowAPI = platformAPI->windowAPI;
    InputAPI* inputAPI = platformAPI->inputAPI;

    WindowState windowState = windowAPI->ProcessMessages(gState->mainWindow);
    if (windowState.isCloseRequested)
    {
        return false;
    }

    InputEvent* events = nullptr;
    u32 eventCount = inputAPI->PullEvents(gState->mainWindow, gState->frameAllocator, &events);
    {
        PROFILE_SCOPE(gProfilerAPI, "Parent");
        //LOG_DEBUG(logAPI, "This is a log\n");

        imguiAPI->BeginFrame(gState->mainWindow, events, eventCount);

        {
            PROFILE_SCOPE(gProfilerAPI, "Drawing");
            f32 color[4] = { 0.0f, 1.0f, 1.0f, 1.0f };
            GPURenderTargetView backbuffer = rhiAPI->GetBackbufferRTV();
            rhiAPI->SetRenderTargets(&backbuffer, 1, GPUDepthStencilView());
            rhiAPI->ClearRenderTarget(backbuffer, color);
            rhiAPI->SetViewports(&gState->viewport, 1);
            rhiAPI->SetGraphicsPipelineState(gState->graphicsPSO);
            // Run demo rendering system
            entityAPI->RunSystems(gState->context, gState->frameAllocator);

        }
    }

    if (imguiAPI->Begin("Logs", nullptr, 0))
    {
        //static ImGuiTextFilter textFilter;
        static bool autoScroll = true;
        // Options menu
        if (imguiAPI->BeginPopup("Options", 0))
        {
            imguiAPI->Checkbox("Auto-scroll", &autoScroll);
            imguiAPI->EndPopup();
        }

        // Main window
        if (imguiAPI->Button("Options", ImVec2(0, 0)))
            imguiAPI->OpenPopup("Options", 0);
        imguiAPI->SameLine(0.0f, -1.0f);
        bool clear = imguiAPI->Button("Clear", ImVec2(0, 0));
        imguiAPI->SameLine(0.0f, -1.0f);
        //textFilter.Draw("Filter", -100.0f);

        imguiAPI->Separator();
        imguiAPI->BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        if (clear)
        {
            logAPI->ClearLogs();
        }

        imguiAPI->PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        const LogItem* items;
        u32 logCount = logAPI->GetLogs(&items);
        for (u32 logIndex = 0; logIndex < logCount; ++logIndex)
        {
            const LogItem* item = items + logIndex;
            //if (!textFilter.PassFilter(item->log)) {
                //continue;
            //}

            switch (item->type)
            {
                case LOG_WARNING:
                {
                    imguiAPI->TextColored(ImVec4(1.0f, 0.8f, 0.6f, 1.0f), item->log);
                } break;
                case LOG_ERROR:
                {
                    imguiAPI->TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), item->log);
                } break;
                default:
                {
                    imguiAPI->TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), item->log);
                } break;
            }
        }
        imguiAPI->PopStyleVar(1);

        if (autoScroll && imguiAPI->GetScrollY() >= imguiAPI->GetScrollMaxY())
            imguiAPI->SetScrollHereY(1.0f);

        imguiAPI->EndChild();
    }
    imguiAPI->End();

    DrawPerformanceWindow(gProfilerAPI, imguiAPI, gState->frameAllocator);

    imguiAPI->Render();

    rhiAPI->Present();

    return true;
}

void SystemQuitting()
{

}

extern "C"
{
    MODULE_EXPORT void LoadPlugin(APIRegistry* registry, bool reload)
    {
        gAPIRegistry = registry;
        gProfilerAPI = (ProfilerAPI*) registry->Get(PROFILER_API_NAME);

        SystemAPI systemAPI = {};
        if (reload)
        {
            SystemAPI* api = (SystemAPI*) registry->Get(SYSTEM_API_NAME);
            ASSERT(api, "Can't find API on reload");
            gState = (SystemState*) api->state;
        }

        systemAPI.state = (void*) gState;
        systemAPI.Init = SystemInitialize;
        systemAPI.Update = SystemTick;
        systemAPI.Shutdown = SystemQuitting;

        registry->Set(SYSTEM_API_NAME, &systemAPI, sizeof(SystemAPI));
    }

    MODULE_EXPORT void UnloadPlugin(APIRegistry* registry, bool reload)
    {
        registry->Remove(SYSTEM_API_NAME);
    }
}
