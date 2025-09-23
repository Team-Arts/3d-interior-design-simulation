#include "MainApp.h"
#include "../Graphics/Renderer.h" // 추가: Renderer의 완전한 정의를 포함
//#include "InputManager.h"
//#include "SceneManager.h"
//#include "UIManager.h"

#include <memory>

namespace visualnnz
{
using namespace std;

// 생성자 (이제 멤버 초기화 리스트에서 초기화할 필요 없음)
MainApp::MainApp() = default;

// 소멸자 (unique_ptr이 자동으로 delete를 호출하므로 내용이 비게 됨)
MainApp::~MainApp() = default;

bool MainApp::Initialize(HINSTANCE hInstance, int nCmdShow)
{
    // 메인 윈도우 생성 
    if (!AppBase::Initialize(hInstance, nCmdShow))
        return false;

    // 렌더러 초기화(Direct3D 설정 포함)
    m_renderer = make_unique<Renderer>();
    if (!m_renderer->Initialize(m_mainWindow))
        return false;

    // 각 매니저 초기화
    // m_sceneManager = make_unique<SceneManager>();
    // m_uiManager = make_unique<UIManager>();
    // m_inputManager = make_unique<InputManager>();

    return true;
}

void MainApp::Update(float deltaTime)
{
    // 각 모듈 업데이트
}

void MainApp::Render()
{
    // 렌더링 시작
    m_renderer->BeginScene(0.1f, 0.1f, 0.1f, 1.0f);

    // 씬 렌더링

    // UI 렌더링

    // 렌더링 종료
    m_renderer->EndScene();
}
// ...
} // namespace visualnnz