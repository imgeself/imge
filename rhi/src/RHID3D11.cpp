#include "pch.h"
#include "RHI.h"
#include "Platform.h"
#include "ApiRegistry.h"
#include "Allocator.h"

#include <d3d11_1.h>

struct D3D11State
{
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* deviceContext = nullptr;
    IDXGISwapChain* swapChain = nullptr;
    ID3D11Debug* debug = nullptr;
    ID3DUserDefinedAnnotation* userDefinedAnnotation = nullptr;

    GPURenderTargetView backbufferRTV;

    HandlePool texture2DPool;
    HandlePool bufferPool;
    HandlePool samplerPool;
    HandlePool rtvPool;
    HandlePool dsvPool;
    HandlePool srvPool;
    HandlePool uavPool;
    HandlePool queryPool;
    HandlePool graphicsPSOPool;
    HandlePool computePSOPool;
};

D3D11State* gState = nullptr;

struct BufferD3D11
{
    ID3D11Buffer* buffer;
};

struct Texture2DD3D11
{
    ID3D11Texture2D* texture2D;
};

struct RenderTargetViewD3D11
{
    ID3D11RenderTargetView* rtv;
};

struct DepthStencilViewD3D11
{
    ID3D11DepthStencilView* dsv;
};

struct ShaderResourceViewD3D11
{
    ID3D11ShaderResourceView* srv;
};

struct UnorderedAccessViewD3D11
{
    ID3D11UnorderedAccessView* uav;
};

struct SamplerStateD3D11
{
    ID3D11SamplerState* samplerState;
};

struct QueryD3D11
{
    ID3D11Query* query;
};

struct GraphicsPipelineStateD3D11
{
    ID3D11VertexShader* vertexShader;
    ID3D11PixelShader* pixelShader;
    ID3D11BlendState* blendState;
    ID3D11RasterizerState* rasterizerState;
    ID3D11DepthStencilState* depthStencilState;
    ID3D11InputLayout* inputLayout;

    D3D11_PRIMITIVE_TOPOLOGY primitiveTopology;
    u32 sampleMask;
};

struct ComputePipelineStateD3D11
{
    ID3D11ComputeShader* computeShader;
};

static APIRegistry* gAPIRegistry = nullptr;

void GpuInit(ILinearAllocator* allocator, PlatformWindow* window)
{
    PlatformAPI* platformAPI = (PlatformAPI*) gAPIRegistry->Get(PLATFORM_API_NAME);
    WindowAPI* windowAPI  = platformAPI->windowAPI;

    gState = (D3D11State*) allocator->Alloc(allocator->instance, sizeof(D3D11State));
    // Update state pointer in api
    RHIAPI* api = (RHIAPI*) gAPIRegistry->Get(RHI_API_NAME);
    api->state = (void*) gState;
    // NOTE: We don't specify refresh rate at the moment.
    // Both numerator and denominator are 0;
    DXGI_RATIONAL refreshRate = {};

    WindowSize clientRect = windowAPI->GetPlatformWindowClientSize(window);
    const UINT width = clientRect.clientWidth;
    const UINT height = clientRect.clientHeight;

    DXGI_MODE_DESC modeDescriptor = {};
    modeDescriptor.Width = width;
    modeDescriptor.Height = height;
    modeDescriptor.RefreshRate = refreshRate;
    modeDescriptor.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    modeDescriptor.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    modeDescriptor.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    // Antialiasing descriptor
    DXGI_SAMPLE_DESC sampleDescriptor = {};
    sampleDescriptor.Count = 1;
    sampleDescriptor.Quality = 0;

    DXGI_SWAP_CHAIN_DESC swapChainDescriptor = {};
    swapChainDescriptor.BufferDesc = modeDescriptor;
    swapChainDescriptor.SampleDesc = sampleDescriptor;
    swapChainDescriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDescriptor.BufferCount = 2;
    swapChainDescriptor.OutputWindow = (HWND) windowAPI->GetPlatformWindowHandle(window);
    swapChainDescriptor.Windowed = true;
    swapChainDescriptor.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapChainDescriptor.Flags = 0;

    const D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
    };

    UINT createDeviceFlags = 0;
#ifdef DEBUG_GPU_DEVICE
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL deviceFeatureLevel;
    HRESULT result = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        (D3D11_CREATE_DEVICE_FLAG)createDeviceFlags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &swapChainDescriptor,
        &gState->swapChain,
        &gState->device,
        &deviceFeatureLevel,
        &gState->deviceContext
    );

    ASSERT(SUCCEEDED(result), "Device context creation failed");
    ASSERT(featureLevels[0] == deviceFeatureLevel, "Hardware doesn't support DX11");

#ifdef DEBUG_GPU_DEVICE
    gState->device->QueryInterface(__uuidof(ID3D11Debug), (void**)(&gState->debug));
#endif

    gState->deviceContext->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void**)(&gState->userDefinedAnnotation));

    ID3D11Texture2D* d3dBackbuffer = nullptr;
    result = gState->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&d3dBackbuffer);
    ASSERT(SUCCEEDED(result), "Couldn't get the backbuffer");

    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = modeDescriptor.Format;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;

    ID3D11RenderTargetView* d3dBackbufferRTV = nullptr;
    result = gState->device->CreateRenderTargetView(d3dBackbuffer, &rtvDesc, &d3dBackbufferRTV);
    ASSERT(SUCCEEDED(result), "Couldn't create backbuffer RTV");
    d3dBackbuffer->Release();

    gState->texture2DPool = InitHandlePool(allocator, 256, sizeof(Texture2DD3D11));
    gState->bufferPool = InitHandlePool(allocator, 256, sizeof(BufferD3D11));
    gState->samplerPool = InitHandlePool(allocator, 64, sizeof(SamplerStateD3D11));
    gState->rtvPool = InitHandlePool(allocator, 256, sizeof(RenderTargetViewD3D11));
    gState->dsvPool = InitHandlePool(allocator, 256, sizeof(DepthStencilViewD3D11));
    gState->srvPool = InitHandlePool(allocator, 256, sizeof(ShaderResourceViewD3D11));
    gState->uavPool = InitHandlePool(allocator, 256, sizeof(UnorderedAccessViewD3D11));
    gState->queryPool = InitHandlePool(allocator, 128, sizeof(QueryD3D11));
    gState->graphicsPSOPool = InitHandlePool(allocator, 256, sizeof(GraphicsPipelineStateD3D11));
    gState->computePSOPool = InitHandlePool(allocator, 256, sizeof(ComputePipelineStateD3D11));

    RenderTargetViewD3D11 rtvD3D11Object = {};
    rtvD3D11Object.rtv = d3dBackbufferRTV;

    Handle handle = ObtainNewHandleFromPool(&gState->rtvPool);
    RenderTargetViewD3D11* data = (RenderTargetViewD3D11*) AccessDataFromHandlePool(&gState->rtvPool, handle);
    *data = rtvD3D11Object;

    gState->backbufferRTV.handle = handle;
}

GPURenderTargetView GpuGetBackbufferRTV()
{
    return gState->backbufferRTV;
}


static inline UINT GetMiscFlagsFromResourceFlags(uint32_t resourceFlags)
{
    return UINT(
        ((resourceFlags & GPUResourceFlags::MISC_TEXTURE_CUBE) ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0) |
        ((resourceFlags & GPUResourceFlags::MISC_GENERATE_MIPS) ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0)
    );
}

static inline D3D11_BIND_FLAG GetBindFlagsFromResourceFlags(uint32_t resourceFlags)
{
    return D3D11_BIND_FLAG(
        ((resourceFlags & GPUResourceFlags::BIND_VERTEX_BUFFER) ? D3D11_BIND_VERTEX_BUFFER : 0) |
        ((resourceFlags & GPUResourceFlags::BIND_INDEX_BUFFER) ? D3D11_BIND_INDEX_BUFFER : 0) |
        ((resourceFlags & GPUResourceFlags::BIND_CONSTANT_BUFFER) ? D3D11_BIND_CONSTANT_BUFFER : 0) |
        ((resourceFlags & GPUResourceFlags::BIND_DEPTH_STENCIL) ? D3D11_BIND_DEPTH_STENCIL : 0) |
        ((resourceFlags & GPUResourceFlags::BIND_RENDER_TARGET) ? D3D11_BIND_RENDER_TARGET : 0) |
        ((resourceFlags & GPUResourceFlags::BIND_UNORDERED_ACCESS) ? D3D11_BIND_UNORDERED_ACCESS : 0) |
        ((resourceFlags & GPUResourceFlags::BIND_SHADER_RESOURCE) ? D3D11_BIND_SHADER_RESOURCE : 0)
    );
}

static inline D3D11_USAGE GetUsageFromResourceFlags(uint32_t resourceFlags)
{
    return D3D11_USAGE(
        ((resourceFlags & GPUResourceFlags::USAGE_STAGING) ? D3D11_USAGE_STAGING :
            ((resourceFlags & GPUResourceFlags::USAGE_DYNAMIC) ? D3D11_USAGE_DYNAMIC :
                ((resourceFlags & GPUResourceFlags::USAGE_IMMUTABLE) ? D3D11_USAGE_IMMUTABLE :
                    D3D11_USAGE_DEFAULT)))
    );
}

static inline D3D11_CPU_ACCESS_FLAG GetCPUAccessFlagFromResourceFlags(uint32_t resourceFlags)
{
    return D3D11_CPU_ACCESS_FLAG(
        ((resourceFlags & GPUResourceFlags::CPU_ACCESS_READ) ? D3D11_CPU_ACCESS_READ : 0) |
        ((resourceFlags & GPUResourceFlags::CPU_ACCESS_WRITE) ? D3D11_CPU_ACCESS_WRITE : 0)
    );
}

static inline D3D11_TEXTURE_ADDRESS_MODE GetD3DAddressModeFromAddressMode(SamplerStateTextureAddressMode mode) {
    D3D11_TEXTURE_ADDRESS_MODE result = {};
    switch (mode) 
    {
        case TextureAddressMode_NONE:
            break;
        case TextureAddressMode_WRAP:
            result = D3D11_TEXTURE_ADDRESS_WRAP;
            break;
        case TextureAddressMode_MIRROR:
            result = D3D11_TEXTURE_ADDRESS_MIRROR;
            break;
        case TextureAddressMode_CLAMP:
            result = D3D11_TEXTURE_ADDRESS_CLAMP;
            break;
        case TextureAddressMode_BORDER:
            result = D3D11_TEXTURE_ADDRESS_BORDER;
            break;
        default:
            ASSERT(false, "Unhandled sampler state texture address mode!");
            break;
    }

    return result;
}

