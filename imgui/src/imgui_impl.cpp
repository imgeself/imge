#include "pch.h"
#include "imgui.h"
#include "imgui_api.h"
#include "RHI.h"
#include "ApiRegistry.h"
#include "Allocator.h"
#include "Platform.h"

#include <d3dcompiler.h>

static APIRegistry* gAPIRegistry = nullptr;
static PlatformAPI* gPlatformAPI = nullptr;
static RHIAPI* gRHIAPI = nullptr;

struct ImguiState
{
    GPUBuffer                vertexBuffer;
    GPUBuffer                indexBuffer;
    GPUBuffer                vertexConstantBuffer;
    GPUSamplerState          fontSampler;
    GPUShaderResourceView    fontTextureSRV;
    GPUGraphicsPipelineState graphicsPipelineState;
    u32                      vertexBufferSize = 5000;
    u32                      indexBufferSize = 10000;
    ImGuiContext*            imGuiContext;
};

static ImguiState* gState = nullptr;

struct VERTEX_CONSTANT_BUFFER
{
    f32 mvp[4][4];
};


void ImGui_Impl_InvalidateDeviceObjects(RHIAPI* rhiAPI)
{
    if (gState->fontSampler.handle != INVALID_HANDLE) { rhiAPI->DestroySamplerState(gState->fontSampler); }
    if (gState->fontTextureSRV.handle != INVALID_HANDLE) { rhiAPI->DestroyShaderResourceView(gState->fontTextureSRV); }
    if (gState->indexBuffer.handle != INVALID_HANDLE) { rhiAPI->DestroyBuffer(gState->indexBuffer); }
    if (gState->vertexBuffer.handle != INVALID_HANDLE) { rhiAPI->DestroyBuffer(gState->vertexBuffer); }
    if (gState->graphicsPipelineState.handle != INVALID_HANDLE) { rhiAPI->DestroyGraphicsPipelineState(gState->graphicsPipelineState); }
    if (gState->vertexConstantBuffer.handle != INVALID_HANDLE) { rhiAPI->DestroyBuffer(gState->vertexConstantBuffer); }
}

static void ImGui_Impl_SetupRenderState(ImDrawData* draw_data, RHIAPI* rhiAPI)
{
    // Setup viewport
    GPUViewport vp = {};
    vp.width = draw_data->DisplaySize.x;
    vp.height = draw_data->DisplaySize.y;
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;
    vp.topLeftX = vp.topLeftY = 0;
    rhiAPI->SetViewports(&vp, 1);

    // Setup shader and vertex buffers
    unsigned int stride = sizeof(ImDrawVert);
    unsigned int offset = 0;
    rhiAPI->SetGraphicsPipelineState(gState->graphicsPipelineState);
    rhiAPI->SetVertexBuffers(&gState->vertexBuffer, 0, 1, &stride, &offset);
    rhiAPI->SetIndexBuffer(gState->indexBuffer, sizeof(ImDrawIdx), 0);
    rhiAPI->SetConstantBuffersVS(0, &gState->vertexConstantBuffer, 1);
    rhiAPI->SetSamplerStatesPS(0, &gState->fontSampler, 1);
}

