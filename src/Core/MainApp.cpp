#include "MainApp.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/Mesh.h"
#include "../Graphics/Shader.h"
#include "../Scene/InteriorObject.h"
#include "../Components/MeshRenderer.h"
#include "../Components/Transform.h"
#include "../UI/UIManager.h"
#include "../Graphics/ModelLoader.h"

#include <memory>
#include <iostream>

namespace visualnnz
{
using namespace std;
using namespace DirectX::SimpleMath;

MainApp::MainApp()
{
    m_renderer = make_unique<Renderer>();
    m_testCube = nullptr;
    m_cubeMesh = nullptr;
    m_basicShader = nullptr;
    
    cout << "MainApp constructor completed" << endl;
}

MainApp::~MainApp() = default;

bool MainApp::Initialize(HINSTANCE hInstance, int nCmdShow)
{
    cout << "MainApp::Initialize starting..." << endl;
    
    // 부모 클래스 초기화 (윈도우 생성)
    if (!AppBase::Initialize(hInstance, nCmdShow))
    {
        cout << "Failed to initialize AppBase!" << endl;
        return false;
    }

    // 그래픽스 초기화
    if (!InitializeGraphics())
    {
        cout << "Failed to initialize graphics!" << endl;
        return false;
    }

    // 매니저들 초기화
    InitializeManagers();

    // UI 콜백 설정
    SetupUICallbacks();

    // 테스트 오브젝트들 생성
    CreateTestObjects();
    
    cout << "MainApp initialized successfully!" << endl;
    return true;
}

bool MainApp::InitializeGraphics()
{
    cout << "Initializing graphics..." << endl;
    
    // 렌더러 초기화(Direct3D 설정 포함)
    if (!m_renderer->Initialize(m_mainWindow))
    {
        cout << "Failed to initialize renderer!" << endl;
        return false;
    }
    
    cout << "Graphics initialized successfully!" << endl;
    return true;
}

void MainApp::InitializeManagers()
{
    cout << "MainApp::InitializeManagers starting..." << endl;
    
    // 부모 클래스의 매니저 초기화 먼저 호출
    AppBase::InitializeManagers();

    // UIManager 초기화 (렌더러가 필요함)
    if (!m_uiManager->Initialize(m_mainWindow, m_renderer->GetDevice(), m_renderer->GetDeviceContext()))
    {
        cout << "Failed to initialize UIManager!" << endl;
        return;
    }

    cout << "MainApp managers initialized successfully!" << endl;
}

void MainApp::CreateTestObjects()
{
    cout << "Creating test objects..." << endl;
    
    // 1. 기본 셰이더 생성
    m_basicShader = make_shared<Shader>();
    wstring vsPath = L"src/Graphics/shaders/BasicVertexShader.hlsl";
    wstring psPath = L"src/Graphics/shaders/BasicPixelShader.hlsl";
    
    if (!m_basicShader->Initialize(m_renderer->GetDevice(), vsPath, psPath))
    {
        cout << "Failed to create basic shader!" << endl;
        return;
    }

    // 2. 큐브 메시 생성
    m_cubeMesh = make_shared<Mesh>();
    auto vertices = Mesh::CreateCubeVertices();
    auto indices = Mesh::CreateCubeIndices();
    
    if (!m_cubeMesh->Initialize(m_renderer->GetDevice(), vertices, indices))
    {
        cout << "Failed to create cube mesh!" << endl;
        return;
    }

    // 3. 테스트 큐브 오브젝트 생성
    m_testCube = make_unique<InteriorObject>();
    
    if (m_testCube->Initialize(m_renderer->GetDevice(), "TestCube", "cube"))
    {
        // MeshRenderer에 메시와 셰이더 설정
        m_testCube->GetMeshRenderer()->SetMesh(m_cubeMesh);
        m_testCube->GetMeshRenderer()->SetShader(m_basicShader);
        
        // Transform 설정
        m_testCube->GetTransform()->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
        m_testCube->GetTransform()->SetScale(Vector3(1.0f, 1.0f, 1.0f));
        
        // 씬 오브젝트 목록에 추가 (AppBase의 m_sceneObjects 사용)
        auto sharedCube = shared_ptr<InteriorObject>(m_testCube.get(), [](InteriorObject*){});
        m_sceneObjects.push_back(sharedCube);
        
        cout << "Test cube created successfully!" << endl;
    }
    else
    {
        cout << "Failed to initialize test cube!" << endl;
    }
}

void MainApp::Update(float deltaTime)
{
    // AppBase에서 이미 씬 오브젝트 업데이트와 UI 업데이트를 처리하므로
    // MainApp에서는 추가적인 로직만 구현
    
    // 큐브 회전 애니메이션 (선택되지 않았을 때만)
    if (m_testCube && !m_testCube->IsSelected())
    {
        static float rotation = 0.0f;
        rotation += deltaTime * 0.5f; // 느린 회전
        m_testCube->GetTransform()->SetRotation(Vector3(0.0f, rotation, 0.0f));
    }
}

void MainApp::Render()
{
    // 3D 씬 렌더링
    float clearColor[4] = {0.2f, 0.2f, 0.2f, 1.0f};
    m_renderer->BeginScene(clearColor);

    // 씬 오브젝트들 렌더링 (AppBase의 m_sceneObjects 사용)
    for (auto& obj : m_sceneObjects)
    {
        if (obj)
        {
            m_renderer->Draw(obj.get());
        }
    }

    m_renderer->EndScene();

    // UI 렌더링
    if (m_uiManager)
    {
        m_uiManager->BeginFrame();
        m_uiManager->Render();
        m_uiManager->EndFrame();
    }
}

void MainApp::OnModelSpawn(const string& modelName)
{
    cout << "MainApp::OnModelSpawn - " << modelName << endl;
    
    string filePath = "asset/models/" + modelName;
    string objectID = "Model_" + to_string(m_sceneObjects.size());
    
    // GLB 파일 로드 시도
    auto newObject = m_modelLoader->LoadModel(m_renderer->GetDevice(), filePath, objectID);
    if (newObject)
    {
        // 기본 셰이더 설정
        newObject->GetMeshRenderer()->SetShader(m_basicShader);
        
        // 기본 위치 설정 (기존 오브젝트들과 겹치지 않도록)
        float offsetX = (m_sceneObjects.size() % 3) * 2.0f - 2.0f;
        float offsetZ = (m_sceneObjects.size() / 3) * 2.0f;
        newObject->GetTransform()->SetPosition(Vector3(offsetX, 0.0f, offsetZ));
        
        m_sceneObjects.push_back(newObject);
        cout << "Model loaded successfully: " << modelName << endl;
    }
    else
    {
        cout << "Failed to load model: " << modelName << " - Creating test cube instead" << endl;
        
        // 테스트용으로 큐브 생성
        auto testObject = make_shared<InteriorObject>();
        if (testObject->Initialize(m_renderer->GetDevice(), objectID, "test_cube"))
        {
            testObject->GetMeshRenderer()->SetMesh(m_cubeMesh);
            testObject->GetMeshRenderer()->SetShader(m_basicShader);
            
            float offsetX = (m_sceneObjects.size() % 3) * 2.0f - 2.0f;
            float offsetZ = (m_sceneObjects.size() / 3) * 2.0f;
            testObject->GetTransform()->SetPosition(Vector3(offsetX, 0.0f, offsetZ));
            
            m_sceneObjects.push_back(testObject);
            cout << "Created test cube instead: " << objectID << endl;
        }
    }
}

} // namespace visualnnz