static inline D3D11_FILTER GetD3DFilterFromFilterMode(SamplerStateTextureFilterMode mode, GPUComparisonFunc cmpFunc) {
    // TODO; Handle mode filters
    D3D11_FILTER result;
    bool isComparison = cmpFunc != GPUComparisonFunc::COMPARISON_NEVER;
    switch (mode)
    {
        case TextureFilterMode_MIN_MAG_MIP_POINT:
            result = isComparison ? D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT : D3D11_FILTER_MIN_MAG_MIP_POINT;
            break;
        case TextureFilterMode_MIN_MAG_MIP_LINEAR:
            result = isComparison ? D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            break;
        case TextureFilterMode_MIN_MAG_LINEAR_MIP_POINT:
            result = isComparison ? D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT : D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
            break;
        case TextureFilterMode_ANISOTROPIC:
            result = isComparison ? D3D11_FILTER_COMPARISON_ANISOTROPIC : D3D11_FILTER_ANISOTROPIC;
            break;
        default:
            ASSERT(false, "Unhandled sampler state texture filter mode!");
            break;
    }

    return result;
}

static inline D3D11_BLEND GetD3D11Blend(GPUBlend blend) {
    switch (blend)
    {
        case BLEND_ZERO:
            return D3D11_BLEND_ZERO;
        case BLEND_ONE:
            return D3D11_BLEND_ONE;
        case BLEND_SRC_COLOR:
            return D3D11_BLEND_SRC_COLOR;
        case BLEND_INV_SRC_COLOR:
            return D3D11_BLEND_INV_SRC_COLOR;
        case BLEND_SRC_ALPHA:
            return D3D11_BLEND_SRC_ALPHA;
        case BLEND_INV_SRC_ALPHA:
            return D3D11_BLEND_INV_SRC_ALPHA;
        case BLEND_DEST_ALPHA:
            return D3D11_BLEND_DEST_ALPHA;
        case BLEND_INV_DEST_ALPHA:
            return D3D11_BLEND_INV_DEST_ALPHA;
        case BLEND_DEST_COLOR:
            return D3D11_BLEND_DEST_COLOR;
        case BLEND_INV_DEST_COLOR:
            return D3D11_BLEND_DEST_COLOR;
        case BLEND_SRC_ALPHA_SAT:
            return D3D11_BLEND_SRC_ALPHA_SAT;
        case BLEND_BLEND_FACTOR:
            return D3D11_BLEND_BLEND_FACTOR;
        case BLEND_INV_BLEND_FACTOR:
            return D3D11_BLEND_INV_BLEND_FACTOR;
        default:
            return D3D11_BLEND();
    }
}

static inline D3D11_BLEND_OP GetD3D11BlendOp(GPUBlendOP blendOP) {
    switch (blendOP)
    {
        case BLEND_OP_ADD:
            return D3D11_BLEND_OP_ADD;
        case BLEND_OP_SUBTRACT:
            return D3D11_BLEND_OP_SUBTRACT;
        case BLEND_OP_REV_SUBTRACT:
            return D3D11_BLEND_OP_REV_SUBTRACT;
        case BLEND_OP_MIN:
            return D3D11_BLEND_OP_MIN;
        case BLEND_OP_MAX:
            return D3D11_BLEND_OP_MAX;
        default:
            return D3D11_BLEND_OP();
    }
}

static inline D3D11_COLOR_WRITE_ENABLE GetD3D11ColorWriteEnable(GPUColorWriteEnable colorWriteEnable) {
    switch (colorWriteEnable)
    {
        case COLOR_WRITE_ENABLE_RED:
            return D3D11_COLOR_WRITE_ENABLE_RED;
        case COLOR_WRITE_ENABLE_GREEN:
            return D3D11_COLOR_WRITE_ENABLE_GREEN;
        case COLOR_WRITE_ENABLE_BLUE:
            return D3D11_COLOR_WRITE_ENABLE_BLUE;
        case COLOR_WRITE_ENABLE_ALPHA:
            return D3D11_COLOR_WRITE_ENABLE_ALPHA;
        case COLOR_WRITE_ENABLE_ALL:
            return D3D11_COLOR_WRITE_ENABLE_ALL;
        default:
            return D3D11_COLOR_WRITE_ENABLE();
    }
}

static inline D3D11_FILL_MODE GetD3D11FillMode(GPUFillMode fillMode) {
    switch (fillMode)
    {
        case FILL_SOLID:
            return D3D11_FILL_SOLID;
        case FILL_WIREFRAME:
            return D3D11_FILL_WIREFRAME;
        default:
            return D3D11_FILL_MODE();
    }
}

static inline D3D11_CULL_MODE GetD3D11CullMode(GPUCullMode cullMode) {
    switch (cullMode)
    {
        case CULL_NONE:
            return D3D11_CULL_NONE;
        case CULL_FRONT:
            return D3D11_CULL_FRONT;
        case CULL_BACK:
            return D3D11_CULL_BACK;
        default:
            return D3D11_CULL_MODE();
    }
}

static inline D3D11_DEPTH_WRITE_MASK GetD3D11DepthWriteMask(GPUDepthWriteMask depthWriteMask) {
    switch (depthWriteMask)
    {
        case DEPTH_WRITE_MASK_ZERO:
            return D3D11_DEPTH_WRITE_MASK_ZERO;
        case DEPTH_WRITE_MASK_ALL:
            return D3D11_DEPTH_WRITE_MASK_ALL;
        default:
            return D3D11_DEPTH_WRITE_MASK();
    }
}

static inline D3D11_COMPARISON_FUNC GetD3D11ComparisonFunc(GPUComparisonFunc comparisonFunc) {
    switch (comparisonFunc)
    {
        case COMPARISON_NEVER:
            return D3D11_COMPARISON_NEVER;
        case COMPARISON_LESS:
            return D3D11_COMPARISON_LESS;
        case COMPARISON_EQUAL:
            return D3D11_COMPARISON_EQUAL;
        case COMPARISON_LESS_EQUAL:
            return D3D11_COMPARISON_LESS_EQUAL;
        case COMPARISON_GREATER:
            return D3D11_COMPARISON_GREATER;
        case COMPARISON_NOT_EQUAL:
            return D3D11_COMPARISON_NOT_EQUAL;
        case COMPARISON_GREATER_EQUAL:
            return D3D11_COMPARISON_GREATER_EQUAL;
        case COMPARISON_ALWAYS:
            return D3D11_COMPARISON_ALWAYS;
        default:
            return D3D11_COMPARISON_FUNC();
    }
}

static inline D3D11_STENCIL_OP GetD3D11StencilOP(GPUStencilOP stencilOP) {
    switch (stencilOP)
    {
        case STENCIL_OP_KEEP:
            return D3D11_STENCIL_OP_KEEP;
        case STENCIL_OP_ZERO:
            return D3D11_STENCIL_OP_ZERO;
        case STENCIL_OP_REPLACE:
            return D3D11_STENCIL_OP_REPLACE;
        case STENCIL_OP_INCR_SAT:
            return D3D11_STENCIL_OP_INCR_SAT;
        case STENCIL_OP_DECR_SAT:
            return D3D11_STENCIL_OP_DECR_SAT;
        case STENCIL_OP_INVERT:
            return D3D11_STENCIL_OP_INVERT;
        case STENCIL_OP_INCR:
            return D3D11_STENCIL_OP_INCR;
        case STENCIL_OP_DECR:
            return D3D11_STENCIL_OP_DECR;
        default:
            return D3D11_STENCIL_OP();
    }
}

static inline D3D11_INPUT_CLASSIFICATION InputClassificationToD3D11(GPUInputClassification classification) {
    switch (classification)
    {
        case PER_VERTEX_DATA:
            return D3D11_INPUT_PER_VERTEX_DATA;
        case PER_INSTANCE_DATA:
            return D3D11_INPUT_PER_INSTANCE_DATA;
        default:
            return D3D11_INPUT_PER_VERTEX_DATA;
    }
}

static inline D3D11_PRIMITIVE_TOPOLOGY GetD3D11PrimitiveTopology(GPUPrimitiveTopology primitiveTopology) {
    switch (primitiveTopology)
    {
        case PRIMITIVE_TOPOLOGY_UNDEFINED:
            return D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
        case PRIMITIVE_TOPOLOGY_POINTLIST:
            return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
        case PRIMITIVE_TOPOLOGY_LINELIST:
            return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        case PRIMITIVE_TOPOLOGY_LINESTRIP:
            return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case PRIMITIVE_TOPOLOGY_TRIANGLELIST:
            return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
            return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        default:
            ASSERT(false, "Unsupported primitive topology");
            break;
    }
}

static inline D3D11_QUERY GetD3D11Query(GPUQueryType queryType) {
    switch (queryType)
    {
        case QUERY_EVENT:
            return D3D11_QUERY_EVENT;
        case QUERY_OCCLUSION:
            return D3D11_QUERY_OCCLUSION;
        case QUERY_TIMESTAMP:
            return D3D11_QUERY_TIMESTAMP;
        case QUERY_TIMESTAMP_DISJOINT:
            return D3D11_QUERY_TIMESTAMP_DISJOINT;
        case QUERY_PIPELINE_STATISTICS:
            return D3D11_QUERY_PIPELINE_STATISTICS;
        case QUERY_OCCLUSION_PREDICATE:
            return D3D11_QUERY_OCCLUSION_PREDICATE;
        default:
            return D3D11_QUERY();
    }
}

static inline D3D11_QUERY_MISC_FLAG GetD3D11QueryMiscFlags(uint8_t flags) {
    return D3D11_QUERY_MISC_FLAG(
        ((flags & GPUQueryFlags::QUERY_MISC_PREDICATEHINT) ? D3D11_QUERY_MISC_PREDICATEHINT : 0)
    );
}

void GpuDraw(u32 vertexCount, u32 vertexBaseLocation)
{
    gState->deviceContext->Draw(vertexCount, vertexBaseLocation);
}

void GpuDrawIndexed(u32 indexCount, u32 startIndex, u32 baseVertexIndex)
{
    gState->deviceContext->DrawIndexed(indexCount, startIndex, baseVertexIndex);
}

void GpuDispatch(u32 threadGroupX, u32 threadGroupY, u32 threadGroupZ)
{
    gState->deviceContext->Dispatch(threadGroupX, threadGroupY, threadGroupZ);
}