// Render function
// (this used to be set in io.RenderDrawListsFn and called by ImGui::Render(), but you can now call this directly from your main loop)
void ImGui_Impl_RenderDrawData(ImDrawData* draw_data, RHIAPI* rhiAPI)
{
    // Avoid rendering when minimized
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
        return;

    // Create and grow vertex/index buffers if needed
    if (gState->vertexBuffer.handle == INVALID_HANDLE || gState->vertexBufferSize < draw_data->TotalVtxCount)
    {
        if (gState->vertexBuffer.handle != INVALID_HANDLE) { rhiAPI->DestroyBuffer(gState->vertexBuffer); }
        gState->vertexBufferSize = draw_data->TotalVtxCount + 5000;
        gState->vertexBuffer = rhiAPI->CreateBuffer(nullptr, gState->vertexBufferSize * sizeof(ImDrawVert),
                                     GPUResourceFlags::CPU_ACCESS_WRITE | GPUResourceFlags::BIND_VERTEX_BUFFER | GPUResourceFlags::USAGE_DYNAMIC,
                                     "ImguiVertexBuffer");
        if (gState->vertexBuffer.handle == INVALID_HANDLE)
            return;
    }
    if (gState->indexBuffer.handle == INVALID_HANDLE || gState->indexBufferSize < draw_data->TotalIdxCount)
    {
        if (gState->indexBuffer.handle != INVALID_HANDLE) { rhiAPI->DestroyBuffer(gState->indexBuffer); }
        gState->indexBufferSize = draw_data->TotalIdxCount + 10000;
        gState->indexBuffer = rhiAPI->CreateBuffer(nullptr, gState->indexBufferSize * sizeof(ImDrawIdx),
                                     GPUResourceFlags::CPU_ACCESS_WRITE | GPUResourceFlags::BIND_INDEX_BUFFER | GPUResourceFlags::USAGE_DYNAMIC,
                                     "ImguiIndexBuffer");
        if (gState->indexBuffer.handle == INVALID_HANDLE)
            return;
    }

    // Upload vertex/index data into a single contiguous GPU buffer
    void* vtx_resource = rhiAPI->MapBuffer(gState->vertexBuffer);
    void* idx_resource = rhiAPI->MapBuffer(gState->indexBuffer);
    ImDrawVert* vtx_dst = (ImDrawVert*)vtx_resource;
    ImDrawIdx* idx_dst = (ImDrawIdx*)idx_resource;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtx_dst += cmd_list->VtxBuffer.Size;
        idx_dst += cmd_list->IdxBuffer.Size;
    }
    rhiAPI->UnmapBuffer(gState->vertexBuffer);
    rhiAPI->UnmapBuffer(gState->indexBuffer);

    // Setup orthographic projection matrix into our constant buffer
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
    {
        void* mapped_resource = rhiAPI->MapBuffer(gState->vertexConstantBuffer);
        VERTEX_CONSTANT_BUFFER* constant_buffer = (VERTEX_CONSTANT_BUFFER*)mapped_resource;
        float L = draw_data->DisplayPos.x;
        float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
        float T = draw_data->DisplayPos.y;
        float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
        float mvp[4][4] =
        {
            { 2.0f/(R-L),   0.0f,           0.0f,       0.0f },
            { 0.0f,         2.0f/(T-B),     0.0f,       0.0f },
            { 0.0f,         0.0f,           0.5f,       0.0f },
            { (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f },
        };
        memcpy(&constant_buffer->mvp, mvp, sizeof(mvp));
        rhiAPI->UnmapBuffer(gState->vertexConstantBuffer);
    }

    // Setup desired DX state
    ImGui_Impl_SetupRenderState(draw_data, rhiAPI);

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int global_idx_offset = 0;
    int global_vtx_offset = 0;
    ImVec2 clip_off = draw_data->DisplayPos;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != NULL)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_Impl_SetupRenderState(draw_data, rhiAPI);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Apply scissor/clipping rectangle
                const GPURect r = { (u32)(pcmd->ClipRect.x - clip_off.x), (u32)(pcmd->ClipRect.y - clip_off.y), (u32)(pcmd->ClipRect.z - clip_off.x), (u32)(pcmd->ClipRect.w - clip_off.y) };
                rhiAPI->SetScissorRects(&r, 1);

                // Bind texture, Draw
                GPUShaderResourceView texture_srv = GPUShaderResourceView{ *((Handle*) &pcmd->TextureId) };
                rhiAPI->SetShaderResourcesPS(0, &texture_srv, 1);
                rhiAPI->DrawIndexed(pcmd->ElemCount, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset);
            }
        }
        global_idx_offset += cmd_list->IdxBuffer.Size;
        global_vtx_offset += cmd_list->VtxBuffer.Size;
    }
}

