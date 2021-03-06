#include "ShaderDefinitions.h"

///////////////////////////////////////////////////////////////////////////
/////// VERTEX SHADER
///////////////////////////////////////////////////////////////////////////
struct VSInput {
    float3 pos : POSITION;
};

struct VSOut {
    float4 pos : SV_Position;
};

VSOut VSMain(VSInput input) {
    VSOut output;
    output.pos = mul(g_ProjViewMatrix, float4(input.pos, 1.0f));

    return output;
}

///////////////////////////////////////////////////////////////////////////
/////// FRAGMENT SHADER
///////////////////////////////////////////////////////////////////////////
float4 PSMain(VSOut input) : SV_Target{
    return float4(1.0f, 1.0f, 0.0f, 1.0f);
}