void Present()
{
    gState->swapChain->Present(1, 0);
}

GPUBuffer GpuCreateBuffer(const SubresourceData* subresource, u32 size, u32 flags, const char* debugName)
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = (UINT) size;
    bufferDesc.Usage = GetUsageFromResourceFlags(flags);
    bufferDesc.BindFlags = GetBindFlagsFromResourceFlags(flags);
    bufferDesc.CPUAccessFlags = GetCPUAccessFlagFromResourceFlags(flags);
    bufferDesc.MiscFlags = 0;
    bufferDesc.StructureByteStride = 0;

    ID3D11Buffer* d3dBuffer = nullptr;
    if (subresource)
    {
        D3D11_SUBRESOURCE_DATA bufferSubresourceData = {};
        bufferSubresourceData.pSysMem = subresource->data;
        bufferSubresourceData.SysMemPitch = subresource->memPitch;
        bufferSubresourceData.SysMemSlicePitch = subresource->memSlicePitch;

        HRESULT result = gState->device->CreateBuffer(&bufferDesc, &bufferSubresourceData, &d3dBuffer);
        ASSERT(SUCCEEDED(result), "Error at creating buffer");
    }
    else 
    {
        HRESULT result = gState->device->CreateBuffer(&bufferDesc, nullptr, &d3dBuffer);
        ASSERT(SUCCEEDED(result), "Error at creating buffer");
    }

#ifdef DEBUG_GPU_DEVICE
    if (debugName)
    {
        size_t debugNameLen = strlen(debugName);
        d3dBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)debugNameLen, debugName);
    }
#endif

    BufferD3D11 bufferD3D11Object = {};
    bufferD3D11Object.buffer = d3dBuffer;

    Handle handle = ObtainNewHandleFromPool(&gState->bufferPool);
    BufferD3D11* data = (BufferD3D11*) AccessDataFromHandlePool(&gState->bufferPool, handle);
    *data = bufferD3D11Object;

    return GPUBuffer{ handle };
}

GPUTexture2D GpuCreateTexture2D(const Texture2DDesc* initParams, const SubresourceData* subresources, const char* debugName)
{
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = (UINT)initParams->width;
    textureDesc.Height = (UINT)initParams->height;
    textureDesc.MipLevels = (UINT)initParams->mipCount;
    textureDesc.ArraySize = (UINT)initParams->arraySize;
    textureDesc.Format = (DXGI_FORMAT) initParams->format;
    textureDesc.SampleDesc.Count = (UINT)initParams->sampleCount;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = GetUsageFromResourceFlags(initParams->flags);
    textureDesc.BindFlags = GetBindFlagsFromResourceFlags(initParams->flags);
    textureDesc.CPUAccessFlags = GetCPUAccessFlagFromResourceFlags(initParams->flags);
    textureDesc.MiscFlags = GetMiscFlagsFromResourceFlags(initParams->flags);

    ID3D11Texture2D* d3dTexture2D;
    if (subresources) 
    {
        D3D11_SUBRESOURCE_DATA* subresourceDatas = (D3D11_SUBRESOURCE_DATA*) alloca(sizeof(D3D11_SUBRESOURCE_DATA) * initParams->arraySize * initParams->mipCount);
        const SubresourceData* textureSubresource = subresources;
        for (size_t arrayIndex = 0; arrayIndex < initParams->arraySize; ++arrayIndex) 
        {
            for (size_t mipIndex = 0; mipIndex < initParams->mipCount; ++mipIndex) 
            {
                D3D11_SUBRESOURCE_DATA* subresourceData = subresourceDatas + (arrayIndex * initParams->mipCount + mipIndex);
                subresourceData->pSysMem = textureSubresource->data;
                subresourceData->SysMemPitch = textureSubresource->memPitch;
                subresourceData->SysMemSlicePitch = textureSubresource->memSlicePitch;
                ++textureSubresource;
            }
        }

        HRESULT result = gState->device->CreateTexture2D(&textureDesc, subresourceDatas, &d3dTexture2D);
        ASSERT(SUCCEEDED(result), "Error at creating texture");
    }
    else 
    {
        HRESULT result = gState->device->CreateTexture2D(&textureDesc, nullptr, &d3dTexture2D);
        ASSERT(SUCCEEDED(result), "Error at creating texture");
    }

#ifdef DEBUG_GPU_DEVICE
    if (debugName)
    {
        size_t debugNameLen = strlen(debugName);
        d3dTexture2D->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)debugNameLen, debugName);
    }
#endif

    Texture2DD3D11 texture2DD3D11Object = {};
    texture2DD3D11Object.texture2D = d3dTexture2D;

    Handle handle = ObtainNewHandleFromPool(&gState->texture2DPool);
    Texture2DD3D11* data = (Texture2DD3D11*) AccessDataFromHandlePool(&gState->texture2DPool, handle);
    *data = texture2DD3D11Object;

    return GPUTexture2D{ handle };
}

GPURenderTargetView GpuCreateRenderTargetView(GPUTexture2D texture, const GPUResourceViewDesc& resourceViewDesc, const char* debugName)
{
    D3D11_TEXTURE2D_DESC textureDesc;
    Texture2DD3D11* textureD3D11 = (Texture2DD3D11*) AccessDataFromHandlePool(&gState->texture2DPool, texture.handle);
    ID3D11Texture2D* d3dTexturePointer = textureD3D11->texture2D;
    d3dTexturePointer->GetDesc(&textureDesc);

    DXGI_FORMAT textureFormat = textureDesc.Format;
    DXGI_FORMAT renderTargetViewFormat;
    switch (textureFormat) 
    {
        case DXGI_FORMAT_R32_TYPELESS:
            renderTargetViewFormat = DXGI_FORMAT_R32_FLOAT;
            break;
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
            renderTargetViewFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
            break;
        default:
            // TODO: Handle other typeless formats
            renderTargetViewFormat = textureFormat;
            break;
    }

    D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {};
    renderTargetViewDesc.Format = renderTargetViewFormat;
    if (textureDesc.SampleDesc.Count > 1)
    {
        if (textureDesc.ArraySize > 1)
        {
            renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
            renderTargetViewDesc.Texture2DMSArray.ArraySize = resourceViewDesc.sliceArrayCount;
            renderTargetViewDesc.Texture2DMSArray.FirstArraySlice = resourceViewDesc.firstArraySlice;
        }
        else
        {
            renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
        }
    }
    else
    {
        if (textureDesc.ArraySize > 1)
        {
            renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            renderTargetViewDesc.Texture2DArray.ArraySize = resourceViewDesc.sliceArrayCount;
            renderTargetViewDesc.Texture2DArray.FirstArraySlice = resourceViewDesc.firstArraySlice;
            renderTargetViewDesc.Texture2DArray.MipSlice = resourceViewDesc.firstMip;
        }
        else
        {
            renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            renderTargetViewDesc.Texture2D.MipSlice = resourceViewDesc.firstMip;
        }
    }

    ID3D11RenderTargetView* d3dRtv = nullptr;
    HRESULT result = gState->device->CreateRenderTargetView(d3dTexturePointer, &renderTargetViewDesc, &d3dRtv);
    ASSERT(SUCCEEDED(result), "Error at creating render target view");

#ifdef DEBUG_GPU_DEVICE
    if (debugName)
    {
        size_t debugNameLen = strlen(debugName);
        d3dRtv->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)debugNameLen, debugName);
    }
#endif

    RenderTargetViewD3D11 rtvD3D11Object = {};
    rtvD3D11Object.rtv = d3dRtv;

    Handle handle = ObtainNewHandleFromPool(&gState->rtvPool);
    RenderTargetViewD3D11* data = (RenderTargetViewD3D11*) AccessDataFromHandlePool(&gState->rtvPool, handle);
    *data = rtvD3D11Object;

    return GPURenderTargetView{ handle };
}

GPUDepthStencilView GpuCreateDepthStencilView(GPUTexture2D texture, const GPUResourceViewDesc& resourceViewDesc, const char* debugName)
{
    D3D11_TEXTURE2D_DESC textureDesc;
    Texture2DD3D11* textureD3D11 = (Texture2DD3D11*)AccessDataFromHandlePool(&gState->texture2DPool, texture.handle);
    ID3D11Texture2D* d3dTexturePointer = textureD3D11->texture2D;
    d3dTexturePointer->GetDesc(&textureDesc);

    DXGI_FORMAT textureFormat = textureDesc.Format;
    DXGI_FORMAT depthStencilViewFormat;
    switch (textureFormat)
    {
        case DXGI_FORMAT_R32_TYPELESS:
            depthStencilViewFormat = DXGI_FORMAT_D32_FLOAT;
            break;
        case DXGI_FORMAT_R16_TYPELESS:
            depthStencilViewFormat = DXGI_FORMAT_D16_UNORM;
            break;
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            depthStencilViewFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
            break;
        default:
            // TODO: Handle other typeless formats
            depthStencilViewFormat = textureFormat;
            break;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
    depthStencilViewDesc.Format = depthStencilViewFormat;
    if (textureDesc.SampleDesc.Count > 1)
    {
        if (textureDesc.ArraySize > 1)
        {
            depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
            depthStencilViewDesc.Texture2DMSArray.ArraySize = resourceViewDesc.sliceArrayCount;
            depthStencilViewDesc.Texture2DMSArray.FirstArraySlice = resourceViewDesc.firstArraySlice;
        }
        else
        {
            depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
        }
    }
    else
    {
        if (textureDesc.ArraySize > 1)
        {
            depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
            depthStencilViewDesc.Texture2DArray.ArraySize = resourceViewDesc.sliceArrayCount;
            depthStencilViewDesc.Texture2DArray.FirstArraySlice = resourceViewDesc.firstArraySlice;
            depthStencilViewDesc.Texture2DArray.MipSlice = resourceViewDesc.firstMip;
        }
        else
        {
            depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            depthStencilViewDesc.Texture2D.MipSlice = resourceViewDesc.firstMip;
        }
    }

    ID3D11DepthStencilView* d3dDsv = nullptr;
    HRESULT result = gState->device->CreateDepthStencilView(d3dTexturePointer, &depthStencilViewDesc, &d3dDsv);
    ASSERT(SUCCEEDED(result), "Error at creating depth-stencil view");

#ifdef DEBUG_GPU_DEVICE
    if (debugName)
    {
        size_t debugNameLen = strlen(debugName);
        d3dDsv->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)debugNameLen, debugName);
    }