static void ImGui_Impl_CreateFontsTexture(RHIAPI* rhiAPI)
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Upload texture to graphics system
    {
        Texture2DDesc desc = {};
        desc.width = width;
        desc.height = height;
        desc.mipCount = 1;
        desc.arraySize = 1;
        desc.format = FORMAT_R8G8B8A8_UNORM;
        desc.sampleCount = 1;
        desc.flags = GPUResourceFlags::BIND_SHADER_RESOURCE;

        SubresourceData subResource;
        subResource.data = pixels;
        subResource.memPitch = desc.width * 4;
        subResource.memSlicePitch = 0;
        GPUTexture2D texture = rhiAPI->CreateTexture2D(&desc, &subResource, 0);

        // Create texture view
        GPUResourceViewDesc srvDesc = {};
        srvDesc.mipCount = desc.mipCount;
        srvDesc.sliceArrayCount = desc.arraySize;
        gState->fontTextureSRV = rhiAPI->CreateShaderResourceView(texture, srvDesc, 0);
        rhiAPI->DestroyTexture2D(texture);
    }

    // Store our identifier
    io.Fonts->TexID = (ImTextureID) *((u32*) &gState->fontTextureSRV.handle);

    // Create texture sampler
    {
        SamplerStateDesc desc = {};
        desc.filterMode = TextureFilterMode_MIN_MAG_MIP_LINEAR;
        desc.addressModeU = TextureAddressMode_WRAP;
        desc.addressModeV = TextureAddressMode_WRAP;
        desc.addressModeW = TextureAddressMode_WRAP;
        //desc.comparisonFunction = COMPARISON_ALWAYS;
        gState->fontSampler = rhiAPI->CreateSamplerState(&desc, 0);
    }
}

