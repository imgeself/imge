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

#include <d3dcompiler.h>

struct SystemState
{
    ILinearAllocator* appAllocator;
    ILinearAllocator* frameAllocator;
    PlatformWindow* mainWindow;

    GPUBuffer vertexBuffer;
    GPUViewport viewport;
    GPUGraphicsPipelineState graphicsPSO;
    DynamicArray<u32> array;
};

static APIRegistry* gAPIRegistry = nullptr;
static SystemState* gState = nullptr;
static ProfilerAPI* gProfilerAPI = nullptr;

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
    gState->array = CreateDynamicArray<u32>(allocatorAPI);

    DynamicArray<const char*> stringArray = CreateDynamicArray<const char*>(allocatorAPI);
    stringArray.Append(AllocateString(applicationAllocator, "Hello"));
    stringArray.Append(AllocateString(applicationAllocator, "My"));
    stringArray.Append(AllocateString(applicationAllocator, "Friend"));
    stringArray.Append(AllocateString(applicationAllocator, "!"));

    for (u32 i = 0; i < stringArray.length; ++i)
    {
        printf("Element %d is %s\n", i, stringArray[i]);
    }
/*
    HashTable<u32, const char*> table = CreateHashTable<u32, const char*>(allocatorAPI, 10);
    table.Add(32, stringArray[0]);
    table.Add(43, stringArray[1]);
    table.Add(54, stringArray[2]);
    table.Add(2, stringArray[3]);
    table.Add(2, stringArray[0]);
    table.Add(12, stringArray[2]);
    table.Add(13, stringArray[2]);
    table.Add(15, stringArray[2]);
    table.Add(1, stringArray[2]);
    //table.Remove(2);
    //table.Remove(1);

    HashTable<const char*, u32> table2 = CreateHashTable<const char*, u32>(allocatorAPI, 10);
    table2.Add(stringArray[0], 10);
    table2.Add(stringArray[1], 20);
    table2.Add(stringArray[2], 30);
    table2.Add(stringArray[3], 40);

    const char* text = nullptr; 
    bool contains = table.Get(55, &text);

    u32 number = 0;
    contains = table2.Get("Mys", &number);
    */
    WindowAPI* windowAPI = platformAPI->windowAPI;

    WindowSize size = { 1280, 720 };
    gState->mainWindow = windowAPI->CreatePlatformWindow(applicationAllocator, "Imge", size);

    rhiAPI->Init(applicationAllocator, gState->mainWindow);

    platformAPI->LoadPlugin(gAPIRegistry, "imgui");
    ImguiAPI* imguiAPI = (ImguiAPI*) gAPIRegistry->Get(IMGUI_API_NAME);
    imguiAPI->Init(applicationAllocator, gState->mainWindow);

    LogAPI* logAPI = (LogAPI*) gAPIRegistry->Get(LOG_API_NAME);
    logAPI->Init(allocatorAPI, gState->appAllocator);

    Texture2DDesc textureDesc = {};
    textureDesc.width = 1280;
    textureDesc.height = 720;
    textureDesc.arraySize = 1;
    textureDesc.mipCount = 1;
    textureDesc.sampleCount = 1;
    textureDesc.format = FORMAT_R8G8B8A8_UNORM;
    textureDesc.flags = GPUResourceFlags::BIND_RENDER_TARGET | GPUResourceFlags::BIND_SHADER_RESOURCE 
        | GPUResourceFlags::BIND_UNORDERED_ACCESS;

    GPUResourceViewDesc resourceView = {};
    resourceView.firstArraySlice = 0;
    resourceView.sliceArrayCount = 1;
    resourceView.firstMip = 0;
    resourceView.mipCount = 1;

    GPUTexture2D texture = rhiAPI->CreateTexture2D(&textureDesc, nullptr, "DEBUG");
    GPURenderTargetView rtv = rhiAPI->CreateRenderTargetView(texture, resourceView, "DEBUG RTV");
    //GPUDepthStencilView dsv = CreateDepthStencilView(texture, resourceView, "DEBUG DSV");
    GPUShaderResourceView srv = rhiAPI->CreateShaderResourceView(texture, resourceView, "DEBUG SRV");
    GPUUnorderedAccessView uav = rhiAPI->CreateUnorderedAccessView(texture, resourceView, "DEBUG UAV");

    rhiAPI->DestroyRenderTargetView(rtv);
    rhiAPI->DestroyShaderResourceView(srv);
    rhiAPI->DestroyUnorderedAccessView(uav);
    rhiAPI->DestroyTexture2D(texture);

    f32 vertexPositions[] =
    {
        -1.0f, -1.0f, 0.0f, 1.0f,
         0.0f,  1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f,
    };

    SubresourceData subresource = {};
    subresource.data = vertexPositions;
    gState->vertexBuffer = rhiAPI->CreateBuffer(&subresource, sizeof(vertexPositions), GPUResourceFlags::BIND_VERTEX_BUFFER | GPUResourceFlags::USAGE_IMMUTABLE, 0);

    const char vertexShader[] = "float4 VSMain(float4 pos : POSITION) : SV_POSITION { return pos; }";

    const char pixelShader[] = R"( 
    float4 PSMain() : SV_TARGET {
        return float4(1.0f, 0.0f, 0.0f, 1.0f);
    })";

    ID3DBlob* shaderBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    D3DCompile(vertexShader, sizeof(vertexShader), NULL, NULL, 0, "VSMain", "vs_5_0", 0, 0, &shaderBlob, &errorBlob);
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
    D3DCompile(pixelShader, sizeof(pixelShader), NULL, NULL, 0, "PSMain", "ps_5_0", 0, 0, &pixelShaderBlob, &pixelErrorBlob);
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
    inputElement.format = FORMAT_R32G32B32A32_FLOAT;
    inputElement.classification = GPUInputClassification::PER_VERTEX_DATA;
    inputElement.inputSlot = 0;
    inputElement.semanticIndex = 0;
    inputElement.semanticName = "POSITION";

    GPUInputLayoutDesc inputLayoutDesc = {};
    inputLayoutDesc.elements = &inputElement;
    inputLayoutDesc.elementCount = 1;

    GPUGraphicsPipelineStateDesc pipelineDesc = {};
    pipelineDesc.vertexShader = vertexByteCode;
    pipelineDesc.pixelShader = pixelByteCode;
    pipelineDesc.inputLayoutDesc = inputLayoutDesc;
    pipelineDesc.primitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    gState->graphicsPSO = rhiAPI->CreateGraphicsPipelineState(pipelineDesc, 0);

    GPUViewport viewport = {};
    viewport.width = 1280;
    viewport.height = 720;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    gState->viewport = viewport;

    windowAPI->ShowPlatformWindow(gState->mainWindow);
}

