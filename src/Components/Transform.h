#pragma once

#include <directxtk/SimpleMath.h>
using namespace DirectX::SimpleMath;

class Transform 
{
private:
    Vector3 m_position;
    Vector3 m_rotation; // Euler angles
    Vector3 m_scale;
    Matrix m_worldMatrix;
    bool m_isDirty;

public:
    // OB-002, OB-003���� ����� ��ġ ����
    void SetPosition(const Vector3& position);
    void SetRotation(const Vector3& rotation); // OB-004-1
    void SetScale(const Vector3& scale);       // OB-004-2
    
    // ��� ���
    Matrix GetWorldMatrix();
    void UpdateMatrix();
};
