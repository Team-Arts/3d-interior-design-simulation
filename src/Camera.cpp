// Camera.cpp - 카메라 클래스 구현
#include "Camera.h"
#include "DummyCharacter.h" // 여기에 포함
#include "EnhancedUI.h"
#include <algorithm>
#include <imgui.h>
using namespace std;
Camera::Camera() : position(0.0f, 0.0f, -20.0f),
                   rotation(0.0f, 0.0f, 0.0f),
                   fieldOfView(XM_PIDIV4),
                   screenAspect(16.0f / 9.0f),
                   nearPlane(0.1f),
                   farPlane(1000.0f),
                   mouseTracking(false),
                   moveSpeed(10.0f),
                   rotateSpeed(0.1f),
                   mode(FREE_CAMERA),
                   character(nullptr) // 초기화 추가

{
    lastMousePos = {0, 0};
}

void Camera::SetPosition(float x, float y, float z)
{
    position = XMFLOAT3(x, y, z);
}

void Camera::SetRotation(float pitch, float yaw, float roll)
{
    // 피치 값 제한 (-89 ~ 89도)
    pitch = max(-89.0f, min(89.0f, pitch));

    rotation = XMFLOAT3(pitch, yaw, roll);
}

void Camera::SetFieldOfView(float fov)
{
    fieldOfView = fov;
}

void Camera::SetNearPlane(float nearZ)
{
    nearPlane = nearZ;
}

void Camera::SetFarPlane(float farZ)
{
    farPlane = farZ;
}

void Camera::MoveForward(float distance)
{
    // 요(yaw) 각도를 라디안으로 변환
    float yawRadian = XMConvertToRadians(rotation.y);

    // 전진 방향 계산 (요 각도 기준)
    position.x += distance * sin(yawRadian);
    position.z += distance * cos(yawRadian);
}

void Camera::MoveRight(float distance)
{
    // 요(yaw) 각도를 라디안으로 변환
    float yawRadian = XMConvertToRadians(rotation.y);

    // 오른쪽 방향 계산 (요 각도 기준)
    position.x += distance * cos(yawRadian);
    position.z -= distance * sin(yawRadian);
}

void Camera::MoveUp(float distance)
{
    position.y += distance;
}

void Camera::Rotate(float pitchDelta, float yawDelta)
{
    // 현재 회전 값 업데이트
    float newPitch = rotation.x + pitchDelta;
    float newYaw = rotation.y + yawDelta;

    // 피치 각도 제한 (-89 ~ 89도)
    newPitch = max(-89.0f, min(89.0f, newPitch));

    // 회전 값 설정
    SetRotation(newPitch, newYaw, rotation.z);
}

XMMATRIX Camera::GetViewMatrix() const
{
    // 카메라 회전 행렬 계산
    XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(
        XMConvertToRadians(rotation.x),
        XMConvertToRadians(rotation.y),
        XMConvertToRadians(rotation.z));

    // 카메라 위치 벡터
    XMVECTOR positionVector = XMLoadFloat3(&position);

    // 기본 방향 벡터 (z축 양의 방향)
    XMVECTOR defaultForward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

    // 회전된 방향 벡터 계산
    XMVECTOR lookAtVector = XMVector3TransformCoord(defaultForward, rotationMatrix);
    lookAtVector = XMVectorAdd(lookAtVector, positionVector);

    // 상단 벡터 (y축 양의 방향)
    XMVECTOR upVector = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    // 최종 뷰 행렬 계산
    return XMMatrixLookAtLH(positionVector, lookAtVector, upVector);
}

XMMATRIX Camera::GetProjectionMatrix() const
{
    return XMMatrixPerspectiveFovLH(
        fieldOfView,
        screenAspect,
        nearPlane,
        farPlane);
}

void Camera::SetProjection(float fov, float aspectRatio, float nearZ, float farZ)
{
    fieldOfView = fov;
    screenAspect = aspectRatio;
    nearPlane = nearZ;
    farPlane = farZ;
}

