#include "Camera.h"
#include <algorithm>

namespace visualnnz
{
using namespace DirectX::SimpleMath;

Camera::Camera()
    : m_position(0.0f, 0.0f, 0.0f),
      m_target(0.0f, 0.0f, 1.0f),
      m_up(0.0f, 1.0f, 0.0f),
      m_rotation(0.0f, 0.0f, 0.0f),
      m_fov(45.0f * 3.14159f / 180.0f),
      m_aspectRatio(16.0f / 9.0f),
      m_nearPlane(0.1f),
      m_farPlane(100.0f),
      m_viewDirty(true),
      m_projDirty(true),
      m_lookAtMode(true)
{
}

Camera::~Camera() = default;

void Camera::SetPosition(const Vector3 &position)
{
    m_position = position;
    m_viewDirty = true;
}

void Camera::SetTarget(const Vector3 &target)
{
    m_target = target;
    m_viewDirty = true;
}

void Camera::SetUpVector(const Vector3 &up)
{
    m_up = up;
    m_viewDirty = true;
}

void Camera::SetPerspective(float fov, float aspectRatio, float nearPlane, float farPlane)
{
    m_fov = fov;
    m_aspectRatio = aspectRatio;
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
    m_projDirty = true;
}

Matrix Camera::GetViewMatrix() const
{
    if (m_viewDirty)
    {
        UpdateMatrices();
    }
    return m_viewMatrix;
}

Matrix Camera::GetProjectionMatrix() const
{
    if (m_projDirty)
    {
        UpdateMatrices();
    }
    return m_projMatrix;
}

void Camera::SetRotation(const Vector3 &rotation)
{
    m_rotation = rotation;
    ClampPitch();
    m_viewDirty = true;
}

void Camera::Move(const Vector3 &direction)
{
    m_position += direction;
    if (!m_lookAtMode)
    {
        m_target += direction;
    }
    m_viewDirty = true;
}

void Camera::Rotate(const Vector3 &rotation)
{
    m_rotation += rotation;
    ClampPitch();
    m_viewDirty = true;
}

Vector3 Camera::GetForward() const
{
    if (m_lookAtMode)
    {
        Vector3 forward = m_target - m_position;
        forward.Normalize();
        return forward;
    }
    else
    {
        Matrix rotMatrix = Matrix::CreateFromYawPitchRoll(m_rotation.y, m_rotation.x, m_rotation.z);
        return Vector3::Transform(Vector3::Forward, rotMatrix);
    }
}

Vector3 Camera::GetRight() const
{
    Vector3 forward = GetForward();
    Vector3 right = forward.Cross(m_up);
    right.Normalize();
    return right;
}

Vector3 Camera::GetUp() const
{
    Vector3 forward = GetForward();
    Vector3 right = GetRight();
    return right.Cross(forward);
}

void Camera::UpdateMatrices() const
{
    if (m_viewDirty)
    {
        if (m_lookAtMode)
        {
            m_viewMatrix = Matrix::CreateLookAt(m_position, m_target, m_up);
        }
        else
        {
            Matrix rotMatrix = Matrix::CreateFromYawPitchRoll(m_rotation.y, m_rotation.x, m_rotation.z);
            Vector3 forward = Vector3::Transform(Vector3::Forward, rotMatrix);
            m_viewMatrix = Matrix::CreateLookAt(m_position, m_position + forward, m_up);
        }
        m_viewDirty = false;
    }
    
    if (m_projDirty)
    {
        m_projMatrix = Matrix::CreatePerspectiveFieldOfView(m_fov, m_aspectRatio, m_nearPlane, m_farPlane);
        m_projDirty = false;
    }
}

void Camera::ClampPitch()
{
    const float maxPitch = 89.0f * 3.14159f / 180.0f;
    m_rotation.x = std::clamp(m_rotation.x, -maxPitch, maxPitch);
}

} // namespace visualnnz