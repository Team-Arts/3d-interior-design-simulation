#include "MainApp.h"
#include "../Graphics/Renderer.h" // �߰�: Renderer�� ������ ���Ǹ� ����
//#include "InputManager.h"
//#include "SceneManager.h"
//#include "UIManager.h"

#include <memory>

namespace visualnnz
{
using namespace std;

// ������ (���� ��� �ʱ�ȭ ����Ʈ���� �ʱ�ȭ�� �ʿ� ����)
MainApp::MainApp() = default;

// �Ҹ��� (unique_ptr�� �ڵ����� delete�� ȣ���ϹǷ� ������ ��� ��)
MainApp::~MainApp() = default;

bool MainApp::Initialize(HINSTANCE hInstance, int nCmdShow)
{
    // ���� ������ ���� 
    if (!AppBase::Initialize(hInstance, nCmdShow))
        return false;

    // ������ �ʱ�ȭ(Direct3D ���� ����)
    m_renderer = make_unique<Renderer>();
    if (!m_renderer->Initialize(m_mainWindow))
        return false;

    // �� �Ŵ��� �ʱ�ȭ
    // m_sceneManager = make_unique<SceneManager>();
    // m_uiManager = make_unique<UIManager>();
    // m_inputManager = make_unique<InputManager>();

    return true;
}

void MainApp::Update(float deltaTime)
{
    // �� ��� ������Ʈ
}

void MainApp::Render()
{
    // ������ ����
    m_renderer->BeginScene(0.1f, 0.1f, 0.1f, 1.0f);

    // �� ������

    // UI ������

    // ������ ����
    m_renderer->EndScene();
}
// ...
} // namespace visualnnz