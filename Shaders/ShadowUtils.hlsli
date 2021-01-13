
inline float PCFShadowMap(float3 shadowCoord, uint cascadeIndex, float bias) {
    float texelSize = 1.0f / g_ShadowMapSize;
    float kernelSize = 3;
    int index = floor(kernelSize / 2);
    float shadowValue = 0;
    float compareValue = shadowCoord.z - bias;
    [unroll]
    for (int i = -index; i <= index; ++i) {
        [unroll]
        for (int j = -index; j <= index; ++j) {
            int2 offset = int2(i, j);
            float sampleCmp = g_ShadowMapTexture.SampleCmpLevelZero(g_ShadowMapSampler, float3(shadowCoord.xy, cascadeIndex), compareValue, offset);
            shadowValue += sampleCmp;
        }
    }
    shadowValue /= (kernelSize * kernelSize);
    return shadowValue;
}

inline float CalculateShadowValue(float3 worldSpacePos, out uint hitCascadeIndex) {
    float shadowFactor = 1;
    float3 visualizeColor = float3(1.0f, 1.0f, 1.0f);
    
    for (uint cascadeIndex = 0; cascadeIndex < g_ShadowCascadeCount; ++cascadeIndex) {
        // ShadowCoordinates
        float4 worldPosLightSpace = mul(g_DirectionLightProjViewMatrices[cascadeIndex], float4(worldSpacePos, 1.0f));
        float3 shadowCoord = worldPosLightSpace.xyz / worldPosLightSpace.w;
        shadowCoord.x = shadowCoord.x * 0.5f + 0.5f;
        shadowCoord.y = shadowCoord.y * -0.5f + 0.5f;

        [branch]
        float bias = 0.0015f;
        if (all(saturate(shadowCoord) == shadowCoord)) {
            bias += (0.0015f * cascadeIndex * cascadeIndex);
            float shadowValue = PCFShadowMap(shadowCoord, cascadeIndex, bias);
            shadowFactor = shadowValue;
            hitCascadeIndex = cascadeIndex;

            // TODO: Blend cascade edges for smooth cascade transition
            break;
        }
    }

    return shadowFactor;
}

static const float3 sampleOffsetDirections[20] = {
    float3(1, 1, 1),  float3(1, -1, 1),  float3(-1, -1, 1),  float3(-1, 1, 1),
    float3(1, 1, -1), float3(1, -1, -1), float3(-1, -1, -1), float3(-1, 1, -1),
    float3(1, 1, 0),  float3(1, -1, 0),  float3(-1, -1, 0),  float3(-1, 1, 0),
    float3(1, 0, 1),  float3(-1, 0, 1),  float3(1, 0, -1),   float3(-1, 0, -1),
    float3(0, 1, 1),  float3(0, -1, 1),  float3(0, -1, -1),  float3(0, 1, -1)
};

inline float PointLightPCFShadowMap(float3 shadowCoord, uint lightIndex, float bias) {
    float n = g_PointLightProjNearPlane;
    float f = g_PointLightProjFarPlane;

    float3 absVec = abs(shadowCoord);
    float viewZ = max(absVec.x, max(absVec.y, absVec.z));

    float projectedZ = (f + n) / (f - n) - ((2 * f * n) / (f - n)) / viewZ;
    projectedZ = (projectedZ + 1.0) * 0.5;
    float compareValue = projectedZ - bias;

    float shadowValue = 0;
    float diskRadius = 0.01f;
    uint sampleCount = 20;
    for (uint i = 0; i < sampleCount; ++i) {
        float3 offset = sampleOffsetDirections[i] * diskRadius;
        float sampleCmp = g_PointLightShadowMapArray.SampleCmpLevelZero(g_ShadowMapSampler, float4(shadowCoord + offset, lightIndex), compareValue);
        shadowValue += sampleCmp;
    }

    return shadowValue / sampleCount;
}

inline float CalculatePointLightShadow(float3 unnormalizedLightVector, uint lightIndex) {
    float bias = 0.005f;

    return PointLightPCFShadowMap(-unnormalizedLightVector, lightIndex, bias);
}

inline float SpotLightPCFShadowMap(float3 shadowCoord, uint lightIndex, float bias) {
    float kernelSize = 3;
    int index = floor(kernelSize / 2);
    float shadowValue = 0;
    float compareValue = shadowCoord.z - bias;
    [unroll]
    for (int i = -index; i <= index; ++i) {
        [unroll]
        for (int j = -index; j <= index; ++j) {
            float2 offset = float2(i, j) * 0.002f;
            float sampleCmp = g_SpotLightShadowMapArray.SampleCmpLevelZero(g_ShadowMapSampler, float3(shadowCoord.xy + offset, lightIndex), compareValue);
            shadowValue += sampleCmp;
        }
    }
    shadowValue /= (kernelSize * kernelSize);
    return shadowValue;
}

inline float CalculateSpotLightShadow(float3 worldSpacePos, uint lightIndex) {
    float bias = 0.001f;

    float4 worldPosLightClipSpace = mul(g_SpotLightProjViewMatrices[lightIndex], float4(worldSpacePos, 1.0f));
    float3 shadowCoord = worldPosLightClipSpace.xyz / worldPosLightClipSpace.w;
    shadowCoord.x = shadowCoord.x * 0.5f + 0.5f;
    shadowCoord.y = shadowCoord.y * -0.5f + 0.5f;

    return SpotLightPCFShadowMap(shadowCoord, lightIndex, bias);
}