static bool CompareProfile(const ProfileData& left, const ProfileData& right)
{
    return (left.startNanoseconds < right.startNanoseconds);
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
        imguiAPI->End();
    }
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
            PROFILE_SCOPE(gProfilerAPI, "Array operations");
            DynamicArray<u32>& array = gState->array;
            for (u32 i = 0; i < 1025; i++)
            {
                array.Append(i);
            }

            for (u32 i = 0; i < 511; i++)
            {
                array.PopBack();
            }

            array.Insert(55, 2);
            array.RemoveAt(3);
            array.PopFirst();
            array.Clear();
        }

        {
            PROFILE_SCOPE(gProfilerAPI, "Drawing");
            f32 color[4] = { 0.0f, 1.0f, 1.0f, 1.0f };
            GPURenderTargetView backbuffer = rhiAPI->GetBackbufferRTV();
            rhiAPI->SetRenderTargets(&backbuffer, 1, GPUDepthStencilView());
            rhiAPI->ClearRenderTarget(backbuffer, color);
            u32 stride = 4 * 4;
            u32 offset = 0;
            rhiAPI->SetVertexBuffers(&gState->vertexBuffer, 0, 1, &stride, &offset);
            rhiAPI->SetViewports(&gState->viewport, 1);
            rhiAPI->SetGraphicsPipelineState(gState->graphicsPSO);
            rhiAPI->Draw(3, 0);
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
        imguiAPI->End();
    }

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