bool ImGui_Impl_CreateDeviceObjects(RHIAPI* rhiAPI)
{
    if (gState->fontSampler.handle.index == INVALID_HANDLE_INDEX)
        ImGui_Impl_InvalidateDeviceObjects(rhiAPI);

    // Create the vertex shader
    static const char* vertexShader =
        "cbuffer vertexBuffer : register(b0) \
        {\
        float4x4 ProjectionMatrix; \
        };\
        struct VS_INPUT\
        {\
        float2 pos : POSITION;\
        float4 col : COLOR0;\
        float2 uv  : TEXCOORD0;\
        };\
        \
        struct PS_INPUT\
        {\
        float4 pos : SV_POSITION;\
        float4 col : COLOR0;\
        float2 uv  : TEXCOORD0;\
        };\
        \
        PS_INPUT main(VS_INPUT input)\
        {\
        PS_INPUT output;\
        output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\
        output.col = input.col;\
        output.uv  = input.uv;\
        return output;\
        }";

    ID3DBlob* vertexShaderBlob = nullptr;
    D3DCompile(vertexShader, strlen(vertexShader), NULL, NULL, NULL, "main", "vs_5_0", 0, 0, &vertexShaderBlob, NULL);
    if (vertexShaderBlob == nullptr) // NB: Pass ID3D10Blob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
        return false;

    // Create the input layout
    GPUVertexInputElement local_layout[] =
    {
        { "POSITION", 0, FORMAT_R32G32_FLOAT,   0, PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, FORMAT_R32G32_FLOAT,   0, PER_VERTEX_DATA, 0 },
        { "COLOR",    0, FORMAT_R8G8B8A8_UNORM, 0, PER_VERTEX_DATA, 0 },
    };
    GPUInputLayoutDesc layoutDesc = {};
    layoutDesc.elements = local_layout;
    layoutDesc.elementCount = sizeof(local_layout) / sizeof(local_layout[0]);

    // Create the constant buffer
    {
        gState->vertexConstantBuffer = rhiAPI->CreateBuffer(nullptr, sizeof(VERTEX_CONSTANT_BUFFER),
                                                        GPUResourceFlags::BIND_CONSTANT_BUFFER | GPUResourceFlags::USAGE_DYNAMIC | GPUResourceFlags::CPU_ACCESS_WRITE,
                                                        "ImguConstantBuffer");
    }

    // Create the pixel shader
    static const char* pixelShader =
        "struct PS_INPUT\
        {\
        float4 pos : SV_POSITION;\
        float4 col : COLOR0;\
        float2 uv  : TEXCOORD0;\
        };\
        sampler sampler0;\
        Texture2D texture0;\
        \
        float4 main(PS_INPUT input) : SV_Target\
        {\
        float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \
        return out_col; \
        }";

    ID3DBlob* pixelShaderBlob = nullptr;
    D3DCompile(pixelShader, strlen(pixelShader), NULL, NULL, NULL, "main", "ps_5_0", 0, 0, &pixelShaderBlob, NULL);
    if (pixelShaderBlob == NULL)  // NB: Pass ID3D10Blob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
        return false;

    // Create the blending setup
    GPUBlendStateDesc blendStateDesc = {};
    blendStateDesc.alphaToCoverageEnable = false;
    blendStateDesc.renderTarget[0].blendEnable = true;
    blendStateDesc.renderTarget[0].srcBlend = BLEND_SRC_ALPHA;
    blendStateDesc.renderTarget[0].destBlend = BLEND_INV_SRC_ALPHA;
    blendStateDesc.renderTarget[0].blendOp = BLEND_OP_ADD;
    blendStateDesc.renderTarget[0].srcBlendAlpha = BLEND_INV_SRC_ALPHA;
    blendStateDesc.renderTarget[0].destBlendAlpha = BLEND_ZERO;
    blendStateDesc.renderTarget[0].blendOpAlpha = BLEND_OP_ADD;
    blendStateDesc.renderTarget[0].renderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;

    // Create the rasterizer state
    GPURasterizerStateDesc rasterizerStateDesc = {};
    rasterizerStateDesc.fillMode = FILL_SOLID;
    rasterizerStateDesc.cullMode = CULL_NONE;
    rasterizerStateDesc.scissorEnable = true;
    rasterizerStateDesc.depthClipEnable = true;

    // Create depth-stencil State
    GPUDepthStencilStateDesc depthStencilStateDesc = {};
    depthStencilStateDesc.depthEnable = false;
    depthStencilStateDesc.depthWriteMask = DEPTH_WRITE_MASK_ALL;
    depthStencilStateDesc.depthFunc = COMPARISON_ALWAYS;
    depthStencilStateDesc.stencilEnable = false;
    depthStencilStateDesc.frontFace.stencilFailOp = depthStencilStateDesc.frontFace.stencilDepthFailOp = depthStencilStateDesc.frontFace.stencilPassOp = STENCIL_OP_KEEP;
    depthStencilStateDesc.frontFace.stencilFunc = COMPARISON_ALWAYS;
    depthStencilStateDesc.backFace = depthStencilStateDesc.frontFace;

    GPUGraphicsPipelineStateDesc pipelineStateDesc = {};
    pipelineStateDesc.blendStateDesc = blendStateDesc;
    pipelineStateDesc.depthStencilStateDesc = depthStencilStateDesc;
    pipelineStateDesc.inputLayoutDesc = layoutDesc;
    pipelineStateDesc.primitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    pipelineStateDesc.rasterizerStateDesc = rasterizerStateDesc;
    pipelineStateDesc.vertexShader = GPUShaderBytecode{ (u8*) vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
    pipelineStateDesc.pixelShader = GPUShaderBytecode{ (u8*) pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };

    gState->graphicsPipelineState = rhiAPI->CreateGraphicsPipelineState(pipelineStateDesc, 0);

    vertexShaderBlob->Release();
    pixelShaderBlob->Release();

    ImGui_Impl_CreateFontsTexture(rhiAPI);

    return true;
}

bool ImGui_Impl_Init(RHIAPI* rhiAPI)
{
    // Setup back-end capabilities flags
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_imge";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

    return true;
}

void ImGui_Impl_Shutdown(RHIAPI* rhiAPI)
{
    ImGui_Impl_InvalidateDeviceObjects(rhiAPI);
}

void ImGui_Impl_NewFrame(RHIAPI* rhiAPI)
{
    if (gState->fontSampler.handle == INVALID_HANDLE)
        ImGui_Impl_CreateDeviceObjects(rhiAPI);
}


void ImguiInit(ILinearAllocator* applicationAllocator, PlatformWindow* window)
{
    IMGUI_CHECKVERSION();
    ImGuiContext* imguiContext = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.BackendPlatformName = "imgui_impl";
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImguiState defaultState = {};
    defaultState.imGuiContext = imguiContext;
    gState = (ImguiState*) applicationAllocator->Alloc(applicationAllocator->instance, sizeof(ImguiState));
    memcpy(gState, &defaultState, sizeof(ImguiState));
    // Update state pointer in api
    ImguiAPI* api = (ImguiAPI*) gAPIRegistry->Get(IMGUI_API_NAME);
    api->state = (void*) gState;

    WindowAPI* windowAPI  = gPlatformAPI->windowAPI;
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    main_viewport->PlatformHandle = main_viewport->PlatformHandleRaw = windowAPI->GetPlatformWindowHandle(window);

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array that we will update during the application lifetime.
    io.KeyMap[ImGuiKey_Tab] = KEY_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = KEY_HOME;
    io.KeyMap[ImGuiKey_End] = KEY_END;
    io.KeyMap[ImGuiKey_Insert] = KEY_INSERT;
    io.KeyMap[ImGuiKey_Delete] = KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Space] = KEY_SPACE;
    io.KeyMap[ImGuiKey_Enter] = KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = KEY_ESCAPE;
    io.KeyMap[ImGuiKey_KeyPadEnter] = KEY_ENTER;
    io.KeyMap[ImGuiKey_A] = KEY_A;
    io.KeyMap[ImGuiKey_C] = KEY_C;
    io.KeyMap[ImGuiKey_V] = KEY_V;
    io.KeyMap[ImGuiKey_X] = KEY_X;
    io.KeyMap[ImGuiKey_Y] = KEY_Y;
    io.KeyMap[ImGuiKey_Z] = KEY_Z;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    ImGui_Impl_Init(gRHIAPI);
}

