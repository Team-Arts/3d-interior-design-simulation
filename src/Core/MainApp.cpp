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
    
    // �θ� Ŭ���� �ʱ�ȭ (������ ����)
    if (!AppBase::Initialize(hInstance, nCmdShow))
    {
        cout << "Failed to initialize AppBase!" << endl;
        return false;
    }

    // �׷��Ƚ� �ʱ�ȭ
    if (!InitializeGraphics())
    {
        cout << "Failed to initialize graphics!" << endl;
        return false;
    }

    // �Ŵ����� �ʱ�ȭ
    InitializeManagers();

    // UI �ݹ� ����
    SetupUICallbacks();

    // �׽�Ʈ ������Ʈ�� ����
    CreateTestObjects();
    
    cout << "MainApp initialized successfully!" << endl;
    return true;
}

bool MainApp::InitializeGraphics()
{
    cout << "Initializing graphics..." << endl;
    
    // ������ �ʱ�ȭ(Direct3D ���� ����)
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
    
    // �θ� Ŭ������ �Ŵ��� �ʱ�ȭ ���� ȣ��
    AppBase::InitializeManagers();

    // UIManager �ʱ�ȭ (�������� �ʿ���)
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
    
    // 1. �⺻ ���̴� ����
    m_basicShader = make_shared<Shader>();
    wstring vsPath = L"src/Graphics/shaders/BasicVertexShader.hlsl";
    wstring psPath = L"src/Graphics/shaders/BasicPixelShader.hlsl";
    
    if (!m_basicShader->Initialize(m_renderer->GetDevice(), vsPath, psPath))
    {
        cout << "Failed to create basic shader!" << endl;
        return;
    }

    // 2. ť�� �޽� ����
    m_cubeMesh = make_shared<Mesh>();
    auto vertices = Mesh::CreateCubeVertices();
    auto indices = Mesh::CreateCubeIndices();
    
    if (!m_cubeMesh->Initialize(m_renderer->GetDevice(), vertices, indices))
    {
        cout << "Failed to create cube mesh!" << endl;
        return;
    }

    // 3. �׽�Ʈ ť�� ������Ʈ ����
    m_testCube = make_unique<InteriorObject>();
    
    if (m_testCube->Initialize(m_renderer->GetDevice(), "TestCube", "cube"))
    {
        // MeshRenderer�� �޽ÿ� ���̴� ����
        m_testCube->GetMeshRenderer()->SetMesh(m_cubeMesh);
        m_testCube->GetMeshRenderer()->SetShader(m_basicShader);
        
        // Transform ����
        m_testCube->GetTransform()->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
        m_testCube->GetTransform()->SetScale(Vector3(1.0f, 1.0f, 1.0f));
        
        // �� ������Ʈ ��Ͽ� �߰� (AppBase�� m_sceneObjects ���)
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
    // AppBase���� �̹� �� ������Ʈ ������Ʈ�� UI ������Ʈ�� ó���ϹǷ�
    // MainApp������ �߰����� ������ ����
    
    // ť�� ȸ�� �ִϸ��̼� (���õ��� �ʾ��� ����)
    if (m_testCube && !m_testCube->IsSelected())
    {
        static float rotation = 0.0f;
        rotation += deltaTime * 0.5f; // ���� ȸ��
        m_testCube->GetTransform()->SetRotation(Vector3(0.0f, rotation, 0.0f));
    }
}

void MainApp::Render()
{
    // 3D �� ������
    float clearColor[4] = {0.2f, 0.2f, 0.2f, 1.0f};
    m_renderer->BeginScene(clearColor);

    // �� ������Ʈ�� ������ (AppBase�� m_sceneObjects ���)
    for (auto& obj : m_sceneObjects)
    {
        if (obj)
        {
            m_renderer->Draw(obj.get());
        }
    }

    m_renderer->EndScene();

    // UI ������
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
    
    // GLB ���� �ε� �õ�
    auto newObject = m_modelLoader->LoadModel(m_renderer->GetDevice(), filePath, objectID);
    if (newObject)
    {
        // �⺻ ���̴� ����
        newObject->GetMeshRenderer()->SetShader(m_basicShader);
        
        // �⺻ ��ġ ���� (���� ������Ʈ��� ��ġ�� �ʵ���)
        float offsetX = (m_sceneObjects.size() % 3) * 2.0f - 2.0f;
        float offsetZ = (m_sceneObjects.size() / 3) * 2.0f;
        newObject->GetTransform()->SetPosition(Vector3(offsetX, 0.0f, offsetZ));
        
        m_sceneObjects.push_back(newObject);
        cout << "Model loaded successfully: " << modelName << endl;
    }
    else
    {
        cout << "Failed to load model: " << modelName << " - Creating test cube instead" << endl;
        
        // �׽�Ʈ������ ť�� ����
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