#include "ShaderDefinitions.h"
///////////////////////////////////////////////////////////////////////////
/////// VERTEX SHADER
///////////////////////////////////////////////////////////////////////////

struct VSOut {
    float3 rawPosition : POSITION;
    float4 pos : SV_Position;
};

VSOut VSMain(float3 pos : POSITION)
{
    VSOut output;
    output.rawPosition = pos;
    float3x3 viewWithoutTranslate = (float3x3) g_ViewMatrix;
    float4x4 viewMatrix = float4x4(float4(viewWithoutTranslate[0], 0.0f), float4(viewWithoutTranslate[1], 0.0f),
        float4(viewWithoutTranslate[2], 0.0f), float4(0.0f, 0.0f, 0.0f, 1.0f));
    float4 transformedPos = mul(g_ProjMatrix, mul(viewMatrix, float4(pos, 1.0f)));
    output.pos = transformedPos.xyww;

    return output;
}

///////////////////////////////////////////////////////////////////////////
/////// FRAGMENT SHADER
///////////////////////////////////////////////////////////////////////////
float4 PSMain(VSOut input) : SV_Target
{
    float4 color = g_SkyboxTexture.Sample(g_LinearWrapSampler, input.rawPosition);
    return color;
}