void ImguiShutdown()
{
    ImGui_Impl_Shutdown(gRHIAPI);
    ImGui::DestroyContext();
}

void ImguiBeginFrame(PlatformWindow* window, InputEvent* events, u32 eventSize)
{
    // Start the Dear ImGui frame
    ImGui_Impl_NewFrame(gRHIAPI);

    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

    // Setup display size (every frame to accommodate for window resizing)
    WindowAPI* windowAPI = gPlatformAPI->windowAPI;
    InputAPI* inputAPI = gPlatformAPI->inputAPI;
    WindowSize clientSize = windowAPI->GetPlatformWindowClientSize(window);
    io.DisplaySize = ImVec2(clientSize.clientWidth, clientSize.clientHeight);

    InputState* inputState = inputAPI->GetState(window);

    //io.DeltaTime = deltaTime;

    for (u32 eventIndex = 0; eventIndex < eventSize; ++eventIndex)
    {
        InputEvent* event = events + eventIndex;
        if (event->eventType == EVENT_CHARACTER_TYPED)
        {
            u32 keyData = event->data.data0;
            io.AddInputCharacter(keyData);
        }
    }

    // Read keyboard modifiers inputs
    io.KeyCtrl = inputState->keyDown[KEY_LEFT_CONTROL] || inputState->keyDown[KEY_RIGHT_CONTROL];
    io.KeyShift = inputState->keyDown[KEY_LEFT_SHIFT] || inputState->keyDown[KEY_RIGHT_SHIFT];
    io.KeyAlt = inputState->keyDown[KEY_LEFT_ALT] || inputState->keyDown[KEY_RIGHT_ALT];
    io.KeySuper = inputState->keyDown[KEY_LEFT_SYSTEM] || inputState->keyDown[KEY_RIGHT_SYSTEM];

    memcpy(io.KeysDown, inputState->keyDown, 512 * sizeof(bool));
    memcpy(io.MouseDown, inputState->mouseKeyDown, 4 * sizeof(bool));

    // Update OS mouse position
    io.MousePos = ImVec2(inputState->mousePosClient.x, inputState->mousePosClient.y);

    ImGui::NewFrame();
}