#endif

    DepthStencilViewD3D11 dsvD3D11Object = {};
    dsvD3D11Object.dsv = d3dDsv;

    Handle handle = ObtainNewHandleFromPool(&gState->dsvPool);
    DepthStencilViewD3D11* data = (DepthStencilViewD3D11*) AccessDataFromHandlePool(&gState->dsvPool, handle);
    *data = dsvD3D11Object;

    return GPUDepthStencilView{ handle };
}

GPUShaderResourceView GpuCreateShaderResourceView(GPUTexture2D texture, const GPUResourceViewDesc& resourceViewDesc, const char* debugName)
{
    D3D11_TEXTURE2D_DESC textureDesc;
    Texture2DD3D11* textureD3D11 = (Texture2DD3D11*) AccessDataFromHandlePool(&gState->texture2DPool, texture.handle);
    ID3D11Texture2D* d3dTexturePointer = textureD3D11->texture2D;
    d3dTexturePointer->GetDesc(&textureDesc);

    DXGI_FORMAT textureFormat = textureDesc.Format;
    DXGI_FORMAT shaderResourceViewFormat;
    switch (textureFormat)
    {
        case DXGI_FORMAT_R32_TYPELESS:
            shaderResourceViewFormat = DXGI_FORMAT_R32_FLOAT;
            break;
        case DXGI_FORMAT_R16_TYPELESS:
            shaderResourceViewFormat = DXGI_FORMAT_R16_UNORM;
            break;
        default:
            // TODO: Handle other typeless formats
            shaderResourceViewFormat = textureFormat;
            break;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResouceViewDesc = {};
    shaderResouceViewDesc.Format = shaderResourceViewFormat;
    if (textureDesc.SampleDesc.Count > 1)
    {
        if (textureDesc.ArraySize > 1)
        {
            shaderResouceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
            shaderResouceViewDesc.Texture2DMSArray.FirstArraySlice = resourceViewDesc.firstArraySlice;
            shaderResouceViewDesc.Texture2DMSArray.ArraySize = resourceViewDesc.sliceArrayCount;
        }
        else
        {
            shaderResouceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
        }
    }
    else
    {
        if (textureDesc.ArraySize > 6 && textureDesc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE)
        {
            shaderResouceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
            shaderResouceViewDesc.TextureCubeArray.First2DArrayFace = resourceViewDesc.firstArraySlice;
            shaderResouceViewDesc.TextureCubeArray.NumCubes = resourceViewDesc.sliceArrayCount / 6;
            shaderResouceViewDesc.TextureCubeArray.MostDetailedMip = resourceViewDesc.firstMip;
            shaderResouceViewDesc.TextureCubeArray.MipLevels = resourceViewDesc.mipCount;
        }
        else if (textureDesc.ArraySize > 1 && textureDesc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE)
        {
            shaderResouceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
            shaderResouceViewDesc.TextureCube.MostDetailedMip = resourceViewDesc.firstMip;
            shaderResouceViewDesc.TextureCube.MipLevels = resourceViewDesc.mipCount;
        }
        else if (textureDesc.ArraySize > 1)
        {
            shaderResouceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
            shaderResouceViewDesc.Texture2DArray.FirstArraySlice = resourceViewDesc.firstArraySlice;
            shaderResouceViewDesc.Texture2DArray.ArraySize = resourceViewDesc.sliceArrayCount;
            shaderResouceViewDesc.Texture2DArray.MostDetailedMip = resourceViewDesc.firstMip;
            shaderResouceViewDesc.Texture2DArray.MipLevels = resourceViewDesc.mipCount;
        }
        else
        {
            shaderResouceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            shaderResouceViewDesc.Texture2D.MostDetailedMip = resourceViewDesc.firstMip;
            shaderResouceViewDesc.Texture2D.MipLevels = resourceViewDesc.mipCount;
        }
    }

    ID3D11ShaderResourceView* d3dSrv = nullptr;
    HRESULT result = gState->device->CreateShaderResourceView(d3dTexturePointer, &shaderResouceViewDesc, &d3dSrv);
    ASSERT(SUCCEEDED(result), "Error at creating shader resource view");

#ifdef DEBUG_GPU_DEVICE
    if (debugName)
    {
        size_t debugNameLen = strlen(debugName);
        d3dSrv->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)debugNameLen, debugName);
    }
#endif

    ShaderResourceViewD3D11 srvD3D11Object = {};
    srvD3D11Object.srv = d3dSrv;

    Handle handle = ObtainNewHandleFromPool(&gState->srvPool);
    ShaderResourceViewD3D11* data = (ShaderResourceViewD3D11*) AccessDataFromHandlePool(&gState->srvPool, handle);
    *data = srvD3D11Object;

    return GPUShaderResourceView{ handle };
}

GPUUnorderedAccessView GpuCreateUnorderedAccessView(GPUTexture2D texture, const GPUResourceViewDesc& resourceViewDesc, const char* debugName)
{
    D3D11_TEXTURE2D_DESC textureDesc;
    Texture2DD3D11* textureD3D11 = (Texture2DD3D11*) AccessDataFromHandlePool(&gState->texture2DPool, texture.handle);
    ID3D11Texture2D* d3dTexturePointer = textureD3D11->texture2D;
    d3dTexturePointer->GetDesc(&textureDesc);

    DXGI_FORMAT textureFormat = textureDesc.Format;
    DXGI_FORMAT shaderResourceViewFormat;
    switch (textureFormat)
    {
        case DXGI_FORMAT_R32_TYPELESS:
            shaderResourceViewFormat = DXGI_FORMAT_R32_FLOAT;
            break;
        case DXGI_FORMAT_R16_TYPELESS:
            shaderResourceViewFormat = DXGI_FORMAT_R16_UNORM;
            break;
        default:
            // TODO: Handle other typeless formats
            shaderResourceViewFormat = textureFormat;
            break;
    }

    D3D11_UNORDERED_ACCESS_VIEW_DESC unorderedAccessViewDesc = {};
    unorderedAccessViewDesc.Format = shaderResourceViewFormat;
    if (textureDesc.ArraySize > 1)
    {
        unorderedAccessViewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
        unorderedAccessViewDesc.Texture2DArray.FirstArraySlice = resourceViewDesc.firstArraySlice;
        unorderedAccessViewDesc.Texture2DArray.ArraySize = resourceViewDesc.sliceArrayCount;
        unorderedAccessViewDesc.Texture2DArray.MipSlice = resourceViewDesc.firstMip;
    }
    else
    {
        unorderedAccessViewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
        unorderedAccessViewDesc.Texture2D.MipSlice = resourceViewDesc.firstMip;
    }

    ID3D11UnorderedAccessView* d3dUav = nullptr;
    HRESULT result = gState->device->CreateUnorderedAccessView(d3dTexturePointer, &unorderedAccessViewDesc, &d3dUav);
    ASSERT(SUCCEEDED(result), "Error at creating unordered access view");

#ifdef DEBUG_GPU_DEVICE
    if (debugName) 
    {
        size_t debugNameLen = strlen(debugName);
        d3dUav->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)debugNameLen, debugName);
    }
#endif

    UnorderedAccessViewD3D11 uavD3D11Object = {};
    uavD3D11Object.uav = d3dUav;

    Handle handle = ObtainNewHandleFromPool(&gState->uavPool);
    UnorderedAccessViewD3D11* data = (UnorderedAccessViewD3D11*) AccessDataFromHandlePool(&gState->uavPool, handle);
    *data = uavD3D11Object;

    return GPUUnorderedAccessView{ handle };
}

GPUSamplerState GpuCreateSamplerState(const SamplerStateDesc* desc, const char* debugName)
{
    D3D11_SAMPLER_DESC samplerStateDesc = {};
    samplerStateDesc.Filter = GetD3DFilterFromFilterMode(desc->filterMode, desc->comparisonFunction);
    samplerStateDesc.AddressU = GetD3DAddressModeFromAddressMode(desc->addressModeU);
    samplerStateDesc.AddressV = GetD3DAddressModeFromAddressMode(desc->addressModeV);
    samplerStateDesc.AddressW = GetD3DAddressModeFromAddressMode(desc->addressModeW);
    samplerStateDesc.ComparisonFunc = GetD3D11ComparisonFunc(desc->comparisonFunction);
    samplerStateDesc.MaxAnisotropy = desc->maxAnisotropy;
    samplerStateDesc.MaxLOD = D3D11_FLOAT32_MAX;
    memcpy(samplerStateDesc.BorderColor, desc->borderColor, sizeof(f32) * 4);

    ID3D11SamplerState* d3dSamplerState = nullptr;
    HRESULT result = gState->device->CreateSamplerState(&samplerStateDesc, &d3dSamplerState);
    ASSERT(SUCCEEDED(result), "Error at creating sampler state");

#ifdef DEBUG_GPU_DEVICE
    if (debugName)
    {
        size_t debugNameLen = strlen(debugName);
        d3dSamplerState->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)debugNameLen, debugName);
    }
#endif

    SamplerStateD3D11 samplerD3D11Object = {};
    samplerD3D11Object.samplerState = d3dSamplerState;

    Handle handle = ObtainNewHandleFromPool(&gState->samplerPool);
    SamplerStateD3D11* data = (SamplerStateD3D11*) AccessDataFromHandlePool(&gState->samplerPool, handle);
    *data = samplerD3D11Object;

    return GPUSamplerState{ handle };
}
GPUQuery GpuCreateQuery(const GPUQueryDesc& queryDesc, const char* debugName)
{
    D3D11_QUERY_DESC dxQueryDesc;
    dxQueryDesc.Query = GetD3D11Query(queryDesc.queryType);
    dxQueryDesc.MiscFlags = GetD3D11QueryMiscFlags(queryDesc.flags);

    ID3D11Query* d3dQuery = nullptr;
    HRESULT result = gState->device->CreateQuery(&dxQueryDesc, &d3dQuery);
    ASSERT(SUCCEEDED(result), "Error at creating query");

#ifdef DEBUG_GPU_DEVICE
    if (debugName)
    {
        size_t debugNameLen = strlen(debugName);
        d3dQuery->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)debugNameLen, debugName);
    }
#endif

    QueryD3D11 queryD3D11Object = {};
    queryD3D11Object.query = d3dQuery;

    Handle handle = ObtainNewHandleFromPool(&gState->queryPool);
    QueryD3D11* data = (QueryD3D11*) AccessDataFromHandlePool(&gState->queryPool, handle);
    *data = queryD3D11Object;

    return GPUQuery{ handle };
}

