#include "Common.hlsli"

cbuffer ConstantBuffer : register(b0)
{
    matrix modelMatrix;
    matrix invTranspose;
    matrix viewMatrix;
    matrix projMatrix;
};

PSInput main(VSInput input)
{
    // 모델(Model) 행렬은 모델 자신의 원점에서 
    // 월드 좌표계에서의 위치로 변환을 시켜줌
    // 모델 좌표계의 위치 -> [모델 행렬 곱하기] -> 월드 좌표계의 위치
    // -> [뷰 행렬 곱하기] -> 뷰 좌표계의 위치 -> [프로젝션 행렬 곱하기]
    // -> 스크린 좌표계의 위치
    
    // 뷰 좌표계는 NDC이기 때문에 월드 좌표를 이용해서 조명 계산
    
    PSInput output;
    
    float4 position = float4(input.posModel, 1.0f);
    
    // World 변환
    position = mul(position, modelMatrix);
    output.posWorld = position.xyz;
    
    // View 변환
    position = mul(position, viewMatrix);
    
    // Projection 변환
    position = mul(position, projMatrix);
    output.posProj = position;
    
    output.texcoord = input.texcoord;
    
    // 노멀 변환 (월드 공간으로)
    float4 normal = float4(input.normalModel, 0.0f);
    output.normalWorld = normalize(mul(normal, invTranspose).xyz);
    
    // 노멀을 색상으로 변환 (디버깅용)
    output.color = output.normalWorld * 0.5f + 0.5f;
    
    return output;
}