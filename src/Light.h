#pragma once
#include <directxmath.h>
#include <d3d11.h>

using namespace DirectX;

// 조명 타입 열거형
enum LightType {
    LIGHT_DIRECTIONAL,
    LIGHT_POINT,
    LIGHT_SPOT
};

// 조명 구조체 (셰이더에 전달될 형식과 일치해야 함) 
struct LightData {
    XMFLOAT4 Position;       // w 컴포넌트는 조명 타입(0: 방향성, 1: 점, 2: 스포트라이트)
    XMFLOAT4 Direction;      // 방향성 및 스포트라이트에 사용
    XMFLOAT4 Color;          // RGB 색상 및 강도
    XMFLOAT4 Factors;        // x: 반경, y: 감쇠 계수, z: 스포트 내각, w: 스포트 외각
};

// 조명 클래스
class Light {
public:
    Light(LightType type = LIGHT_DIRECTIONAL);
    ~Light() = default;

    // 설정 함수들
    void SetPosition(float x, float y, float z);
    void SetDirection(float x, float y, float z);
    void SetColor(float r, float g, float b);
    void SetIntensity(float intensity);
    void SetRange(float range);             // 점 조명 및 스포트라이트용
    void SetAttenuation(float attenuation); // 점 조명 및 스포트라이트용
    void SetSpotAngles(float innerAngle, float outerAngle); // 스포트라이트용

    // 게터 함수들
    XMFLOAT3 GetPosition() const;
    XMFLOAT3 GetDirection() const;
    XMFLOAT3 GetColor() const;
    float GetIntensity() const;
    float GetRange() const;
    float GetAttenuation() const;
    LightType GetType() const;

    // 조명 데이터 얻기
    LightData GetLightData() const;

    // UI 렌더링
    void RenderUI(int lightIndex);

private:
    LightType type;
    XMFLOAT3 position;
    XMFLOAT3 direction;
    XMFLOAT3 color;
    float intensity;
    float range;
    float attenuation;
    float spotInnerAngle;
    float spotOuterAngle;
};