void Camera::ProcessInput(HWND hwnd, float deltaTime)
{
    // 윈도우가 활성화된 상태인지 확인
    if (GetActiveWindow() != hwnd && GetForegroundWindow() != hwnd)
    {
        // 윈도우가 비활성화된 상태이면 입력 처리하지 않음
        mouseTracking = false; // 마우스 추적도 중단
        return;
    }

    // 키보드 입력 처리
    bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    float currentSpeed = shiftPressed ? moveSpeed * 2.0f : moveSpeed;
    currentSpeed *= deltaTime; // 프레임 시간에 따른 이동 속도 조정

    if (GetAsyncKeyState('W') & 0x8000)
    {
        MoveForward(currentSpeed);
    }

    if (GetAsyncKeyState('S') & 0x8000)
    {
        MoveForward(-currentSpeed);
    }

    if (GetAsyncKeyState('A') & 0x8000)
    {
        MoveRight(-currentSpeed);
    }

    if (GetAsyncKeyState('D') & 0x8000)
    {
        MoveRight(currentSpeed);
    }

    if (GetAsyncKeyState('Q') & 0x8000)
    {
        MoveUp(currentSpeed);
    }

    if (GetAsyncKeyState('E') & 0x8000)
    {
        MoveUp(-currentSpeed);
    }

    // 마우스 입력 처리
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
    {
        POINT currentMousePos;
        GetCursorPos(&currentMousePos);
        ScreenToClient(hwnd, &currentMousePos);

        if (!mouseTracking)
        {
            lastMousePos = currentMousePos;
            mouseTracking = true;
        }
        else
        {
            float yawDelta = (currentMousePos.x - lastMousePos.x) * rotateSpeed;
            float pitchDelta = (currentMousePos.y - lastMousePos.y) * rotateSpeed;

            Rotate(pitchDelta, yawDelta);
            lastMousePos = currentMousePos;
        }
    }
    else
    {
        mouseTracking = false;
    }
}

