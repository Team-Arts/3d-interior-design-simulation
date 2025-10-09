#include "InputManager.h"
#include "../Character/Camera.h"
#include <iostream>

namespace visualnnz
{
using namespace std;
using namespace DirectX::SimpleMath;

InputManager& InputManager::GetInstance()
{
    static InputManager instance;
    return instance;
}

bool InputManager::Initialize(HWND hWnd)
{
    m_hWnd = hWnd;
    
    // Ű ���� �ʱ�ȭ
    for (int i = 0; i < 256; ++i)
    {
        m_keyStates[i] = false;
        m_prevKeyStates[i] = false;
    }
    
    return true;
}

void InputManager::Update(float deltaTime)
{
    // ���� ������ Ű ���� ����
    for (int i = 0; i < 256; ++i)
    {
        m_prevKeyStates[i] = m_keyStates[i];
    }
    
    // ���� Ű ���� ������Ʈ
    for (int i = 0; i < 256; ++i)
    {
        m_keyStates[i] = (GetAsyncKeyState(i) & 0x8000) != 0;
    }
    
    // Ű���� �Է� ó��
    ProcessKeyboardInput(deltaTime);
}

bool InputManager::IsKeyPressed(int vkCode) const
{
    return m_keyStates[vkCode] && !m_prevKeyStates[vkCode];
}

bool InputManager::IsKeyDown(int vkCode) const
{
    return m_keyStates[vkCode];
}

bool InputManager::IsKeyUp(int vkCode) const
{
    return !m_keyStates[vkCode] && m_prevKeyStates[vkCode];
}

void InputManager::OnMouseMove(int x, int y)
{
    if (m_firstMouse)
    {
        m_lastMouseX = x;
        m_lastMouseY = y;
        m_firstMouse = false;
        return;
    }
    
    float deltaX = static_cast<float>(x - m_lastMouseX);
    float deltaY = static_cast<float>(y - m_lastMouseY);
    
    m_lastMouseX = x;
    m_lastMouseY = y;
    
    // ī�޶� ȸ�� ó��
    if (m_camera && (m_keyStates[VK_RBUTTON] || m_keyStates[VK_LBUTTON]))
    {
        Vector3 rotation = m_camera->GetRotation();
        rotation.x -= deltaY * m_mouseSensitivity;
        rotation.y += deltaX * m_mouseSensitivity;
        m_camera->SetRotation(rotation);
    }
}

void InputManager::ProcessKeyboardInput(float deltaTime)
{
    if (!m_camera) return;
    
    Vector3 movement = Vector3::Zero;
    
    // WASD �̵�
    if (m_keyStates['W']) movement += m_camera->GetForward();
    if (m_keyStates['S']) movement -= m_camera->GetForward();
    if (m_keyStates['A']) movement -= m_camera->GetRight();
    if (m_keyStates['D']) movement += m_camera->GetRight();
    if (m_keyStates['Q']) movement += m_camera->GetUp();
    if (m_keyStates['E']) movement -= m_camera->GetUp();
    
    if (movement.Length() > 0)
    {
        movement.Normalize();
        movement *= m_moveSpeed * deltaTime;
        m_camera->Move(movement);
    }
}

void InputManager::ProcessMouseInput()
{
    // ���콺 �Է� ó�� (�ʿ�� ����)
}

} // namespace visualnnz