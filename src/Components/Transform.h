#pragma once

#include <directxtk/SimpleMath.h>

namespace visualnnz
{
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Matrix;

class Transform
{
public:
    Transform();

    // OB-002, OB-003에서 필요한 위치 설정
    void SetPosition(const Vector3 &position);
    void SetRotation(const Vector3 &rotation); // OB-004-1
    void SetScale(const Vector3 &scale);       // OB-004-2

    Vector3 GetPosition() const { return m_position; }
    Vector3 GetRotation() const { return m_rotation; }
    Vector3 GetScale() const { return m_scale; }

    // 행렬 관련
    Matrix GetModelMatrix();
    void UpdateMatrix();

private:
    Vector3 m_position;
    Vector3 m_rotation; // Euler angles
    Vector3 m_scale;
    Matrix m_modelMatrix;
    bool m_isDirty;
};

} // namespace visualnnz
