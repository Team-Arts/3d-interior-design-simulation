#include "Transform.h"
#include <directxtk/SimpleMath.h>

namespace visualnnz
{
using namespace DirectX::SimpleMath;

Transform::Transform()
    : m_position(0.0f, 0.0f, 0.0f), m_rotation(0.0f, 0.0f, 0.0f), m_scale(1.0f, 1.0f, 1.0f), m_modelMatrix(Matrix::Identity), m_isDirty(true)
{
}

void Transform::SetPosition(const Vector3 &position)
{
    m_position = position;
    m_isDirty = true;
}

void Transform::SetRotation(const Vector3 &rotation)
{
    m_rotation = rotation;
    m_isDirty = true;
}

void Transform::SetScale(const Vector3 &scale)
{
    m_scale = scale;
    m_isDirty = true;
}

void Transform::UpdateMatrix()
{
    if (m_isDirty)
    {
        // 스케일, 회전, 이동 순서로 변환 행렬 생성
        Matrix scaleMatrix = Matrix::CreateScale(m_scale);
        Matrix rotationMatrix = Matrix::CreateFromYawPitchRoll(m_rotation.y, m_rotation.x, m_rotation.z);
        Matrix translationMatrix = Matrix::CreateTranslation(m_position);

        m_modelMatrix = scaleMatrix * rotationMatrix * translationMatrix;
        m_isDirty = false;
    }
}

Matrix Transform::GetModelMatrix()
{
    UpdateMatrix();
    return m_modelMatrix;
}
} // namespace visualnnz