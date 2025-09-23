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

    // ���콺 �Է� (OB �䱸���׿�)
    bool IsMouseButtonPressed(int button) const;
    bool IsMouseButtonDown(int button) const;
    bool IsMouseButtonReleased(int button) const;

    // Ű���� �Է� (UC-005 ĳ���� �̵���)
    bool IsKeyPressed(int key) const;
    bool IsKeyDown(int key) const;

    // ���콺 ��ġ (OB-001, OB-002��)
    int GetMouseX() const { return m_mouseX; }
    int GetMouseY() const { return m_mouseY; }
    int GetMouseDeltaX() const { return m_mouseX - m_prevMouseX; }
    int GetMouseDeltaY() const { return m_mouseY - m_prevMouseY; }
};
} // namespace visualnnz