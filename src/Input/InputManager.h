#pragma once
#include <Windows.h>
#include <memory>
#include <directxtk/SimpleMath.h>

namespace visualnnz
{
using std::shared_ptr;

// ���� ����
class Camera;

class InputManager
{
public:
    static InputManager& GetInstance();

    bool Initialize(HWND hWnd);
    void Update(float deltaTime);

    // Ű �Է� Ȯ��
    bool IsKeyPressed(int vkCode) const;
    bool IsKeyDown(int vkCode) const;
    bool IsKeyUp(int vkCode) const;

    // ���콺 �Է�
    void OnMouseMove(int x, int y);
    void SetCamera(shared_ptr<Camera> camera) { m_camera = camera; }

    // ����
    void SetMouseSensitivity(float sensitivity) { m_mouseSensitivity = sensitivity; }
    void SetMoveSpeed(float speed) { m_moveSpeed = speed; }

private:
    InputManager() = default;
    ~InputManager() = default;
    InputManager(const InputManager &) = delete;
    InputManager &operator=(const InputManager &) = delete;

    void ProcessKeyboardInput(float deltaTime);
    void ProcessMouseInput();

    HWND m_hWnd = nullptr;
    shared_ptr<Camera> m_camera;

    // Ű ����
    bool m_keyStates[256] = {false};
    bool m_prevKeyStates[256] = {false};

    // ���콺 ����
    float m_mouseSensitivity = 0.002f;
    float m_moveSpeed = 5.0f;
    int m_lastMouseX = 0;
    int m_lastMouseY = 0;
    bool m_firstMouse = true;

    // �̱��� �ν��Ͻ��� ���� static ��� (private���� �̵�)
    static shared_ptr<InputManager> s_instance;
};
} // namespace visualnnz