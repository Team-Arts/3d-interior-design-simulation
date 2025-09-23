#pragma once

#include <memory>

#include "AppBase.h"

namespace visualnnz
{
using std::unique_ptr;
using std::vector;
using std::wstring;

// 전방 선언
class Renderer;
// class SceneManager;
// class UIManager;
// class InputManager;

// AppBase를 상속받아 실제 프로그램 로직을 구현하는 클래스
class MainApp : public AppBase
{
public:
    MainApp();
    virtual ~MainApp() override;

    // AppBase 가상 함수 재정의
    virtual bool Initialize(HINSTANCE hInstance, int nCmdShow) override;
    virtual void Update(float deltaTime) override;
    virtual void Render() override;

private:
    // 프로그램이 관리할 핵심 모듈들
    unique_ptr<Renderer> m_renderer;
    // unique_ptr<SceneManager> m_sceneManager;
    // unique_ptr<UIManager> m_uiManager;
    // unique_ptr<InputManager> m_inputManager;

    
};
} // namespace visualnnz