void Camera::RenderUI()
{
    if (ImGui::Begin("카메라 설정", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("위치 (X, Y, Z):");
        bool posChanged = false;
        posChanged |= ImGui::SliderFloat("X##pos", &position.x, -50.0f, 50.0f);
        posChanged |= ImGui::SliderFloat("Y##pos", &position.y, -50.0f, 50.0f);
        posChanged |= ImGui::SliderFloat("Z##pos", &position.z, -50.0f, 50.0f);

        ImGui::Text("회전 (피치, 요, 롤):");
        bool rotChanged = false;
        float pitch = rotation.x;
        float yaw = rotation.y;
        float roll = rotation.z;

        rotChanged |= ImGui::SliderFloat("Pitch", &pitch, -89.0f, 89.0f);
        rotChanged |= ImGui::SliderFloat("Yaw", &yaw, -180.0f, 180.0f);
        rotChanged |= ImGui::SliderFloat("Roll", &roll, -180.0f, 180.0f);

        if (rotChanged)
        {
            SetRotation(pitch, yaw, roll);
        }

        ImGui::Separator();
        ImGui::Text("카메라 조작:");
        ImGui::BulletText("WASD: 전후좌우 이동");
        ImGui::BulletText("QE: 상하 이동");
        ImGui::BulletText("Shift: 이동 속도 증가");
        ImGui::BulletText("마우스 우클릭 + 드래그: 회전");

        ImGui::Separator();
        ImGui::SliderFloat("이동 속도", &moveSpeed, 1.0f, 50.0f);
        ImGui::SliderFloat("Rotation Speed", &rotateSpeed, 0.01f, 1.0f);

        if (ImGui::Button("초기 위치로 리셋"))
        {
            SetPosition(0.0f, 0.0f, -20.0f);
            SetRotation(0.0f, 0.0f, 0.0f);
        }
    }
    ImGui::End();
}

// 향상된 UI 렌더링 함수 구현
void Camera::RenderEnhancedUI()
{
    // 카메라 위치
    EnhancedUI::RenderHeader("카메라 위치");

    ImGui::PushItemWidth(-1);

    // position 변수 사용
    XMFLOAT3 pos = position;
    bool positionChanged = false;
    positionChanged |= EnhancedUI::SliderFloat("X##CamPos", &pos.x, -50.0f, 50.0f);
    positionChanged |= EnhancedUI::SliderFloat("Y##CamPos", &pos.y, -50.0f, 50.0f);
    positionChanged |= EnhancedUI::SliderFloat("Z##CamPos", &pos.z, -50.0f, 50.0f);

    ImGui::PopItemWidth();

    if (positionChanged)
    {
        SetPosition(pos.x, pos.y, pos.z);
    }

    // 카메라 회전
    EnhancedUI::RenderHeader("카메라 회전");

    ImGui::PushItemWidth(-1);

    // rotation 변수 사용
    XMFLOAT3 rot = rotation;
    bool rotationChanged = false;
    rotationChanged |= EnhancedUI::SliderFloat("X (피치)##CamRot", &rot.x, -80.0f, 80.0f, "위/아래 회전 (고개 끄덕임)");
    rotationChanged |= EnhancedUI::SliderFloat("Y (요)##CamRot", &rot.y, -180.0f, 180.0f, "좌/우 회전 (고개 흔듦)");
    rotationChanged |= EnhancedUI::SliderFloat("Z (롤)##CamRot", &rot.z, -45.0f, 45.0f, "틸트 회전 (고개 기울임)");

    ImGui::PopItemWidth();

    if (rotationChanged)
    {
        SetRotation(rot.x, rot.y, rot.z);
    }

    // 카메라 이동 속도
    EnhancedUI::SliderFloat("이동 속도", &moveSpeed, 1.0f, 20.0f, "카메라 이동 속도");

    // 카메라 회전 속도
    EnhancedUI::SliderFloat("회전 속도", &rotateSpeed, 0.1f, 5.0f, "카메라 회전 속도");

    // 카메라 프리셋
    ImGui::Spacing();
    ImGui::BeginGroup();

    if (ImGui::Button("정면 뷰", ImVec2(80, 0)))
    {
        SetPosition(0.0f, 0.0f, -20.0f);
        SetRotation(0.0f, 0.0f, 0.0f);
    }
    ImGui::SameLine();

    if (ImGui::Button("옆면 뷰", ImVec2(80, 0)))
    {
        SetPosition(20.0f, 0.0f, 0.0f);
        SetRotation(0.0f, -90.0f, 0.0f);
    }
    ImGui::SameLine();

    if (ImGui::Button("윗면 뷰", ImVec2(80, 0)))
    {
        SetPosition(0.0f, 40.0f, 0.0f);
        SetRotation(90.0f, 0.0f, 0.0f);
    }

    ImGui::EndGroup();

    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "키보드: WASD 이동, QE 상하 이동");
}
void Camera::UpdateFirstPersonView()
{
    if (character == nullptr)
    {
        OutputDebugStringA("UpdateFirstPersonView: character is NULL\n");
        return;
    }

    try
    {
        // 캐릭터 1인칭 시점 위치와 회전 가져오기
        XMFLOAT3 charPos = character->GetFirstPersonCameraPosition();
        XMFLOAT3 charRot = character->GetFirstPersonCameraRotation();

        // 카메라 위치와 회전 설정
        position = charPos;
        rotation = charRot;
    }
    catch (std::exception &e)
    {
        OutputDebugStringA(("UpdateFirstPersonView 예외: " + std::string(e.what()) + "\n").c_str());
    }
    catch (...)
    {
        OutputDebugStringA("UpdateFirstPersonView: 알 수 없는 예외 발생\n");
    }
}
