#pragma once

struct APIRegistry;

struct GPUBuffer 
{
    Handle handle = INVALID_HANDLE;
};

struct GPUTexture2D
{
    Handle handle = INVALID_HANDLE;
};

struct GPURenderTargetView
{
    Handle handle = INVALID_HANDLE;
};

struct GPUDepthStencilView
{
    Handle handle = INVALID_HANDLE;
};

struct GPUShaderResourceView
{
    Handle handle = INVALID_HANDLE;
};

struct GPUUnorderedAccessView
{
    Handle handle = INVALID_HANDLE;
};

struct GPUSamplerState
{
    Handle handle = INVALID_HANDLE;
};

struct GPUQuery
{
    Handle handle = INVALID_HANDLE;
};

struct GPUGraphicsPipelineState
{
    Handle handle = INVALID_HANDLE;
};

struct GPUComputePipelineState
{
    Handle handle = INVALID_HANDLE;
};

enum DataFormat
{
    FORMAT_UNKNOWN = 0,
    FORMAT_R32G32B32A32_TYPELESS = 1,
    FORMAT_R32G32B32A32_FLOAT = 2,
    FORMAT_R32G32B32A32_UINT = 3,
    FORMAT_R32G32B32A32_SINT = 4,
    FORMAT_R32G32B32_TYPELESS = 5,
    FORMAT_R32G32B32_FLOAT = 6,
    FORMAT_R32G32B32_UINT = 7,
    FORMAT_R32G32B32_SINT = 8,
    FORMAT_R16G16B16A16_TYPELESS = 9,
    FORMAT_R16G16B16A16_FLOAT = 10,
    FORMAT_R16G16B16A16_UNORM = 11,
    FORMAT_R16G16B16A16_UINT = 12,
    FORMAT_R16G16B16A16_SNORM = 13,
    FORMAT_R16G16B16A16_SINT = 14,
    FORMAT_R32G32_TYPELESS = 15,
    FORMAT_R32G32_FLOAT = 16,
    FORMAT_R32G32_UINT = 17,
    FORMAT_R32G32_SINT = 18,
    FORMAT_R32G8X24_TYPELESS = 19,
    FORMAT_D32_FLOAT_S8X24_UINT = 20,
    FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
    FORMAT_X32_TYPELESS_G8X24_UINT = 22,
    FORMAT_R10G10B10A2_TYPELESS = 23,
    FORMAT_R10G10B10A2_UNORM = 24,
    FORMAT_R10G10B10A2_UINT = 25,
    FORMAT_R11G11B10_FLOAT = 26,
    FORMAT_R8G8B8A8_TYPELESS = 27,
    FORMAT_R8G8B8A8_UNORM = 28,
    FORMAT_R8G8B8A8_UNORM_SRGB = 29,
    FORMAT_R8G8B8A8_UINT = 30,
    FORMAT_R8G8B8A8_SNORM = 31,
    FORMAT_R8G8B8A8_SINT = 32,
    FORMAT_R16G16_TYPELESS = 33,
    FORMAT_R16G16_FLOAT = 34,
    FORMAT_R16G16_UNORM = 35,
    FORMAT_R16G16_UINT = 36,
    FORMAT_R16G16_SNORM = 37,
    FORMAT_R16G16_SINT = 38,
    FORMAT_R32_TYPELESS = 39,
    FORMAT_D32_FLOAT = 40,
    FORMAT_R32_FLOAT = 41,
    FORMAT_R32_UINT = 42,
    FORMAT_R32_SINT = 43,
    FORMAT_R24G8_TYPELESS = 44,
    FORMAT_D24_UNORM_S8_UINT = 45,
    FORMAT_R24_UNORM_X8_TYPELESS = 46,
    FORMAT_X24_TYPELESS_G8_UINT = 47,
    FORMAT_R8G8_TYPELESS = 48,
    FORMAT_R8G8_UNORM = 49,
    FORMAT_R8G8_UINT = 50,
    FORMAT_R8G8_SNORM = 51,
    FORMAT_R8G8_SINT = 52,
    FORMAT_R16_TYPELESS = 53,
    FORMAT_R16_FLOAT = 54,
    FORMAT_D16_UNORM = 55,
    FORMAT_R16_UNORM = 56,
    FORMAT_R16_UINT = 57,
    FORMAT_R16_SNORM = 58,
    FORMAT_R16_SINT = 59,
    FORMAT_R8_TYPELESS = 60,
    FORMAT_R8_UNORM = 61,
    FORMAT_R8_UINT = 62,
    FORMAT_R8_SNORM = 63,
    FORMAT_R8_SINT = 64,
    FORMAT_A8_UNORM = 65,
    FORMAT_R1_UNORM = 66,
    FORMAT_R9G9B9E5_SHAREDEXP = 67,
    FORMAT_R8G8_B8G8_UNORM = 68,
    FORMAT_G8R8_G8B8_UNORM = 69,
    FORMAT_BC1_TYPELESS = 70,
    FORMAT_BC1_UNORM = 71,
    FORMAT_BC1_UNORM_SRGB = 72,
    FORMAT_BC2_TYPELESS = 73,
    FORMAT_BC2_UNORM = 74,
    FORMAT_BC2_UNORM_SRGB = 75,
    FORMAT_BC3_TYPELESS = 76,
    FORMAT_BC3_UNORM = 77,
    FORMAT_BC3_UNORM_SRGB = 78,
    FORMAT_BC4_TYPELESS = 79,
    FORMAT_BC4_UNORM = 80,
    FORMAT_BC4_SNORM = 81,
    FORMAT_BC5_TYPELESS = 82,
    FORMAT_BC5_UNORM = 83,
    FORMAT_BC5_SNORM = 84,
    FORMAT_B5G6R5_UNORM = 85,
    FORMAT_B5G5R5A1_UNORM = 86,
    FORMAT_B8G8R8A8_UNORM = 87,
    FORMAT_B8G8R8X8_UNORM = 88,
    FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
    FORMAT_B8G8R8A8_TYPELESS = 90,
    FORMAT_B8G8R8A8_UNORM_SRGB = 91,
    FORMAT_B8G8R8X8_TYPELESS = 92,
    FORMAT_B8G8R8X8_UNORM_SRGB = 93,
    FORMAT_BC6H_TYPELESS = 94,
    FORMAT_BC6H_UF16 = 95,
    FORMAT_BC6H_SF16 = 96,
    FORMAT_BC7_TYPELESS = 97,
    FORMAT_BC7_UNORM = 98,
    FORMAT_BC7_UNORM_SRGB = 99,
    FORMAT_AYUV = 100,
    FORMAT_Y410 = 101,
    FORMAT_Y416 = 102,
    FORMAT_NV12 = 103,
    FORMAT_P010 = 104,
    FORMAT_P016 = 105,
    FORMAT_420_OPAQUE = 106,
    FORMAT_YUY2 = 107,
    FORMAT_Y210 = 108,
    FORMAT_Y216 = 109,
    FORMAT_NV11 = 110,
    FORMAT_AI44 = 111,
    FORMAT_IA44 = 112,
    FORMAT_P8 = 113,
    FORMAT_A8P8 = 114,
    FORMAT_B4G4R4A4_UNORM = 115,

