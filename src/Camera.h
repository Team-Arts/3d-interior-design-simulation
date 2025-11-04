#pragma once
#include <d3d11.h>
#include <directxmath.h>
#include <windows.h>

// 전방 선언(forward declaration)
class DummyCharacter;

using namespace DirectX;

enum CameraMode
{
    FREE_CAMERA,
    FIRST_PERSON
};

class Camera
{
public:
    Camera();
    ~Camera() = default;

    // 카메라 설정
    void SetPosition(float x, float y, float z);
    void SetRotation(float pitch, float yaw, float roll);
    void SetFieldOfView(float fov);
    void SetNearPlane(float nearZ);
    void SetFarPlane(float farZ);

    // 카메라 이동
    void MoveForward(float distance);
    void MoveRight(float distance);
    void MoveUp(float distance);
    void Rotate(float pitchDelta, float yawDelta);

    // 카메라 속성 가져오기
    XMFLOAT3 GetPosition() const { return position; }
    XMFLOAT3 GetRotation() const { return rotation; }
    float GetFieldOfView() const { return fieldOfView; }
    float GetNearPlane() const { return nearPlane; }
    float GetFarPlane() const { return farPlane; }

    // 뷰 및 투영 행렬 계산
    XMMATRIX GetViewMatrix() const;
    XMMATRIX GetProjectionMatrix() const;

    // 투영 행렬 설정
    void SetProjection(float fov, float aspectRatio, float nearZ, float farZ);

    // 마우스/키보드 입력 처리
    void ProcessInput(HWND hwnd, float deltaTime);


    // UI에서 카메라 속성 편집
    void RenderUI();

    // 향상된 UI에서 카메라 속성 편집 (추가)
    void RenderEnhancedUI();

    // 카메라 모드 관련
    void SetCameraMode(CameraMode newMode) { mode = newMode; }
    CameraMode GetCameraMode() const { return mode; }
    void SetCharacter(DummyCharacter *character) { this->character = character; }

    // UpdateFirstPersonView 함수 추가
    void UpdateFirstPersonView();

private:
    XMFLOAT3 position; // 카메라 위치
    XMFLOAT3 rotation; // 카메라 회전 (피치, 요, 롤)

    // 투영 행렬 매개변수
    float fieldOfView;
    float screenAspect;
    float nearPlane;
    float farPlane;

    // 마우스 입력 추적
    POINT lastMousePos;
    bool mouseTracking;

    // 카메라 속도 설정
    float moveSpeed;   // 초당 이동 속도
    float rotateSpeed; // 마우스 회전 민감도

    // 1인칭 모드 관련
    CameraMode mode;
    DummyCharacter *character; // 캐릭터 참조
};