void ImguiRender()
{
    // Rendering
    ImGui::Render();
    ImGui_Impl_RenderDrawData(ImGui::GetDrawData(), gRHIAPI);
}

ImGuiContext* GetContext()
{
    return ImGui::GetCurrentContext();
}

extern "C"
{

    MODULE_EXPORT void LoadPlugin(APIRegistry* registry, bool reload)
    {
        gAPIRegistry = registry;
        gPlatformAPI = (PlatformAPI*) registry->Get(PLATFORM_API_NAME);
        gRHIAPI = (RHIAPI*) registry->Get(RHI_API_NAME);

        ImguiAPI imguiAPI = {};
        if (reload)
        {
            ImguiAPI* api = (ImguiAPI*) registry->Get(IMGUI_API_NAME);
            ASSERT(api, "Can't find API on reload");
            gState = (ImguiState*) api->state;
            ImGui::SetCurrentContext(gState->imGuiContext);
        }

        imguiAPI.state = (void*) gState;
        imguiAPI.Init = ImguiInit;
        imguiAPI.Shutdown = ImguiShutdown;
        imguiAPI.BeginFrame = ImguiBeginFrame;
        imguiAPI.Render = ImguiRender;

        imguiAPI.Begin = ImGui::Begin;
        imguiAPI.End = ImGui::End;
        imguiAPI.Button = ImGui::Button;
        imguiAPI.Checkbox = ImGui::Checkbox;
        imguiAPI.Combo = ImGui::Combo;

        imguiAPI.BeginPopup = ImGui::BeginPopup;
        imguiAPI.EndPopup = ImGui::EndPopup;
        imguiAPI.OpenPopup = ImGui::OpenPopup;
        imguiAPI.SameLine = ImGui::SameLine;
        imguiAPI.Separator = ImGui::Separator;
        imguiAPI.BeginChild = ImGui::BeginChild;
        imguiAPI.EndChild = ImGui::EndChild;
        imguiAPI.PushStyleVar = ImGui::PushStyleVar;
        imguiAPI.PopStyleVar = ImGui::PopStyleVar;
        imguiAPI.TextUnformatted = ImGui::TextUnformatted;
        imguiAPI.Text = ImGui::Text;
        imguiAPI.TextColored = ImGui::TextColored;
        imguiAPI.GetScrollX = ImGui::GetScrollX; 
        imguiAPI.GetScrollY = ImGui::GetScrollY;
        imguiAPI.GetScrollMaxX = ImGui::GetScrollMaxX;
        imguiAPI.GetScrollMaxY = ImGui::GetScrollMaxY;
        imguiAPI.SetScrollX = ImGui::SetScrollX; 
        imguiAPI.SetScrollY = ImGui::SetScrollY; 
        imguiAPI.SetScrollHereX = ImGui::SetScrollHereX; 
        imguiAPI.SetScrollHereY = ImGui::SetScrollHereY; 
        imguiAPI.TreeNode = ImGui::TreeNode;
        imguiAPI.TreePop = ImGui::TreePop;

        registry->Set(IMGUI_API_NAME, &imguiAPI, sizeof(RHIAPI));
    }

    MODULE_EXPORT void UnloadPlugin(APIRegistry* registry, bool reload)
    {
        registry->Remove(IMGUI_API_NAME);
    }
}
