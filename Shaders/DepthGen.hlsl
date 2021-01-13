#include "ShaderDefinitions.h"

///////////////////////////////////////////////////////////////////////////
/////// VERTEX SHADER
///////////////////////////////////////////////////////////////////////////

struct VSInput {
    float3 pos : POSITION;
#ifdef ALPHA_TEST
    float2 texCoord : TEXCOORD;
#endif
};

struct VSOut {
#ifdef ALPHA_TEST
    float2 texCoord : TEXCOORD;
#endif
    float4 pos : SV_Position;
};

VSOut VSMain(VSInput input) {
    VSOut output;
#ifdef ALPHA_TEST
    output.texCoord = input.texCoord;
#endif
    output.pos = mul(g_ProjViewMatrix, mul(g_ModelMatrix, float4(input.pos, 1.0f)));
    return output;
}

///////////////////////////////////////////////////////////////////////////
/////// FRAGMENT SHADER
///////////////////////////////////////////////////////////////////////////
#ifdef ALPHA_TEST
void PSMain(VSOut input) {
    float alpha = g_Material.diffuseColor.a;
    if (g_Material.hasAlbedoTexture) {
        alpha = g_AlbedoTexture.SampleLevel(g_LinearWrapSampler, input.texCoord, 0.0f).a;
    }

    clip(alpha - 0.1f);
}
#endif

