#pragma once

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <iostream>
#include <vector>
#include <memory>
#include <functional>
#include <windows.h>
#include <directxtk/SimpleMath.h>

namespace visualnnz
{
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Ray;
using std::unique_ptr;
using std::shared_ptr;
using std::vector;
using std::string;
using std::function;

// 전방 선언
class UIManager;
class InputManager;
class ModelLoader;
class ObjectPicker;
class Camera;
class Renderer;
class InteriorObject;
class Mesh;
class Shader;

// 애플리케이션의 모든 공통 기능을 담당하는 추상 기반 클래스
class AppBase
{
public:
    AppBase();
    virtual ~AppBase();

    // 메인 루프 실행
    int Run();

    // Win32 윈도우 생성
    virtual bool Initialize(HINSTANCE hInstance, int nCmdShow);

    // Win32 윈도우 닫기
    virtual void Shutdown();

    // MainApp에서 재정의할 순수 가상 함수
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;

    // 다른 모듈에서 윈도우 핸들을 사용할 수 있도록 하는 getter
    HWND GetWindowHandle() const;

    // Win32 메시지 처리 함수 - 모든 메시지 처리를 여기서 총괄
    virtual LRESULT CALLBACK MsgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

protected:
    HWND m_mainWindow;
    HINSTANCE m_hInstance;

    // 핵심 매니저들 - AppBase에서 관리
    unique_ptr<UIManager> m_uiManager;
    InputManager* m_inputManager;  // Singleton 패턴
    unique_ptr<ModelLoader> m_modelLoader;
    unique_ptr<ObjectPicker> m_objectPicker;
    shared_ptr<Camera> m_camera;

    // 씬 오브젝트들
    vector<shared_ptr<InteriorObject>> m_sceneObjects;
    shared_ptr<InteriorObject> m_selectedObject;

    // 드래그 상태
    bool m_isDragging = false;
    Vector3 m_dragOffset;
    int m_lastMouseX = 0;
    int m_lastMouseY = 0;

    // MainApp에서 오버라이드할 수 있는 가상 함수들
    virtual void InitializeManagers();
    virtual void SetupUICallbacks();

    // 이벤트 핸들러들 - MainApp에서 오버라이드 가능
    virtual void OnModelSpawn(const string& modelName);
    virtual void OnObjectDelete(shared_ptr<InteriorObject> object);
    virtual void OnMouseClick(int x, int y);
    virtual void OnMouseMove(int x, int y);
    virtual void OnMouseRelease(int x, int y);
    virtual void OnKeyDown(WPARAM wParam);
    virtual void OnKeyUp(WPARAM wParam);

    // MainApp에서 접근할 수 있는 헬퍼 함수들
    void UpdateSceneObjects(float deltaTime);
    void UpdateUI();

    // MainApp에서 구현해야 하는 가상 함수들
    virtual bool InitializeGraphics() = 0;
    virtual void CreateTestObjects() = 0;

private:
    int m_screenWidth;
    int m_screenHeight;
};

} // namespace visualnnz
