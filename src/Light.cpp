#include "Light.h"
#include "EnhancedUI.h"
#include <imgui.h>

Light::Light(LightType type)
    : type(type),
      position(0.0f, 5.0f, 0.0f),
      direction(0.0f, -1.0f, 0.0f),
      color(1.0f, 1.0f, 1.0f),
      intensity(1.0f),
      range(10.0f),
      attenuation(1.0f),
      spotInnerAngle(XM_PIDIV4),     // 45도
      spotOuterAngle((XM_PI / 3.0f)) // 60도
{
    // 조명 타입에 따른 기본값 조정
    if (type == LIGHT_DIRECTIONAL)
    {
        // 방향성 조명은 위에서 아래로 비추도록
        direction = XMFLOAT3(0.5f, -1.0f, 0.5f);
    }
    else if (type == LIGHT_POINT)
    {
        // 점 조명은 중앙 위쪽에 위치
        position = XMFLOAT3(0.0f, 5.0f, 0.0f);
    }
    else if (type == LIGHT_SPOT)
    {
        // 스포트라이트는 모서리에서 중앙을 비추도록
        position = XMFLOAT3(5.0f, 5.0f, 5.0f);
        direction = XMFLOAT3(-0.5f, -0.5f, -0.5f);
    }
}

// 설정 함수들
void Light::SetPosition(float x, float y, float z)
{
    position = XMFLOAT3(x, y, z);
}

void Light::SetDirection(float x, float y, float z)
{
    // 정규화된 방향 벡터 저장
    XMVECTOR dir = XMVector3Normalize(XMVectorSet(x, y, z, 0.0f));
    XMStoreFloat3(&direction, dir);
}

void Light::SetColor(float r, float g, float b)
{
    color = XMFLOAT3(r, g, b);
}

void Light::SetIntensity(float value)
{
    intensity = value;
}

void Light::SetRange(float value)
{
    range = value;
}

void Light::SetAttenuation(float value)
{
    attenuation = value;
}

void Light::SetSpotAngles(float innerAngle, float outerAngle)
{
    spotInnerAngle = innerAngle;
    spotOuterAngle = outerAngle;
}

// 게터 함수들
XMFLOAT3 Light::GetPosition() const
{
    return position;
}

XMFLOAT3 Light::GetDirection() const
{
    return direction;
}

XMFLOAT3 Light::GetColor() const
{
    return color;
}

float Light::GetIntensity() const
{
    return intensity;
}

float Light::GetRange() const
{
    return range;
}

float Light::GetAttenuation() const
{
    return attenuation;
}

LightType Light::GetType() const
{
    return type;
}

// 조명 데이터 얻기
LightData Light::GetLightData() const
{
    LightData data;

    // 위치 및 타입
    data.Position = XMFLOAT4(position.x, position.y, position.z, static_cast<float>(type));

    // 방향 (정규화)
    data.Direction = XMFLOAT4(direction.x, direction.y, direction.z, 0.0f);

    // 색상 및 강도
    data.Color = XMFLOAT4(color.x, color.y, color.z, intensity);

    // 기타 요소들
    data.Factors = XMFLOAT4(
        range,          // x: 반경
        attenuation,    // y: 감쇠 계수
        spotInnerAngle, // z: 스포트 내각
        spotOuterAngle  // w: 스포트 외각
    );

    return data;
}

// UI 렌더링
void Light::RenderUI(int lightIndex)
{
    // 조명 UI를 구현합니다.
    std::string headerTitle = "조명 #" + std::to_string(lightIndex + 1);
    std::string id = "##light" + std::to_string(lightIndex);

    ImGui::PushID(lightIndex);

    // 조명 타입 선택
    const char *lightTypeNames[] = {"방향성 조명", "점 조명", "스포트라이트"};
    int currentType = static_cast<int>(type);

    if (ImGui::Combo(("조명 유형" + id).c_str(), &currentType, lightTypeNames, IM_ARRAYSIZE(lightTypeNames)))
    {
        type = static_cast<LightType>(currentType);
    }

    // 색상 및 강도
    float colorArray[3] = {color.x, color.y, color.z};
    if (ImGui::ColorEdit3(("색상" + id).c_str(), colorArray))
    {
        color = XMFLOAT3(colorArray[0], colorArray[1], colorArray[2]);
    }

    ImGui::SliderFloat(("강도" + id).c_str(), &intensity, 0.0f, 5.0f, "%.2f");

    // 조명 타입별 속성
    if (type != LIGHT_DIRECTIONAL)
    {
        // 점 조명 및 스포트라이트의 위치
        float pos[3] = {position.x, position.y, position.z};
        if (ImGui::DragFloat3(("위치" + id).c_str(), pos, 0.1f))
        {
            position = XMFLOAT3(pos[0], pos[1], pos[2]);
        }

        // 점 조명 및 스포트라이트의 범위
        ImGui::SliderFloat(("범위" + id).c_str(), &range, 1.0f, 50.0f, "%.1f");
        ImGui::SliderFloat(("감쇠" + id).c_str(), &attenuation, 0.0f, 2.0f, "%.2f");
    }

    if (type == LIGHT_DIRECTIONAL || type == LIGHT_SPOT)
    {
        // 방향성 조명 및 스포트라이트의 방향
        float dir[3] = {direction.x, direction.y, direction.z};
        if (ImGui::DragFloat3(("방향" + id).c_str(), dir, 0.1f))
        {
            // 정규화 필요
            SetDirection(dir[0], dir[1], dir[2]);
        }
    }

    if (type == LIGHT_SPOT)
    {
        // 스포트라이트의 각도 (라디안 -> 도)
        float innerAngleDegrees = XMConvertToDegrees(spotInnerAngle);
        float outerAngleDegrees = XMConvertToDegrees(spotOuterAngle);

        if (ImGui::SliderFloat(("내각(도)" + id).c_str(), &innerAngleDegrees, 1.0f, 90.0f, "%.1f°"))
        {
            spotInnerAngle = XMConvertToRadians(innerAngleDegrees);
            // 내각은 항상 외각보다 작아야 함
            if (spotInnerAngle > spotOuterAngle)
            {
                spotOuterAngle = spotInnerAngle;
                outerAngleDegrees = innerAngleDegrees;
            }
        }

        if (ImGui::SliderFloat(("외각(도)" + id).c_str(), &outerAngleDegrees, 1.0f, 90.0f, "%.1f°"))
        {
            spotOuterAngle = XMConvertToRadians(outerAngleDegrees);
            // 외각은 항상 내각보다 커야 함
            if (spotOuterAngle < spotInnerAngle)
            {
                spotInnerAngle = spotOuterAngle;
            }
        }
    }

    ImGui::PopID();
}
