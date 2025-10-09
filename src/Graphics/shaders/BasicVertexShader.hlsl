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
    // ��(Model) ����� �� �ڽ��� �������� 
    // ���� ��ǥ�迡���� ��ġ�� ��ȯ�� ������
    // �� ��ǥ���� ��ġ -> [�� ��� ���ϱ�] -> ���� ��ǥ���� ��ġ
    // -> [�� ��� ���ϱ�] -> �� ��ǥ���� ��ġ -> [�������� ��� ���ϱ�]
    // -> ��ũ�� ��ǥ���� ��ġ
    
    // �� ��ǥ��� NDC�̱� ������ ���� ��ǥ�� �̿��ؼ� ���� ���
    
    PSInput output;
    
    float4 position = float4(input.posModel, 1.0f);
    
    // World ��ȯ
    position = mul(position, modelMatrix);
    output.posWorld = position.xyz;
    
    // View ��ȯ
    position = mul(position, viewMatrix);
    
    // Projection ��ȯ
    position = mul(position, projMatrix);
    output.posProj = position;
    
    output.texcoord = input.texcoord;
    
    // ��� ��ȯ (���� ��������)
    float4 normal = float4(input.normalModel, 0.0f);
    output.normalWorld = normalize(mul(normal, invTranspose).xyz);
    
    // ����� �������� ��ȯ (������)
    output.color = output.normalWorld * 0.5f + 0.5f;
    
    return output;
}