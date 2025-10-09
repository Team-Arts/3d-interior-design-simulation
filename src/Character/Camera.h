#pragma once

#include <directxtk/SimpleMath.h>

namespace visualnnz
{
using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector3;

class Camera
{
public:
    Camera();
    ~Camera();

    // 기존 메서드들
    void SetPosition(const Vector3 &position);
    void SetTarget(const Vector3 &target);
    void SetUpVector(const Vector3 &up);

    void SetPerspective(float fov, float aspectRatio, float nearPlane, float farPlane);

    Matrix GetViewMatrix() const;
    Matrix GetProjectionMatrix() const;

    Vector3 GetPosition() const { return m_position; }
    Vector3 GetTarget() const { return m_target; }

    // 추가된 메서드들 (자유 이동을 위한)
    void SetRotation(const Vector3 &rotation);
    void Move(const Vector3 &direction);
    void Rotate(const Vector3 &rotation);

    Vector3 GetForward() const;
    Vector3 GetRight() const;
    Vector3 GetUp() const;
    Vector3 GetRotation() const { return m_rotation; }

    // 카메라 모드 설정
    void SetLookAtMode(bool enabled) { m_lookAtMode = enabled; }
    bool IsLookAtMode() const { return m_lookAtMode; }

    // 설정값 접근자
    float GetFOV() const { return m_fov; }
    float GetAspectRatio() const { return m_aspectRatio; }
    float GetNearPlane() const { return m_nearPlane; }
    float GetFarPlane() const { return m_farPlane; }

private:
    void UpdateMatrices() const;
    void ClampPitch();

    Vector3 m_position;
    Vector3 m_target;
    Vector3 m_up;
    Vector3 m_rotation; // Pitch, Yaw, Roll (라디안)

    float m_fov;
    float m_aspectRatio;
    float m_nearPlane;
    float m_farPlane;

    mutable Matrix m_viewMatrix;
    mutable Matrix m_projMatrix;
    mutable bool m_viewDirty;
    mutable bool m_projDirty;

    bool m_lookAtMode; // true: LookAt 모드, false: 자유 이동 모드
};
} // namespace visualnnz