    FORMAT_P208 = 130,
    FORMAT_V208 = 131,
    FORMAT_V408 = 132,


    FORMAT_FORCE_UINT = 0xffffffff
};

enum GPUResourceFlags
{
    // Bind flags
    BIND_SHADER_RESOURCE = BIT(1),
    BIND_RENDER_TARGET = BIT(2),
    BIND_DEPTH_STENCIL = BIT(3),
    BIND_UNORDERED_ACCESS = BIT(4),

    BIND_VERTEX_BUFFER = BIT(5),
    BIND_INDEX_BUFFER = BIT(6),
    BIND_CONSTANT_BUFFER = BIT(7),

    // Usage flags
    CPU_ACCESS_READ = BIT(10),
    CPU_ACCESS_WRITE = BIT(11),

    USAGE_DYNAMIC = BIT(12),
    USAGE_STAGING = BIT(13),
    USAGE_IMMUTABLE = BIT(14),

    MISC_TEXTURE_CUBE = BIT(15),
    MISC_GENERATE_MIPS = BIT(16)
};

struct Texture2DDesc
{
    u32 width;
    u32 height;
    DataFormat format;
    u32 flags;
    u32 sampleCount;
    u32 arraySize;
    u32 mipCount;
};

struct GPUResourceViewDesc
{
    u32 firstArraySlice = 0;
    u32 sliceArrayCount = 1;
    u32 firstMip = 0;
    u32 mipCount = 1;
};

struct SubresourceData
{
    const void* data = nullptr;
    u32 memPitch = 0;
    u32 memSlicePitch = 0;
};

enum SamplerStateTextureAddressMode
{
    TextureAddressMode_NONE,
    TextureAddressMode_WRAP,
    TextureAddressMode_MIRROR,
    TextureAddressMode_CLAMP,
    TextureAddressMode_BORDER
};

