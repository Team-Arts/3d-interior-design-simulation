namespace visualnnz
{
using namespace std;

class InputManager
{
private:
    bool m_keys[256];
    bool m_prevKeys[256];
    int m_mouseX, m_mouseY;
    int m_prevMouseX, m_prevMouseY;
    bool m_mouseButtons[3]; // Left, Right, Middle
    bool m_prevMouseButtons[3];

public:
    void Update();

    // 마우스 입력 (OB 요구사항용)
    bool IsMouseButtonPressed(int button) const;
    bool IsMouseButtonDown(int button) const;
    bool IsMouseButtonReleased(int button) const;

    // 키보드 입력 (UC-005 캐릭터 이동용)
    bool IsKeyPressed(int key) const;
    bool IsKeyDown(int key) const;

    // 마우스 위치 (OB-001, OB-002용)
    int GetMouseX() const { return m_mouseX; }
    int GetMouseY() const { return m_mouseY; }
    int GetMouseDeltaX() const { return m_mouseX - m_prevMouseX; }
    int GetMouseDeltaY() const { return m_mouseY - m_prevMouseY; }
};
} // namespace visualnnz