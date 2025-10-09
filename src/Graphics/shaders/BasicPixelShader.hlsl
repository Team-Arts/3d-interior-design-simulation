#include "Common.hlsli"

/* Texture2D g_texture0 : register(t0);
TextureCube g_diffuseCube : register(t1);
TextureCube g_specularCube : register(t2);
SamplerState g_sampler : register(s0);

cbuffer BasicPixelConstantData : register(b0)
{
    float3 eyeWorld;
    bool useTexture;
    Material material;
    Light light[MAX_LIGHTS];
    float4 indexColor; // 피킹(Picking)에 사용
}; */

float4 main(PSInput input) : SV_TARGET
{
    // 간단한 조명 계산
    float3 lightDir = normalize(float3(1.0f, 1.0f, -1.0f));
    float3 normal = normalize(input.normalWorld);
    
    float diffuse = max(dot(normal, lightDir), 0.2f);
    float3 baseColor = float3(0.8f, 0.6f, 0.4f); // 밝은 갈색
    
    float3 finalColor = baseColor * diffuse;
    
    return float4(finalColor, 1.0f);
}