enum SamplerStateTextureFilterMode
{
    TextureFilterMode_MIN_MAG_MIP_LINEAR,
    TextureFilterMode_MIN_MAG_MIP_POINT,
    TextureFilterMode_MIN_MAG_LINEAR_MIP_POINT,
    TextureFilterMode_ANISOTROPIC
};

enum GPUComparisonFunc
{
    COMPARISON_NEVER,
    COMPARISON_LESS,
    COMPARISON_EQUAL,
    COMPARISON_LESS_EQUAL,
    COMPARISON_GREATER,
    COMPARISON_NOT_EQUAL,
    COMPARISON_GREATER_EQUAL,
    COMPARISON_ALWAYS,
};

struct SamplerStateDesc
{
    SamplerStateTextureFilterMode filterMode;
    SamplerStateTextureAddressMode addressModeU;
    SamplerStateTextureAddressMode addressModeV;
    SamplerStateTextureAddressMode addressModeW;
    GPUComparisonFunc comparisonFunction;
    u32 maxAnisotropy;
    f32 borderColor[4];
};

// Blend state
enum GPUBlend
{
    BLEND_ZERO,
    BLEND_ONE,
    BLEND_SRC_COLOR,
    BLEND_INV_SRC_COLOR,
    BLEND_SRC_ALPHA,
    BLEND_INV_SRC_ALPHA,
    BLEND_DEST_ALPHA,
    BLEND_INV_DEST_ALPHA,
    BLEND_DEST_COLOR,
    BLEND_INV_DEST_COLOR,
    BLEND_SRC_ALPHA_SAT,
    BLEND_BLEND_FACTOR,
    BLEND_INV_BLEND_FACTOR,
};

enum GPUBlendOP
{
    BLEND_OP_ADD,
    BLEND_OP_SUBTRACT,
    BLEND_OP_REV_SUBTRACT,
    BLEND_OP_MIN,
    BLEND_OP_MAX
};

enum GPUColorWriteEnable
{
    COLOR_WRITE_ENABLE_RED,
    COLOR_WRITE_ENABLE_GREEN,
    COLOR_WRITE_ENABLE_BLUE,
    COLOR_WRITE_ENABLE_ALPHA,
    COLOR_WRITE_ENABLE_ALL
};

struct GPURenderTargetBlendDesc
{
    bool blendEnable = false;
    GPUBlend srcBlend = BLEND_ONE;
    GPUBlend destBlend = BLEND_ZERO;
    GPUBlendOP blendOp = BLEND_OP_ADD;
    GPUBlend srcBlendAlpha = BLEND_ONE;
    GPUBlend destBlendAlpha = BLEND_ZERO;
    GPUBlendOP blendOpAlpha = BLEND_OP_ADD;
    GPUColorWriteEnable renderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
};

struct GPUBlendStateDesc
{
    bool alphaToCoverageEnable = false;
    bool independentBlendEnable = false;
    GPURenderTargetBlendDesc renderTarget[8];
};

// Rasterization state
enum GPUFillMode
{
    FILL_SOLID,
    FILL_WIREFRAME,
};

enum GPUCullMode
{
    CULL_NONE,
    CULL_FRONT,
    CULL_BACK
};

struct GPURasterizerStateDesc
{
    GPUFillMode fillMode = FILL_SOLID;
    GPUCullMode cullMode = CULL_BACK;
    bool frontCounterClockwise = false;
    i32 depthBias = 0;
    f32 depthBiasClamp = 0.0f;
    f32 slopeScaledDepthBias = 0.0f;
    bool depthClipEnable = true;
    bool scissorEnable = false;
    bool multisampleEnable = false;
    bool antialiasedLineEnable = false;
};

// Depth-stencil state

enum GPUDepthWriteMask
{
    DEPTH_WRITE_MASK_ZERO,
    DEPTH_WRITE_MASK_ALL
};

enum GPUStencilOP
{
    STENCIL_OP_KEEP,
    STENCIL_OP_ZERO,
    STENCIL_OP_REPLACE,
    STENCIL_OP_INCR_SAT,
    STENCIL_OP_DECR_SAT,
    STENCIL_OP_INVERT,
    STENCIL_OP_INCR,
    STENCIL_OP_DECR
};

struct GPUDepthStencilOPDesc
{
    GPUStencilOP stencilFailOp = STENCIL_OP_KEEP;
    GPUStencilOP stencilDepthFailOp = STENCIL_OP_KEEP;
    GPUStencilOP stencilPassOp = STENCIL_OP_KEEP;
    GPUComparisonFunc stencilFunc = COMPARISON_ALWAYS;
};