GPUGraphicsPipelineState GpuCreateGraphicsPipelineState(const GPUGraphicsPipelineStateDesc& pipelineDesc, const char* debugName)
{
    ID3D11VertexShader* d3dVertexShader = nullptr;
    GPUShaderBytecode vertexBytecode = pipelineDesc.vertexShader;
    HRESULT result = gState->device->CreateVertexShader(vertexBytecode.shaderBytecode, vertexBytecode.bytecodeLength, 0, &d3dVertexShader);
    ASSERT(SUCCEEDED(result), "Error at creating vertex shader");

    ID3D11PixelShader* d3dPixelShader = nullptr;
    GPUShaderBytecode pixelBytecode = pipelineDesc.pixelShader;
    if (pixelBytecode.shaderBytecode)
    {
        result = gState->device->CreatePixelShader(pixelBytecode.shaderBytecode, pixelBytecode.bytecodeLength, 0, &d3dPixelShader);
        ASSERT(SUCCEEDED(result), "Error at creating pixel shader");
    }

    // Blend state
    GPUBlendStateDesc blendDesc = pipelineDesc.blendStateDesc;
    D3D11_BLEND_DESC dxBlendDesc = {};
    dxBlendDesc.AlphaToCoverageEnable = blendDesc.alphaToCoverageEnable;
    dxBlendDesc.IndependentBlendEnable = blendDesc.independentBlendEnable;
    for (size_t i = 0; i < ARRAYSIZE(dxBlendDesc.RenderTarget); ++i)
    {
        const GPURenderTargetBlendDesc& rtBlendDesc = blendDesc.renderTarget[i];
        D3D11_RENDER_TARGET_BLEND_DESC& dxRenderTargetBlendDesc = dxBlendDesc.RenderTarget[i];

        dxRenderTargetBlendDesc.BlendEnable = rtBlendDesc.blendEnable;
        dxRenderTargetBlendDesc.SrcBlend = GetD3D11Blend(rtBlendDesc.srcBlend);
        dxRenderTargetBlendDesc.DestBlend = GetD3D11Blend(rtBlendDesc.destBlend);
        dxRenderTargetBlendDesc.BlendOp = GetD3D11BlendOp(rtBlendDesc.blendOp);
        dxRenderTargetBlendDesc.SrcBlendAlpha = GetD3D11Blend(rtBlendDesc.srcBlendAlpha);
        dxRenderTargetBlendDesc.DestBlendAlpha = GetD3D11Blend(rtBlendDesc.destBlendAlpha);
        dxRenderTargetBlendDesc.BlendOpAlpha = GetD3D11BlendOp(rtBlendDesc.blendOpAlpha);
        dxRenderTargetBlendDesc.RenderTargetWriteMask = GetD3D11ColorWriteEnable(rtBlendDesc.renderTargetWriteMask);
    }

    ID3D11BlendState* d3dBlendState = nullptr;
    result = gState->device->CreateBlendState(&dxBlendDesc, &d3dBlendState);
    ASSERT(SUCCEEDED(result), "Error at creating blend state");

    // Rasterizer state
    GPURasterizerStateDesc rasterizerDesc = pipelineDesc.rasterizerStateDesc;
    D3D11_RASTERIZER_DESC dxRasterizerDesc = {};
    dxRasterizerDesc.FillMode = GetD3D11FillMode(rasterizerDesc.fillMode);
    dxRasterizerDesc.CullMode = GetD3D11CullMode(rasterizerDesc.cullMode);
    dxRasterizerDesc.FrontCounterClockwise = rasterizerDesc.frontCounterClockwise;
    dxRasterizerDesc.DepthBias = rasterizerDesc.depthBias;
    dxRasterizerDesc.DepthBiasClamp = rasterizerDesc.depthBiasClamp;
    dxRasterizerDesc.SlopeScaledDepthBias = rasterizerDesc.slopeScaledDepthBias;
    dxRasterizerDesc.DepthClipEnable = rasterizerDesc.depthClipEnable;
    dxRasterizerDesc.ScissorEnable = rasterizerDesc.scissorEnable;
    dxRasterizerDesc.MultisampleEnable = rasterizerDesc.multisampleEnable;
    dxRasterizerDesc.AntialiasedLineEnable = rasterizerDesc.antialiasedLineEnable;

    ID3D11RasterizerState* d3dRasterizerState = nullptr;
    result = gState->device->CreateRasterizerState(&dxRasterizerDesc, &d3dRasterizerState);
    ASSERT(SUCCEEDED(result), "Error at creating rasterizer state");

    // Input layout
    ID3D11InputLayout* d3dInputLayout = nullptr;
    GPUInputLayoutDesc inputLayoutDesc = pipelineDesc.inputLayoutDesc;
    if (inputLayoutDesc.elements && inputLayoutDesc.elementCount > 0)
    {
        D3D11_INPUT_ELEMENT_DESC* dxInputElements = (D3D11_INPUT_ELEMENT_DESC*) alloca(sizeof(D3D11_INPUT_ELEMENT_DESC) * inputLayoutDesc.elementCount);
        for (size_t i = 0; i < inputLayoutDesc.elementCount; ++i)
        {
            const GPUVertexInputElement* element = inputLayoutDesc.elements + i;
            D3D11_INPUT_ELEMENT_DESC dx11Element = {};
            dx11Element.SemanticName = element->semanticName;
            dx11Element.SemanticIndex = element->semanticIndex;
            dx11Element.Format = (DXGI_FORMAT) element->format;
            dx11Element.InputSlot = element->inputSlot;
            dx11Element.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
            dx11Element.InputSlotClass = InputClassificationToD3D11(element->classification);
            dx11Element.InstanceDataStepRate = element->instanceStepRate;

            memcpy(dxInputElements + i, &dx11Element, sizeof(D3D11_INPUT_ELEMENT_DESC));
        }

        
        result = gState->device->CreateInputLayout(dxInputElements, (UINT)inputLayoutDesc.elementCount,
            vertexBytecode.shaderBytecode, vertexBytecode.bytecodeLength,
            &d3dInputLayout);
        ASSERT(SUCCEEDED(result), "Error at creating input layout");
    }

    // DepthStencil state
    GPUDepthStencilStateDesc depthStencilDesc = pipelineDesc.depthStencilStateDesc;

    D3D11_DEPTH_STENCIL_DESC dxDepthStencilDesc = {};
    dxDepthStencilDesc.DepthEnable = depthStencilDesc.depthEnable;
    dxDepthStencilDesc.DepthWriteMask = GetD3D11DepthWriteMask(depthStencilDesc.depthWriteMask);
    dxDepthStencilDesc.DepthFunc = GetD3D11ComparisonFunc(depthStencilDesc.depthFunc);
    dxDepthStencilDesc.StencilEnable = depthStencilDesc.stencilEnable;
    dxDepthStencilDesc.StencilReadMask = depthStencilDesc.stencilReadMask;
    dxDepthStencilDesc.StencilWriteMask = depthStencilDesc.stencilWriteMask;
    dxDepthStencilDesc.FrontFace.StencilFunc = GetD3D11ComparisonFunc(depthStencilDesc.frontFace.stencilFunc);
    dxDepthStencilDesc.BackFace.StencilFunc = GetD3D11ComparisonFunc(depthStencilDesc.backFace.stencilFunc);;
    dxDepthStencilDesc.FrontFace.StencilFailOp = GetD3D11StencilOP(depthStencilDesc.frontFace.stencilFailOp);
    dxDepthStencilDesc.BackFace.StencilFailOp = GetD3D11StencilOP(depthStencilDesc.backFace.stencilFailOp);
    dxDepthStencilDesc.FrontFace.StencilPassOp = GetD3D11StencilOP(depthStencilDesc.frontFace.stencilPassOp);
    dxDepthStencilDesc.BackFace.StencilPassOp = GetD3D11StencilOP(depthStencilDesc.backFace.stencilPassOp);
    dxDepthStencilDesc.FrontFace.StencilDepthFailOp = GetD3D11StencilOP(depthStencilDesc.frontFace.stencilDepthFailOp);
    dxDepthStencilDesc.BackFace.StencilDepthFailOp = GetD3D11StencilOP(depthStencilDesc.backFace.stencilDepthFailOp);

    ID3D11DepthStencilState* d3dDepthStencilState = nullptr;
    result = gState->device->CreateDepthStencilState(&dxDepthStencilDesc, &d3dDepthStencilState);
    ASSERT(SUCCEEDED(result), "Error at creating depth-stencil state");

    // Variables
    D3D11_PRIMITIVE_TOPOLOGY d3dPrimitiveTopology = GetD3D11PrimitiveTopology(pipelineDesc.primitiveTopology);
    u32 d3dSampleMask = pipelineDesc.sampleMask;

#ifdef DEBUG_GPU_DEVICE
    if (debugName)
    {
        size_t debugNameLen = strlen(debugName);
        d3dVertexShader->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)debugNameLen, debugName);
        d3dPixelShader->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)debugNameLen, debugName);

        d3dBlendState->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)debugNameLen, debugName);
        d3dRasterizerState->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)debugNameLen, debugName);
        d3dInputLayout->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)debugNameLen, debugName);
        d3dDepthStencilState->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)debugNameLen, debugName);
    }
#endif

    GraphicsPipelineStateD3D11 d3dPSO = {};
    d3dPSO.vertexShader = d3dVertexShader;
    d3dPSO.pixelShader = d3dPixelShader;
    d3dPSO.blendState = d3dBlendState;
    d3dPSO.rasterizerState = d3dRasterizerState;
    d3dPSO.depthStencilState = d3dDepthStencilState;
    d3dPSO.inputLayout = d3dInputLayout;
    d3dPSO.primitiveTopology = d3dPrimitiveTopology;
    d3dPSO.sampleMask = d3dSampleMask;

    Handle handle = ObtainNewHandleFromPool(&gState->graphicsPSOPool);
    GraphicsPipelineStateD3D11* data = (GraphicsPipelineStateD3D11*) AccessDataFromHandlePool(&gState->graphicsPSOPool, handle);
    *data = d3dPSO;

    return GPUGraphicsPipelineState{ handle };

}

