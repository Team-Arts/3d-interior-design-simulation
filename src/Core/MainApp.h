#pragma once

#include <memory>

#include "AppBase.h"

namespace visualnnz
{
using std::unique_ptr;
using std::vector;
using std::wstring;

// ���� ����
class Renderer;
// class SceneManager;
// class UIManager;
// class InputManager;

// AppBase�� ��ӹ޾� ���� ���α׷� ������ �����ϴ� Ŭ����
class MainApp : public AppBase
{
public:
    MainApp();
    virtual ~MainApp() override;

    // AppBase ���� �Լ� ������
    virtual bool Initialize(HINSTANCE hInstance, int nCmdShow) override;
    virtual void Update(float deltaTime) override;
    virtual void Render() override;

private:
    // ���α׷��� ������ �ٽ� ����
    unique_ptr<Renderer> m_renderer;
    // unique_ptr<SceneManager> m_sceneManager;
    // unique_ptr<UIManager> m_uiManager;
    // unique_ptr<InputManager> m_inputManager;

    
};
} // namespace visualnnz