struct GPUDepthStencilStateDesc
{
    bool depthEnable = true;
    GPUDepthWriteMask depthWriteMask = DEPTH_WRITE_MASK_ALL;
    GPUComparisonFunc depthFunc = COMPARISON_LESS;
    bool stencilEnable = false;
    u8 stencilReadMask = 0xFF;
    u8 stencilWriteMask = 0xFF;
    GPUDepthStencilOPDesc frontFace;
    GPUDepthStencilOPDesc backFace;
};

enum GPUInputClassification
{
    PER_VERTEX_DATA,
    PER_INSTANCE_DATA
};

struct GPUVertexInputElement
{
    const char* semanticName;
    u32 semanticIndex;
    DataFormat format;
    u32 inputSlot;
    GPUInputClassification classification;
    u32 instanceStepRate;
};

struct GPUInputLayoutDesc
{
    const GPUVertexInputElement* elements = nullptr;
    u32 elementCount = 0;
};

enum GPUPrimitiveTopology
{
    PRIMITIVE_TOPOLOGY_UNDEFINED,
    PRIMITIVE_TOPOLOGY_POINTLIST,
    PRIMITIVE_TOPOLOGY_LINELIST,
    PRIMITIVE_TOPOLOGY_LINESTRIP,
    PRIMITIVE_TOPOLOGY_TRIANGLELIST,
    PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
};

struct GPUShaderBytecode
{
    u8* shaderBytecode = nullptr;
    u64 bytecodeLength;
};

struct GPUGraphicsPipelineStateDesc
{
    GPUShaderBytecode vertexShader;
    GPUShaderBytecode pixelShader;

    GPUBlendStateDesc blendStateDesc;
    u32 sampleMask = 0xFFFFFFFF;
    GPURasterizerStateDesc rasterizerStateDesc;
    GPUInputLayoutDesc inputLayoutDesc;
    GPUDepthStencilStateDesc depthStencilStateDesc;

    GPUPrimitiveTopology primitiveTopology;
};

struct GPUComputePipelineStateDesc
{
    GPUShaderBytecode computeShader;
};

enum GPUQueryType
{
    QUERY_EVENT,
    QUERY_OCCLUSION,
    QUERY_TIMESTAMP,
    QUERY_TIMESTAMP_DISJOINT,
    QUERY_PIPELINE_STATISTICS,
    QUERY_OCCLUSION_PREDICATE
};

enum GPUQueryFlags : u8
{
    QUERY_MISC_PREDICATEHINT = BIT(1)
};

struct GPUQueryDesc
{
    GPUQueryType queryType;
    u8 flags;
};

struct GPUViewport
{
    f32 topLeftX;
    f32 topLeftY;
    f32 width;
    f32 height;
    f32 minDepth;
    f32 maxDepth;
};

struct GPUBox
{
    u32 left;
    u32 top;
    u32 front;
    u32 right;
    u32 bottom;
    u32 back;
};

struct GPURect
{
    u32 left;
    u32 top;
    u32 right;
    u32 bottom;
};

#define RHI_API_NAME "RHI"

struct ILinearAllocator;
struct PlatformWindow;

struct RHIAPI
{
    void* state;

    void (*Init)(ILinearAllocator* allocator, PlatformWindow* window);

    GPURenderTargetView (*GetBackbufferRTV)();

    void (*Draw)(u32 vertexCount, u32 vertexBaseLocation);
    void (*DrawIndexed)(u32 indexCount, u32 startIndex, u32 baseVertexIndex);
    void (*Dispatch)(u32 threadGroupX, u32 threadGroupY, u32 threadGroupZ);
    void (*Present)();

    GPUBuffer (*CreateBuffer)(const SubresourceData* subresource, u32 size, u32 flags, const char* debugName);
    GPUTexture2D (*CreateTexture2D)(const Texture2DDesc* initParams, const SubresourceData* subresources, const char* debugName);
    GPURenderTargetView (*CreateRenderTargetView)(GPUTexture2D texture, const GPUResourceViewDesc& resourceViewDesc, const char* debugName);
    GPUDepthStencilView (*CreateDepthStencilView)(GPUTexture2D texture, const GPUResourceViewDesc& resourceViewDesc, const char* debugName);
    GPUShaderResourceView (*CreateShaderResourceView)(GPUTexture2D texture, const GPUResourceViewDesc& resourceViewDesc, const char* debugName);
    GPUUnorderedAccessView (*CreateUnorderedAccessView)(GPUTexture2D texture, const GPUResourceViewDesc& resourceViewDesc, const char* debugName);
    GPUSamplerState (*CreateSamplerState)(const SamplerStateDesc* desc, const char* debugName);
    GPUQuery (*CreateQuery)(const GPUQueryDesc& queryDesc, const char* debugName);
    GPUGraphicsPipelineState (*CreateGraphicsPipelineState)(const GPUGraphicsPipelineStateDesc& pipelineDesc, const char* debugName);
    GPUComputePipelineState (*CreateComputePipelineState)(const GPUComputePipelineStateDesc& pipelineDesc, const char* debugName);