GPUComputePipelineState GpuCreateComputePipelineState(const GPUComputePipelineStateDesc& pipelineDesc, const char* debugName)
{
    ID3D11ComputeShader* d3dComputeShader = nullptr;
    GPUShaderBytecode computeShaderBytecode = pipelineDesc.computeShader;
    HRESULT result = gState->device->CreateComputeShader(computeShaderBytecode.shaderBytecode, computeShaderBytecode.bytecodeLength, 0, &d3dComputeShader);
    ASSERT(SUCCEEDED(result), "Error at creating compute shader");

#ifdef DEBUG_GPU_DEVICE
    if (debugName)
    {
        size_t debugNameLen = strlen(debugName);
        d3dComputeShader->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)debugNameLen, debugName);
    }
#endif

    ComputePipelineStateD3D11 d3dPSO = {};
    d3dPSO.computeShader = d3dComputeShader;

    Handle handle = ObtainNewHandleFromPool(&gState->computePSOPool);
    ComputePipelineStateD3D11* data = (ComputePipelineStateD3D11*) AccessDataFromHandlePool(&gState->computePSOPool, handle);
    *data = d3dPSO;

    return GPUComputePipelineState{ handle };
}

void GpuDestroyBuffer(GPUBuffer buffer)
{
    BufferD3D11* d3d11Buffer = (BufferD3D11*) AccessDataFromHandlePool(&gState->bufferPool, buffer.handle);
    d3d11Buffer->buffer->Release();
    ReleaseHandle(&gState->bufferPool, buffer.handle);
}

void GpuDestroyTexture2D(GPUTexture2D texture)
{
    Texture2DD3D11* d3d11Texture2D = (Texture2DD3D11*) AccessDataFromHandlePool(&gState->texture2DPool, texture.handle);
    d3d11Texture2D->texture2D->Release();
    ReleaseHandle(&gState->texture2DPool, texture.handle);
}

void GpuDestroyRenderTargetView(GPURenderTargetView renderTargetView)
{
    RenderTargetViewD3D11* d3d11RTV = (RenderTargetViewD3D11*) AccessDataFromHandlePool(&gState->rtvPool, renderTargetView.handle);
    d3d11RTV->rtv->Release();
    ReleaseHandle(&gState->rtvPool, renderTargetView.handle);
}

void GpuDestroyDepthStencilView(GPUDepthStencilView depthStencilView)
{
    DepthStencilViewD3D11* d3d11DSV = (DepthStencilViewD3D11*) AccessDataFromHandlePool(&gState->dsvPool, depthStencilView.handle);
    d3d11DSV->dsv->Release();
    ReleaseHandle(&gState->dsvPool, depthStencilView.handle);
}

void GpuDestroyShaderResourceView(GPUShaderResourceView shaderResourceView)
{
    ShaderResourceViewD3D11* d3d11SRV = (ShaderResourceViewD3D11*) AccessDataFromHandlePool(&gState->srvPool, shaderResourceView.handle);
    d3d11SRV->srv->Release();
    ReleaseHandle(&gState->srvPool, shaderResourceView.handle);
}

void GpuDestroyUnorderedAccessView(GPUUnorderedAccessView unorderedAccessView)
{
    UnorderedAccessViewD3D11* d3d11UAV = (UnorderedAccessViewD3D11*) AccessDataFromHandlePool(&gState->uavPool, unorderedAccessView.handle);
    d3d11UAV->uav->Release();
    ReleaseHandle(&gState->uavPool, unorderedAccessView.handle);
}

void GpuDestroySamplerState(GPUSamplerState samplerState)
{
    SamplerStateD3D11* d3d11Sampler = (SamplerStateD3D11*) AccessDataFromHandlePool(&gState->samplerPool, samplerState.handle);
    d3d11Sampler->samplerState->Release();
    ReleaseHandle(&gState->samplerPool, samplerState.handle);
}

void GpuDestroyQuery(GPUQuery query)
{
    QueryD3D11* d3d11Query = (QueryD3D11*) AccessDataFromHandlePool(&gState->queryPool, query.handle);
    d3d11Query->query->Release();
    ReleaseHandle(&gState->queryPool, query.handle);
}

void GpuDestroyGraphicsPipelineState(GPUGraphicsPipelineState graphicsPipelineState)
{
    GraphicsPipelineStateD3D11* d3d11PSO = (GraphicsPipelineStateD3D11*) AccessDataFromHandlePool(&gState->graphicsPSOPool, graphicsPipelineState.handle);
    d3d11PSO->vertexShader->Release();
    d3d11PSO->pixelShader->Release();
    d3d11PSO->blendState->Release();
    d3d11PSO->depthStencilState->Release();
    d3d11PSO->inputLayout->Release();
    d3d11PSO->rasterizerState->Release();
    ReleaseHandle(&gState->graphicsPSOPool, graphicsPipelineState.handle);
}

void GpuDestroyComputePipelineState(GPUComputePipelineState computePipelineState)
{
    ComputePipelineStateD3D11* d3d11PSO = (ComputePipelineStateD3D11*) AccessDataFromHandlePool(&gState->computePSOPool, computePipelineState.handle);
    d3d11PSO->computeShader->Release();
    ReleaseHandle(&gState->computePSOPool, computePipelineState.handle);
}

void GpuSetVertexBuffers(GPUBuffer* vertexBuffers, u32 startIndex, u32 vertexBufferCount, u32* strideByteCounts, u32* offsets)
{
    ID3D11Buffer** d3dBuffers = (ID3D11Buffer**) alloca(sizeof(ID3D11Buffer*) * vertexBufferCount);
    for (size_t i = 0; i < vertexBufferCount; ++i)
    {
        GPUBuffer* vertexBuffer = vertexBuffers + i;
        ID3D11Buffer* d3dBuffer = nullptr;
        if (vertexBuffer->handle != INVALID_HANDLE)
        {
            BufferD3D11* bufferObject = (BufferD3D11*) AccessDataFromHandlePool(&gState->bufferPool, vertexBuffer->handle);
            d3dBuffer = bufferObject->buffer;
        }
        *(d3dBuffers + i) = d3dBuffer;
    }

    gState->deviceContext->IASetVertexBuffers(startIndex, vertexBufferCount, d3dBuffers, (UINT*) strideByteCounts, (UINT*) offsets);
}

void GpuSetIndexBuffer(GPUBuffer indexBuffer, u32 strideByteCount, u32 offset)
{
    DXGI_FORMAT indexFormat;
    switch (strideByteCount)
    {
        case 2:
            indexFormat = DXGI_FORMAT_R16_UINT;
            break;
        case 4:
            indexFormat = DXGI_FORMAT_R32_UINT;
            break;
        default:
            ASSERT(false, "Unsupported index buffer byte stride!");
            break;
    }

    ID3D11Buffer* d3dBuffer = nullptr;
    if (indexBuffer.handle != INVALID_HANDLE)
    {
        BufferD3D11* bufferObject = (BufferD3D11*) AccessDataFromHandlePool(&gState->bufferPool, indexBuffer.handle);
        d3dBuffer = bufferObject->buffer;
    }

    gState->deviceContext->IASetIndexBuffer(d3dBuffer, indexFormat, (UINT) offset);
}

void GpuSetRenderTargets(GPURenderTargetView* renderTargets, u32 renderTargetCount, GPUDepthStencilView depthStencilView)
{
    ID3D11RenderTargetView** d3dRTVs = (ID3D11RenderTargetView**) alloca(sizeof(ID3D11RenderTargetView*) * renderTargetCount);
    for (size_t i = 0; i < renderTargetCount; ++i)
    {
        GPURenderTargetView* rtv = renderTargets + i;
        ID3D11RenderTargetView* d3dRTV = nullptr;
        if (rtv->handle != INVALID_HANDLE)
        {
            RenderTargetViewD3D11* rtvObject = (RenderTargetViewD3D11*) AccessDataFromHandlePool(&gState->rtvPool, rtv->handle);
            d3dRTV = rtvObject->rtv;
        }
        *(d3dRTVs + i) = d3dRTV;
    }

    ID3D11DepthStencilView* d3dDSV = nullptr;
    if (depthStencilView.handle != INVALID_HANDLE)
    {
        DepthStencilViewD3D11* dsvObject = (DepthStencilViewD3D11*)AccessDataFromHandlePool(&gState->bufferPool, depthStencilView.handle);
        d3dDSV = dsvObject->dsv;
    }

    gState->deviceContext->OMSetRenderTargets(renderTargetCount, d3dRTVs, d3dDSV);
}

void GpuSetShaderResourcesVS(u32 startSlot, GPUShaderResourceView* shaderResources, u32 shaderResourceCount)
{
    ID3D11ShaderResourceView** d3dSRVs = (ID3D11ShaderResourceView**) alloca(sizeof(ID3D11ShaderResourceView*) * shaderResourceCount);
    for (size_t i = 0; i < shaderResourceCount; ++i)
    {
        GPUShaderResourceView* srv = shaderResources + i;
        ID3D11ShaderResourceView* d3dSRV = nullptr;
        if (srv->handle != INVALID_HANDLE)
        {
            ShaderResourceViewD3D11* srvObject = (ShaderResourceViewD3D11*)AccessDataFromHandlePool(&gState->srvPool, srv->handle);
            d3dSRV = srvObject->srv;
        }
        *(d3dSRVs + i) = d3dSRV;
    }

    gState->deviceContext->VSSetShaderResources(startSlot, shaderResourceCount, d3dSRVs);
}

void GpuSetShaderResourcesPS(u32 startSlot, GPUShaderResourceView* shaderResources, u32 shaderResourceCount)
{
    ID3D11ShaderResourceView** d3dSRVs = (ID3D11ShaderResourceView**) alloca(sizeof(ID3D11ShaderResourceView*) * shaderResourceCount);
    for (size_t i = 0; i < shaderResourceCount; ++i)
    {
        GPUShaderResourceView* srv = shaderResources + i;
        ID3D11ShaderResourceView* d3dSRV = nullptr;
        if (srv->handle != INVALID_HANDLE)
        {
            ShaderResourceViewD3D11* srvObject = (ShaderResourceViewD3D11*)AccessDataFromHandlePool(&gState->srvPool, srv->handle);
            d3dSRV = srvObject->srv;
        }
        *(d3dSRVs + i) = d3dSRV;
    }

    gState->deviceContext->PSSetShaderResources(startSlot, shaderResourceCount, d3dSRVs);
}

