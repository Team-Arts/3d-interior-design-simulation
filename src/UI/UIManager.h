#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <d3d11.h>
#include <wrl.h>

// ImGui ���
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

namespace visualnnz
{
using Microsoft::WRL::ComPtr;
using std::string;
using std::vector;
using std::function;
using std::shared_ptr;

class InteriorObject;
class ModelLoader;

class UIManager
{
public:
    UIManager();
    ~UIManager();

    bool Initialize(HWND hWnd, ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context);
    void Shutdown();

    // �� ������ UI ������Ʈ
    void BeginFrame();
    void Render();
    void EndFrame();

    // UI �̺�Ʈ �ݹ� ����
    void SetOnModelSpawnCallback(function<void(const string&)> callback) 
    { 
        m_onModelSpawn = callback; 
    }
    
    void SetOnObjectDeleteCallback(function<void(shared_ptr<InteriorObject>)> callback) 
    { 
        m_onObjectDelete = callback; 
    }

    // ���� ������Ʈ ��� ������Ʈ
    void UpdateObjectList(const vector<shared_ptr<InteriorObject>>& objects);
    
    // ���õ� ������Ʈ ����
    void SetSelectedObject(shared_ptr<InteriorObject> object);

private:
    void RenderMainMenuBar();
    void RenderObjectBrowser();
    void RenderSceneHierarchy();
    void RenderInspector();
    void RenderToolbar();
    void RenderStatusWindow();

    // UI ����
    bool m_showObjectBrowser = true;
    bool m_showSceneHierarchy = true;
    bool m_showInspector = true;
    bool m_showDemo = false;

    // ������
    vector<string> m_availableModels;
    vector<shared_ptr<InteriorObject>> m_sceneObjects;
    shared_ptr<InteriorObject> m_selectedObject;

    // �ݹ� �Լ���
    function<void(const string&)> m_onModelSpawn;
    function<void(shared_ptr<InteriorObject>)> m_onObjectDelete;

    // ModelLoader ����
    shared_ptr<ModelLoader> m_modelLoader;
};

} // namespace visualnnz