    void (*DestroyBuffer)(GPUBuffer buffer);
    void (*DestroyTexture2D)(GPUTexture2D texture);
    void (*DestroyRenderTargetView)(GPURenderTargetView renderTargetView);
    void (*DestroyDepthStencilView)(GPUDepthStencilView depthStencilView);
    void (*DestroyShaderResourceView)(GPUShaderResourceView shaderResourceView);
    void (*DestroyUnorderedAccessView)(GPUUnorderedAccessView unrderedAccessView);
    void (*DestroySamplerState)(GPUSamplerState samplerState);
    void (*DestroyQuery)(GPUQuery query);
    void (*DestroyGraphicsPipelineState)(GPUGraphicsPipelineState graphicsPipelineState);
    void (*DestroyComputePipelineState)(GPUComputePipelineState computePipelineState);

    void (*SetVertexBuffers)(GPUBuffer* vertexBuffers, u32 startIndex, u32 vertexBufferCount, u32* strideByteCounts, u32* offsets);
    void (*SetIndexBuffer)(GPUBuffer indexBuffer, u32 strideByteCount, u32 offset);
    void (*SetRenderTargets)(GPURenderTargetView* renderTargets, u32 renderTargetCount, GPUDepthStencilView depthStencilView);

    void (*SetShaderResourcesVS)(u32 startSlot, GPUShaderResourceView* shaderResources, u32 shaderResourceCount);
    void (*SetShaderResourcesPS)(u32 startSlot, GPUShaderResourceView* shaderResources, u32 shaderResourceCount);
    void (*SetShaderResourcesCS)(u32 startSlot, GPUShaderResourceView* shaderResources, u32 shaderResourceCount);

    void (*SetUnorderedAcessViewsCS)(u32 startSlot, GPUUnorderedAccessView* unorderedAccessViews, u32 uavCount, u32* uavInitialCounts);

    void (*SetSamplerStatesVS)(u32 startSlot, GPUSamplerState* samplerStates, u32 samplerStateCount);
    void (*SetSamplerStatesPS)(u32 startSlot, GPUSamplerState* samplerStates, u32 samplerStateCount);
    void (*SetSamplerStatesCS)(u32 startSlot, GPUSamplerState* samplerStates, u32 samplerStateCount);

    void (*SetGraphicsPipelineState)(GPUGraphicsPipelineState pipelineState);
    void (*SetComputePipelineState)(GPUComputePipelineState pipelineState);

    void (*SetViewports)(const GPUViewport* viewports, u32 viewportCount);
    void (*SetScissorRects)(const GPURect* rects, u32 rectCount);

    void (*SetConstantBuffersVS)(u32 startSlot, GPUBuffer* constantBuffers, u32 count);
    void (*SetConstantBuffersPS)(u32 startSlot, GPUBuffer* constantBuffers, u32 count);
    void (*SetConstantBuffersCS)(u32 startSlot, GPUBuffer* constantBuffers, u32 count);

    void (*ClearRenderTarget)(GPURenderTargetView renderTargetView, f32 color[4]);
    void (*ClearDepthStencilView)(GPUDepthStencilView depthStencilView, bool clearStencil, f32 depthClearData, u8 stencilClearData);

    void (*CopyTexture)(GPUTexture2D source, GPUTexture2D destination);

    void* (*MapBuffer)(GPUBuffer resource);
    void (*UnmapBuffer)(GPUBuffer resource);

    void (*UpdateTextureSubresource)(GPUTexture2D resource, u32 dstSubresource, const GPUBox* updateBox, const SubresourceData& subresourceData);

    void (*MSAAResolve)(GPUTexture2D dest, GPUTexture2D source);
    void (*GenerateMips)(GPUShaderResourceView shaderResourceView);

    void (*BeginEvent)(const char* eventName);
    void (*EndEvent)();

    void (*BeginQuery)(GPUQuery query);
    void (*EndQuery)(GPUQuery query);
    bool (*GetDataQuery)(GPUQuery query, void* outData, u32 size, u32 flags);
};