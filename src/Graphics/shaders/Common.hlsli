#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

// ���̴����� include�� ������� .hlsli ���Ͽ� �ۼ�
// Properties -> Item Type: Does not participate in build���� ����

/* ����: C++ SimpleMath -> HLSL */
// Vector3 -> float3
// float3 a = normalize(b);
// float a = dot(v1, v2);
// Satuarate() -> saturate() ���
// float l = length(v);
// struct A{ float a = 1.0f; }; <- ����ü �ȿ��� �ʱ�ȭ �Ұ�
// Vector3(0.0f) -> float3(0.0f, 0.0f, 0.0f)
// Vector4::Transform(v, M) -> mul(v, M)

#define MAX_LIGHTS 3
#define NUM_DIR_LIGHTS 1
#define NUM_POINT_LIGHTS 1
#define NUM_SPOT_LIGHTS 1

// ����
struct Material
{
    float3 ambient;
    float shininess;
    float3 diffuse;
    float dummy1; // 16 bytes �����ֱ� ���� �߰�
    float3 specular;
    float dummy2;
    float3 fresnelR0;
    float dummy3;
};

// ����
struct Light
{
    float3 strength;
    float fallOffStart;
    float3 direction;
    float fallOffEnd;
    float3 position;
    float spotPower;
};

struct VSInput
{
    float3 posModel : POSITION; //�� ��ǥ���� ��ġ position
    float3 normalModel : NORMAL; // �� ��ǥ���� normal    
    float2 texcoord : TEXCOORD0; // <- ���� �������� ���
    
    // float3 color : COLOR0; <- ���ʿ� (���̵�)
};

struct PSInput
{
    float4 posProj : SV_POSITION; // Screen position
    float3 posWorld : POSITION; // World position (���� ��꿡 ���)
    float3 normalWorld : NORMAL;
    float2 texcoord : TEXCOORD;
    float3 color : COLOR; // Normal lines ���̴����� ���
};

struct PSOutput
{
    float4 pixelColor : SV_Target0;
    float4 indexColor : SV_Target1;
};

#endif // __COMMON_HLSLI__