void GpuSetShaderResourcesCS(u32 startSlot, GPUShaderResourceView* shaderResources, u32 shaderResourceCount)
{
    ID3D11ShaderResourceView** d3dSRVs = (ID3D11ShaderResourceView**) alloca(sizeof(ID3D11ShaderResourceView*) * shaderResourceCount);
    for (size_t i = 0; i < shaderResourceCount; ++i)
    {
        GPUShaderResourceView* srv = shaderResources + i;
        ID3D11ShaderResourceView* d3dSRV = nullptr;
        if (srv->handle != INVALID_HANDLE)
        {
            ShaderResourceViewD3D11* srvObject = (ShaderResourceViewD3D11*) AccessDataFromHandlePool(&gState->srvPool, srv->handle);
            d3dSRV = srvObject->srv;
        }
        *(d3dSRVs + i) = d3dSRV;
    }

    gState->deviceContext->CSSetShaderResources(startSlot, shaderResourceCount, d3dSRVs);
}

void GpuSetUnorderedAcessViewsCS(u32 startSlot, GPUUnorderedAccessView* unorderedAccessViews, u32 uavCount, u32* uavInitialCounts)
{
    ID3D11UnorderedAccessView** d3dUAVs = (ID3D11UnorderedAccessView**) alloca(sizeof(ID3D11UnorderedAccessView*) * uavCount);
    for (size_t i = 0; i < uavCount; ++i)
    {
        GPUUnorderedAccessView* uav = unorderedAccessViews + i;
        ID3D11UnorderedAccessView* d3dUAV = nullptr;
        if (uav->handle != INVALID_HANDLE)
        {
            UnorderedAccessViewD3D11* uavObject = (UnorderedAccessViewD3D11*) AccessDataFromHandlePool(&gState->uavPool, uav->handle);
            d3dUAV = uavObject->uav;
        }
        *(d3dUAVs + i) = d3dUAV;
    }

    gState->deviceContext->CSSetUnorderedAccessViews(startSlot, uavCount, d3dUAVs, uavInitialCounts);
}

void GpuSetSamplerStatesVS(u32 startSlot, GPUSamplerState* samplerStates, u32 samplerStateCount)
{
    ID3D11SamplerState** d3dSamplers = (ID3D11SamplerState**) alloca(sizeof(ID3D11SamplerState*) * samplerStateCount);
    for (size_t i = 0; i < samplerStateCount; ++i)
    {
        GPUSamplerState* sampler = samplerStates + i;
        ID3D11SamplerState* d3dSampler = nullptr;
        if (sampler->handle != INVALID_HANDLE)
        {
            SamplerStateD3D11* samplerObject = (SamplerStateD3D11*) AccessDataFromHandlePool(&gState->samplerPool, sampler->handle);
            d3dSampler = samplerObject->samplerState;
        }
        *(d3dSamplers + i) = d3dSampler;
    }

    gState->deviceContext->VSSetSamplers(startSlot, samplerStateCount, d3dSamplers);
}

void GpuSetSamplerStatesPS(u32 startSlot, GPUSamplerState* samplerStates, u32 samplerStateCount)
{
    ID3D11SamplerState** d3dSamplers = (ID3D11SamplerState**) alloca(sizeof(ID3D11SamplerState*) * samplerStateCount);
    for (size_t i = 0; i < samplerStateCount; ++i)
    {
        GPUSamplerState* sampler = samplerStates + i;
        ID3D11SamplerState* d3dSampler = nullptr;
        if (sampler->handle != INVALID_HANDLE)
        {
            SamplerStateD3D11* samplerObject = (SamplerStateD3D11*) AccessDataFromHandlePool(&gState->samplerPool, sampler->handle);
            d3dSampler = samplerObject->samplerState;
        }
        *(d3dSamplers + i) = d3dSampler;
    }

    gState->deviceContext->PSSetSamplers(startSlot, samplerStateCount, d3dSamplers);
}

void GpuSetSamplerStatesCS(u32 startSlot, GPUSamplerState* samplerStates, u32 samplerStateCount)
{
    ID3D11SamplerState** d3dSamplers = (ID3D11SamplerState**) alloca(sizeof(ID3D11SamplerState*) * samplerStateCount);
    for (size_t i = 0; i < samplerStateCount; ++i)
    {
        GPUSamplerState* sampler = samplerStates + i;
        ID3D11SamplerState* d3dSampler = nullptr;
        if (sampler->handle != INVALID_HANDLE)
        {
            SamplerStateD3D11* samplerObject = (SamplerStateD3D11*) AccessDataFromHandlePool(&gState->samplerPool, sampler->handle);
            d3dSampler = samplerObject->samplerState;
        }
        *(d3dSamplers + i) = d3dSampler;
    }

    gState->deviceContext->CSSetSamplers(startSlot, samplerStateCount, d3dSamplers);
}

void GpuSetGraphicsPipelineState(GPUGraphicsPipelineState pipelineState)
{
    GraphicsPipelineStateD3D11* d3dPSO = (GraphicsPipelineStateD3D11*) AccessDataFromHandlePool(&gState->graphicsPSOPool, pipelineState.handle);

    gState->deviceContext->IASetPrimitiveTopology(d3dPSO->primitiveTopology);
    gState->deviceContext->IASetInputLayout(d3dPSO->inputLayout);
    gState->deviceContext->RSSetState(d3dPSO->rasterizerState);
    gState->deviceContext->OMSetBlendState(d3dPSO->blendState, nullptr, d3dPSO->sampleMask);
    gState->deviceContext->OMSetDepthStencilState(d3dPSO->depthStencilState, 0xFF);

    gState->deviceContext->VSSetShader(d3dPSO->vertexShader, nullptr, 0);
    gState->deviceContext->PSSetShader(d3dPSO->pixelShader, nullptr, 0);
}

void GpuSetComputePipelineState(GPUComputePipelineState pipelineState)
{
    ComputePipelineStateD3D11* d3dPSO = (ComputePipelineStateD3D11*) AccessDataFromHandlePool(&gState->computePSOPool, pipelineState.handle);

    gState->deviceContext->CSSetShader(d3dPSO->computeShader, nullptr, 0);
}

void GpuSetViewports(const GPUViewport* viewports, u32 viewportCount)
{
    D3D11_VIEWPORT* d3dViewports = (D3D11_VIEWPORT*) alloca(sizeof(D3D11_VIEWPORT) * viewportCount);
    for (size_t i = 0; i < viewportCount; ++i)
    {
        const GPUViewport* viewport = viewports + i;
        D3D11_VIEWPORT* d3dViewport = d3dViewports + i;
        d3dViewport->TopLeftX = viewport->topLeftX;
        d3dViewport->TopLeftY = viewport->topLeftY;
        d3dViewport->Width = viewport->width;
        d3dViewport->Height = viewport->height;
        d3dViewport->MinDepth = viewport->minDepth;
        d3dViewport->MaxDepth = viewport->maxDepth;
    }

    gState->deviceContext->RSSetViewports(viewportCount, d3dViewports);
}

void GpuSetScissorRects(const GPURect* rects, u32 rectCount)
{
    D3D11_RECT* d3dScissorRects = (D3D11_RECT*) alloca(sizeof(D3D11_RECT) * rectCount);
    for (size_t i = 0; i < rectCount; ++i)
    {
        const GPURect* rect = rects + i;
        D3D11_RECT* d3dRect = d3dScissorRects + i;
        d3dRect->top = rect->top;
        d3dRect->left = rect->left;
        d3dRect->right = rect->right;
        d3dRect->bottom = rect->bottom;
    }

    gState->deviceContext->RSSetScissorRects(rectCount, d3dScissorRects);
}

void GpuSetConstantBuffersVS(u32 startSlot, GPUBuffer* constantBuffers, u32 count)
{
    ID3D11Buffer** d3dBuffers = (ID3D11Buffer**) alloca(sizeof(ID3D11Buffer*) * count);
    for (size_t i = 0; i < count; ++i)
    {
        GPUBuffer* buffer = constantBuffers + i;
        ID3D11Buffer* d3dBuffer = nullptr;
        if (buffer->handle != INVALID_HANDLE)
        {
            BufferD3D11* bufferObject = (BufferD3D11*) AccessDataFromHandlePool(&gState->bufferPool, buffer->handle);
            d3dBuffer = bufferObject->buffer;
        }
        *(d3dBuffers + i) = d3dBuffer;
    }

    gState->deviceContext->VSSetConstantBuffers(startSlot, count, d3dBuffers);
}

void GpuSetConstantBuffersPS(u32 startSlot, GPUBuffer* constantBuffers, u32 count)
{
    ID3D11Buffer** d3dBuffers = (ID3D11Buffer**) alloca(sizeof(ID3D11Buffer*) * count);
    for (size_t i = 0; i < count; ++i)
    {
        GPUBuffer* buffer = constantBuffers + i;
        ID3D11Buffer* d3dBuffer = nullptr;
        if (buffer->handle != INVALID_HANDLE)
        {
            BufferD3D11* bufferObject = (BufferD3D11*) AccessDataFromHandlePool(&gState->bufferPool, buffer->handle);
            d3dBuffer = bufferObject->buffer;
        }
        *(d3dBuffers + i) = d3dBuffer;
    }

    gState->deviceContext->PSSetConstantBuffers(startSlot, count, d3dBuffers);
}

void GpuSetConstantBuffersCS(u32 startSlot, GPUBuffer* constantBuffers, u32 count)
{
    ID3D11Buffer** d3dBuffers = (ID3D11Buffer**) alloca(sizeof(ID3D11Buffer*) * count);
    for (size_t i = 0; i < count; ++i)
    {
        GPUBuffer* buffer = constantBuffers + i;
        ID3D11Buffer* d3dBuffer = nullptr;
        if (buffer->handle != INVALID_HANDLE)
        {
            BufferD3D11* bufferObject = (BufferD3D11*)AccessDataFromHandlePool(&gState->bufferPool, buffer->handle);
            d3dBuffer = bufferObject->buffer;
        }
        *(d3dBuffers + i) = d3dBuffer;
    }

    gState->deviceContext->CSSetConstantBuffers(startSlot, count, d3dBuffers);
}

void GpuClearRenderTarget(GPURenderTargetView renderTargetView, f32 color[4])
{
    RenderTargetViewD3D11* rtvObject = (RenderTargetViewD3D11*) AccessDataFromHandlePool(&gState->rtvPool, renderTargetView.handle);
    ID3D11RenderTargetView* d3dRTV = rtvObject->rtv;
    gState->deviceContext->ClearRenderTargetView(d3dRTV, color);
}

