#include "ShaderDefinitions.h"

///////////////////////////////////////////////////////////////////////////
/////// VERTEX SHADER
///////////////////////////////////////////////////////////////////////////

struct VSOut {
    float2 texCoords : TEXCOORD0;
    float4 pos : SV_POSITION;
};

VSOut VSMain(uint vid : SV_VertexID)
{
    VSOut output;
    output.texCoords = float2((vid << 1) & 2, vid & 2);
    output.pos = float4(output.texCoords * float2(2, -2) + float2(-1, 1), 0, 1);

    return output;
}

///////////////////////////////////////////////////////////////////////////
/////// FRAGMENT SHADER
///////////////////////////////////////////////////////////////////////////
// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
float3 ACESFast(float3 x)
{
    x *= 0.6f;
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

static const float3x3 ACESInputMat =
{
    {0.59719, 0.35458, 0.04823},
    {0.07600, 0.90834, 0.01566},
    {0.02840, 0.13383, 0.83777}
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
static const float3x3 ACESOutputMat =
{
    { 1.60475, -0.53108, -0.07367},
    {-0.10208,  1.10813, -0.00605},
    {-0.00327, -0.07276,  1.07602}
};

float3 RRTAndODTFit(float3 v)
{
    float3 a = v * (v + 0.0245786f) - 0.000090537f;
    float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

float3 ACESFitted(float3 color)
{
    color = mul(ACESInputMat, color);

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = mul(ACESOutputMat, color);

    // Clamp to [0, 1]
    color = saturate(color);

    return color;
}


float4 PSMain(VSOut input) : SV_Target
{
    float3 hdrColor = g_PostProcessTexture0.Load(input.pos.xyz).rgb;
    hdrColor *= g_PPExposure;

    // ACES film curve from https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
    float3 tonemappedColor = ACESFitted(hdrColor);

    // Gamma correction
    float inverseGamma = 1.0f / g_PPGamma;
    tonemappedColor = pow(tonemappedColor, inverseGamma);

    return float4(tonemappedColor, 1.0f);
}
