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

// ���� ����
class UIManager;
class InputManager;
class ModelLoader;
class ObjectPicker;
class Camera;
class Renderer;
class InteriorObject;
class Mesh;
class Shader;

// ���ø����̼��� ��� ���� ����� ����ϴ� �߻� ��� Ŭ����
class AppBase
{
public:
    AppBase();
    virtual ~AppBase();

    // ���� ���� ����
    int Run();

    // Win32 ������ ����
    virtual bool Initialize(HINSTANCE hInstance, int nCmdShow);

    // Win32 ������ �ݱ�
    virtual void Shutdown();

    // MainApp���� �������� ���� ���� �Լ�
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;

    // �ٸ� ��⿡�� ������ �ڵ��� ����� �� �ֵ��� �ϴ� getter
    HWND GetWindowHandle() const;

    // Win32 �޽��� ó�� �Լ� - ��� �޽��� ó���� ���⼭ �Ѱ�
    virtual LRESULT CALLBACK MsgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

protected:
    HWND m_mainWindow;
    HINSTANCE m_hInstance;

    // �ٽ� �Ŵ����� - AppBase���� ����
    unique_ptr<UIManager> m_uiManager;
    InputManager* m_inputManager;  // Singleton ����
    unique_ptr<ModelLoader> m_modelLoader;
    unique_ptr<ObjectPicker> m_objectPicker;
    shared_ptr<Camera> m_camera;

    // �� ������Ʈ��
    vector<shared_ptr<InteriorObject>> m_sceneObjects;
    shared_ptr<InteriorObject> m_selectedObject;

    // �巡�� ����
    bool m_isDragging = false;
    Vector3 m_dragOffset;
    int m_lastMouseX = 0;
    int m_lastMouseY = 0;

    // MainApp���� �������̵��� �� �ִ� ���� �Լ���
    virtual void InitializeManagers();
    virtual void SetupUICallbacks();

    // �̺�Ʈ �ڵ鷯�� - MainApp���� �������̵� ����
    virtual void OnModelSpawn(const string& modelName);
    virtual void OnObjectDelete(shared_ptr<InteriorObject> object);
    virtual void OnMouseClick(int x, int y);
    virtual void OnMouseMove(int x, int y);
    virtual void OnMouseRelease(int x, int y);
    virtual void OnKeyDown(WPARAM wParam);
    virtual void OnKeyUp(WPARAM wParam);

    // MainApp���� ������ �� �ִ� ���� �Լ���
    void UpdateSceneObjects(float deltaTime);
    void UpdateUI();

    // MainApp���� �����ؾ� �ϴ� ���� �Լ���
    virtual bool InitializeGraphics() = 0;
    virtual void CreateTestObjects() = 0;

private:
    int m_screenWidth;
    int m_screenHeight;
};

} // namespace visualnnz