void GpuClearDepthStencilView(GPUDepthStencilView depthStencilView, bool clearStencil, f32 depthClearData, u8 stencilClearData)
{
    DepthStencilViewD3D11* dsvObject = (DepthStencilViewD3D11*) AccessDataFromHandlePool(&gState->rtvPool, depthStencilView.handle);
    ID3D11DepthStencilView* d3dSRV = dsvObject->dsv;
    D3D11_CLEAR_FLAG clearFlag = D3D11_CLEAR_DEPTH;
    if (clearStencil)
    {
        clearFlag = D3D11_CLEAR_FLAG(clearFlag | D3D11_CLEAR_STENCIL);
    }
    gState->deviceContext->ClearDepthStencilView(d3dSRV, clearFlag, depthClearData, stencilClearData);
}

void GpuCopyTexture(GPUTexture2D source, GPUTexture2D destination)
{
    Texture2DD3D11* sourceTexture = (Texture2DD3D11*) AccessDataFromHandlePool(&gState->texture2DPool, source.handle);
    Texture2DD3D11* destinationTexture = (Texture2DD3D11*) AccessDataFromHandlePool(&gState->texture2DPool, destination.handle);
    ID3D11Texture2D* d3dSourceTexture = sourceTexture->texture2D;
    ID3D11Texture2D* d3dDestinationTexture = destinationTexture->texture2D;

    gState->deviceContext->CopyResource(d3dDestinationTexture, d3dSourceTexture);
}

void* GpuMapBuffer(GPUBuffer resource)
{
    BufferD3D11* buffer = (BufferD3D11*) AccessDataFromHandlePool(&gState->bufferPool, resource.handle);
    ID3D11Buffer* d3dBuffer = buffer->buffer;

    D3D11_MAPPED_SUBRESOURCE mappedResource = {};
    gState->deviceContext->Map(d3dBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    return mappedResource.pData;
}

void GpuUnmapBuffer(GPUBuffer resource)
{
    BufferD3D11* buffer = (BufferD3D11*) AccessDataFromHandlePool(&gState->bufferPool, resource.handle);
    ID3D11Buffer* d3dBuffer = buffer->buffer;
    gState->deviceContext->Unmap(d3dBuffer, 0);
}

void GpuUpdateTextureSubresource(GPUTexture2D resource, u32 dstSubresource, const GPUBox* updateBox, const SubresourceData& subresourceData)
{
    Texture2DD3D11* texture = (Texture2DD3D11*) AccessDataFromHandlePool(&gState->texture2DPool, resource.handle);
    ID3D11Texture2D* d3dTexture = texture->texture2D;

    gState->deviceContext->UpdateSubresource(d3dTexture, (UINT)dstSubresource, (const D3D11_BOX*)updateBox,
        subresourceData.data, subresourceData.memPitch, subresourceData.memSlicePitch);
}

void GpuMSAAResolve(GPUTexture2D dest, GPUTexture2D source)
{
    Texture2DD3D11* sourceTexture = (Texture2DD3D11*) AccessDataFromHandlePool(&gState->texture2DPool, source.handle);
    Texture2DD3D11* destinationTexture = (Texture2DD3D11*) AccessDataFromHandlePool(&gState->texture2DPool, dest.handle);
    ID3D11Texture2D* d3dSourceTexture = sourceTexture->texture2D;
    ID3D11Texture2D* d3dDestinationTexture = destinationTexture->texture2D;

    D3D11_TEXTURE2D_DESC destDesc;
    d3dDestinationTexture->GetDesc(&destDesc);
    gState->deviceContext->ResolveSubresource(d3dDestinationTexture, 0, d3dSourceTexture, 0, destDesc.Format);
}

void GpuGenerateMips(GPUShaderResourceView shaderResourceView)
{
    ShaderResourceViewD3D11* srv = (ShaderResourceViewD3D11*) AccessDataFromHandlePool(&gState->srvPool, shaderResourceView.handle);
    ID3D11ShaderResourceView* d3dSRV = srv->srv;
    gState->deviceContext->GenerateMips(d3dSRV);
}

void GpuBeginEvent(const char* eventName)
{
    const size_t eventNameSize = strlen(eventName) + 1;
    wchar_t* wcEventName = (wchar_t*) alloca(sizeof(wchar_t) * eventNameSize);

    size_t numberOfConverted;
    mbstowcs_s(&numberOfConverted, wcEventName, eventNameSize, eventName, eventNameSize - 1);

    gState->userDefinedAnnotation->BeginEvent(wcEventName);
}

void GpuEndEvent()
{
    gState->userDefinedAnnotation->EndEvent();
}

void GpuBeginQuery(GPUQuery query)
{
    QueryD3D11* queryObject = (QueryD3D11*) AccessDataFromHandlePool(&gState->queryPool, query.handle);
    ID3D11Query* d3dQuery = queryObject->query;
    gState->deviceContext->Begin(d3dQuery);
}

void GpuEndQuery(GPUQuery query)
{
    QueryD3D11* queryObject = (QueryD3D11*) AccessDataFromHandlePool(&gState->queryPool, query.handle);
    ID3D11Query* d3dQuery = queryObject->query;
    gState->deviceContext->End(d3dQuery);
}

bool GpuGetDataQuery(GPUQuery query, void* outData, u32 size, u32 flags)
{
    QueryD3D11* queryObject = (QueryD3D11*) AccessDataFromHandlePool(&gState->queryPool, query.handle);
    ID3D11Query* d3dQuery = queryObject->query;
    HRESULT result = gState->deviceContext->GetData(d3dQuery, outData, size, flags);
    return false;
}

extern "C"
{

    MODULE_EXPORT void LoadPlugin(APIRegistry* registry, bool reload)
    {
        gAPIRegistry = registry;

        RHIAPI rhiAPI = {};
        if (reload)
        {
            RHIAPI* api = (RHIAPI*) registry->Get(RHI_API_NAME);
            ASSERT(api, "Can't find API on reload");
            gState = (D3D11State*) api->state;
        }

        rhiAPI.state = (void*) gState;
        rhiAPI.Init = GpuInit;
        rhiAPI.GetBackbufferRTV = GpuGetBackbufferRTV;
        rhiAPI.Draw = GpuDraw;
        rhiAPI.DrawIndexed = GpuDrawIndexed;
        rhiAPI.Dispatch = GpuDispatch;
        rhiAPI.Present = Present;
        rhiAPI.CreateBuffer = GpuCreateBuffer;
        rhiAPI.CreateTexture2D = GpuCreateTexture2D;
        rhiAPI.CreateRenderTargetView = GpuCreateRenderTargetView;
        rhiAPI.CreateDepthStencilView = GpuCreateDepthStencilView;
        rhiAPI.CreateShaderResourceView = GpuCreateShaderResourceView;
        rhiAPI.CreateUnorderedAccessView = GpuCreateUnorderedAccessView;
        rhiAPI.CreateSamplerState = GpuCreateSamplerState;
        rhiAPI.CreateQuery = GpuCreateQuery;
        rhiAPI.CreateGraphicsPipelineState = GpuCreateGraphicsPipelineState;
        rhiAPI.CreateComputePipelineState = GpuCreateComputePipelineState;
        rhiAPI.DestroyBuffer = GpuDestroyBuffer;
        rhiAPI.DestroyTexture2D = GpuDestroyTexture2D;
        rhiAPI.DestroyRenderTargetView = GpuDestroyRenderTargetView;
        rhiAPI.DestroyDepthStencilView = GpuDestroyDepthStencilView;
        rhiAPI.DestroyShaderResourceView = GpuDestroyShaderResourceView;
        rhiAPI.DestroyUnorderedAccessView = GpuDestroyUnorderedAccessView;
        rhiAPI.DestroySamplerState = GpuDestroySamplerState;
        rhiAPI.DestroyQuery = GpuDestroyQuery;
        rhiAPI.DestroyGraphicsPipelineState = GpuDestroyGraphicsPipelineState;
        rhiAPI.DestroyComputePipelineState = GpuDestroyComputePipelineState;
        rhiAPI.SetVertexBuffers = GpuSetVertexBuffers;
        rhiAPI.SetIndexBuffer = GpuSetIndexBuffer;
        rhiAPI.SetRenderTargets = GpuSetRenderTargets;
        rhiAPI.SetShaderResourcesVS = GpuSetShaderResourcesVS;
        rhiAPI.SetShaderResourcesPS = GpuSetShaderResourcesPS;
        rhiAPI.SetShaderResourcesCS = GpuSetShaderResourcesCS;
        rhiAPI.SetUnorderedAcessViewsCS = GpuSetUnorderedAcessViewsCS;
        rhiAPI.SetSamplerStatesVS = GpuSetSamplerStatesVS;
        rhiAPI.SetSamplerStatesPS = GpuSetSamplerStatesPS;
        rhiAPI.SetSamplerStatesCS = GpuSetSamplerStatesCS;
        rhiAPI.SetGraphicsPipelineState = GpuSetGraphicsPipelineState;
        rhiAPI.SetComputePipelineState = GpuSetComputePipelineState;
        rhiAPI.SetViewports = GpuSetViewports;
        rhiAPI.SetScissorRects = GpuSetScissorRects;
        rhiAPI.SetConstantBuffersVS = GpuSetConstantBuffersVS;
        rhiAPI.SetConstantBuffersPS = GpuSetConstantBuffersPS;
        rhiAPI.SetConstantBuffersCS = GpuSetConstantBuffersCS;
        rhiAPI.ClearRenderTarget = GpuClearRenderTarget;
        rhiAPI.ClearDepthStencilView = GpuClearDepthStencilView;
        rhiAPI.CopyTexture = GpuCopyTexture;
        rhiAPI.MapBuffer = GpuMapBuffer;
        rhiAPI.UnmapBuffer = GpuUnmapBuffer;
        rhiAPI.UpdateTextureSubresource = GpuUpdateTextureSubresource;
        rhiAPI.MSAAResolve = GpuMSAAResolve;
        rhiAPI.GenerateMips = GpuGenerateMips;
        rhiAPI.BeginEvent = GpuBeginEvent;
        rhiAPI.EndEvent = GpuEndEvent;
        rhiAPI.BeginQuery = GpuBeginQuery;
        rhiAPI.EndQuery = GpuEndQuery;
        rhiAPI.GetDataQuery = GpuGetDataQuery;

        registry->Set(RHI_API_NAME, &rhiAPI, sizeof(RHIAPI));
    }

    MODULE_EXPORT void UnloadPlugin(APIRegistry* registry, bool reload)
    {
        registry->Remove(RHI_API_NAME);
    }
}