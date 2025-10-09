#include <algorithm>
#include <tchar.h>

#include "../Character/Camera.h"
#include "../Components/Transform.h"
#include "../Graphics/ModelLoader.h"
#include "../Input/InputManager.h"
#include "../Input/ObjectPicker.h"
#include "../Scene/InteriorObject.h"
#include "../UI/UIManager.h"
#include "AppBase.h"

// ImGui Win32 �޽��� �ڵ鷯 ����
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace visualnnz
{
using namespace std;
using namespace DirectX::SimpleMath;

// RegisterClassEx()���� ��� �Լ��� ���� ����� ���� ���� ������
// Ŭ������ ��� �Լ����� ���������� �޽����� ó���� �� �ֵ��� �����ش�.
AppBase *g_appBase = nullptr;

// RegisterClassEx()���� ������ ��ϵ� �ݹ� �Լ�
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // g_appBase�� �̿��ؼ� ���������� ��� �Լ� ȣ��
    return g_appBase->MsgProc(hWnd, msg, wParam, lParam);
}

AppBase::AppBase()
    : m_screenWidth(1280),
      m_screenHeight(720),
      m_inputManager(nullptr)
{
    g_appBase = this;

    // �Ŵ����� �ʱ�ȭ
    m_uiManager = make_unique<UIManager>();
    m_modelLoader = make_unique<ModelLoader>();
    m_objectPicker = make_unique<ObjectPicker>();
    m_camera = make_shared<Camera>();

    cout << "AppBase constructor completed" << endl;
}

AppBase::~AppBase()
{
    cout << "AppBase destructor called" << endl;
    if (m_uiManager)
    {
        m_uiManager->Shutdown();
    }
}

bool AppBase::Initialize(HINSTANCE hInstance, int nCmdShow)
{
    m_hInstance = hInstance;
    const TCHAR *className = _T("3D Interior Design Simulation");

    // 1. ������ Ŭ���� ���
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc; // �޽��� ó�� �Լ� ����
    wc.hInstance = m_hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = className;

    if (!RegisterClassEx(&wc))
    {
        MessageBox(nullptr, _T("Window Registration Failed!"), _T("Error"), MB_ICONEXCLAMATION | MB_OK);
        return false;
    }

    // 2. ������ ����
    m_screenWidth = 1280;
    m_screenHeight = 720;

    m_mainWindow = CreateWindowEx(
        WS_EX_APPWINDOW,
        className,
        _T("3D Interior Simulation"), // â ����
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        m_screenWidth,
        m_screenHeight,
        nullptr,
        nullptr,
        m_hInstance,
        this // ������ ���ڷ� this�� �Ѱ� WndProc�� ����
    );

    if (!m_mainWindow)
    {
        cout << "CreateWindowEx() failed." << endl;
        return false;
    }

    // 3. ������ ǥ��
    ShowWindow(m_mainWindow, nCmdShow);
    UpdateWindow(m_mainWindow);

    cout << "Window created successfully" << endl;
    return true;
}

void AppBase::InitializeManagers()
{
    cout << "Initializing base managers..." << endl;

    // InputManager �ʱ�ȭ
    m_inputManager = &InputManager::GetInstance();
    if (!m_inputManager->Initialize(m_mainWindow))
    {
        cout << "Failed to initialize InputManager!" << endl;
        return;
    }
    m_inputManager->SetCamera(m_camera);

    // Camera �ʱ�ȭ - ȭ�� ���� ����
    RECT clientRect;
    GetClientRect(m_mainWindow, &clientRect);
    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    m_camera->SetPerspective(45.0f * 3.14159f / 180.0f, aspectRatio, 0.1f, 100.0f);
    m_camera->SetPosition(Vector3(0.0f, 2.0f, 5.0f));
    m_camera->SetTarget(Vector3(0.0f, 0.0f, 0.0f));

    cout << "Base managers initialized successfully!" << endl;
}

void AppBase::SetupUICallbacks()
{
    cout << "Setting up UI callbacks..." << endl;

    if (!m_uiManager)
    {
        cout << "UIManager is null!" << endl;
        return;
    }

    // �� ���� �ݹ�
    m_uiManager->SetOnModelSpawnCallback(
        [this](const string &modelName)
        {
            cout << "Model spawn callback triggered: " << modelName << endl;
            OnModelSpawn(modelName);
        });

    // ������Ʈ ���� �ݹ�
    m_uiManager->SetOnObjectDeleteCallback(
        [this](shared_ptr<InteriorObject> object)
        {
            cout << "Object delete callback triggered" << endl;
            OnObjectDelete(object);
        });

    cout << "UI callbacks set up successfully!" << endl;
}

void AppBase::Shutdown()
{
    cout << "AppBase shutdown..." << endl;

    if (m_uiManager)
    {
        m_uiManager->Shutdown();
    }

    DestroyWindow(m_mainWindow);
}

int AppBase::Run()
{
    // ������ Ÿ�̸� ����
    LARGE_INTEGER frequency, prevTime, currTime;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&prevTime);

    float deltaTime = 1.0f / 60.0f;

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // ��Ÿ Ÿ�� ���
            QueryPerformanceCounter(&currTime);
            deltaTime = static_cast<float>(currTime.QuadPart - prevTime.QuadPart) / frequency.QuadPart;
            prevTime = currTime;

            // �ִ� ��Ÿ Ÿ�� ���� (60fps ���� �ּ�)
            if (deltaTime > 1.0f / 30.0f)
                deltaTime = 1.0f / 60.0f;

            // �� ������Ʈ��� UI ������Ʈ
            UpdateSceneObjects(deltaTime);
            UpdateUI();

            // MainApp�� ������Ʈ�� ������
            Update(deltaTime);
            Render();
        }
    }
    return static_cast<int>(msg.wParam);
}

HWND AppBase::GetWindowHandle() const
{
    return m_mainWindow;
}

LRESULT CALLBACK AppBase::MsgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // ImGui �޽��� ���� ó��
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        return true;

    switch (message)
    {
        case WM_SIZE:
            // ������ ũ�� ���� ó��
            if (m_camera && wParam != SIZE_MINIMIZED)
            {
                RECT clientRect;
                GetClientRect(m_mainWindow, &clientRect);
                int width = clientRect.right - clientRect.left;
                int height = clientRect.bottom - clientRect.top;

                if (width > 0 && height > 0)
                {
                    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
                    m_camera->SetPerspective(45.0f * 3.14159f / 180.0f, aspectRatio, 0.1f, 100.0f);
                }
            }
            break;
        case WM_LBUTTONDOWN:
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            OnMouseClick(x, y);
            m_lastMouseX = x;
            m_lastMouseY = y;
            break;
        case WM_MOUSEMOVE:
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            if (m_isDragging)
            {
                OnMouseMove(x, y);
            }
            m_lastMouseX = x;
            m_lastMouseY = y;
            break;
        case WM_LBUTTONUP:
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            OnMouseRelease(x, y);
            break;
        case WM_KEYDOWN:
            OnKeyDown(wParam);
            break;
        case WM_KEYUP:
            OnKeyUp(wParam);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

void AppBase::UpdateSceneObjects(float deltaTime)
{
    // InputManager ������Ʈ
    if (m_inputManager)
    {
        m_inputManager->Update(deltaTime);
    }

    // �� ������Ʈ�� ������Ʈ
    for (auto &obj : m_sceneObjects)
    {
        if (obj)
        {
            obj->Update(deltaTime);
        }
    }
}

void AppBase::UpdateUI()
{
    // UI�� �� ������Ʈ ��� ������Ʈ
    if (m_uiManager)
    {
        m_uiManager->UpdateObjectList(m_sceneObjects);
    }
}

// �⺻ �̺�Ʈ �ڵ鷯 ������ - MainApp���� �������̵� ����
void AppBase::OnModelSpawn(const string &modelName)
{
    cout << "AppBase::OnModelSpawn - " << modelName << endl;
    // �⺻ ����: MainApp���� �������̵� �ʿ�
}

void AppBase::OnObjectDelete(shared_ptr<InteriorObject> object)
{
    if (!object)
        return;

    auto it = find(m_sceneObjects.begin(), m_sceneObjects.end(), object);
    if (it != m_sceneObjects.end())
    {
        cout << "Deleting object: " << object->GetObjectID() << endl;
        m_sceneObjects.erase(it);

        if (m_selectedObject == object)
        {
            m_selectedObject = nullptr;
            if (m_uiManager)
            {
                m_uiManager->SetSelectedObject(nullptr);
            }
        }
    }
}

void AppBase::OnMouseClick(int x, int y)
{
    if (!m_objectPicker || !m_camera)
        return;

    RECT clientRect;
    GetClientRect(m_mainWindow, &clientRect);
    int screenWidth = clientRect.right - clientRect.left;
    int screenHeight = clientRect.bottom - clientRect.top;

    cout << "Mouse click at: " << x << ", " << y << endl;

    auto pickResult = m_objectPicker->PickObjectAtScreenPos(
        x, y, screenWidth, screenHeight, *m_camera, m_sceneObjects);

    if (pickResult.hit)
    {
        cout << "Object picked: " << pickResult.object->GetObjectID() << endl;

        // ���� ���� ����
        if (m_selectedObject)
        {
            m_selectedObject->SetSelected(false);
        }

        m_selectedObject = pickResult.object;
        m_selectedObject->SetSelected(true);

        if (m_uiManager)
        {
            m_uiManager->SetSelectedObject(m_selectedObject);
        }

        // �巡�� ����
        m_isDragging = true;
        Vector3 objPos = m_selectedObject->GetTransform()->GetPosition();
        m_dragOffset = objPos - pickResult.hitPoint;
    }
    else
    {
        cout << "No object picked" << endl;
        // ���� ���� ����
        if (m_selectedObject)
        {
            m_selectedObject->SetSelected(false);
        }
        m_selectedObject = nullptr;
        if (m_uiManager)
        {
            m_uiManager->SetSelectedObject(nullptr);
        }
    }
}

void AppBase::OnMouseMove(int x, int y)
{
    if (m_isDragging && m_selectedObject && m_objectPicker && m_camera)
    {
        RECT clientRect;
        GetClientRect(m_mainWindow, &clientRect);
        int screenWidth = clientRect.right - clientRect.left;
        int screenHeight = clientRect.bottom - clientRect.top;

        Ray ray = m_objectPicker->CreatePickingRay(x, y, screenWidth, screenHeight, *m_camera);

        // ������ ��� ���� ��� (Y=0 ���)
        if (abs(ray.direction.y) > 0.001f) // 0���� ������ ����
        {
            float t = -ray.position.y / ray.direction.y;
            if (t > 0)
            {
                Vector3 newPos = ray.position + ray.direction * t + m_dragOffset;
                m_selectedObject->GetTransform()->SetPosition(newPos);
            }
        }
    }
}

void AppBase::OnMouseRelease(int x, int y)
{
    if (m_isDragging)
    {
        cout << "Mouse released, ending drag" << endl;
        m_isDragging = false;
    }
}

void AppBase::OnKeyDown(WPARAM wParam)
{
    switch (wParam)
    {
        case VK_ESCAPE:
            // ���� ����
            if (m_selectedObject)
            {
                m_selectedObject->SetSelected(false);
            }
            m_selectedObject = nullptr;
            if (m_uiManager)
            {
                m_uiManager->SetSelectedObject(nullptr);
            }
            cout << "Selection cleared" << endl;
            break;
        case VK_DELETE:
            // ���õ� ������Ʈ ����
            if (m_selectedObject)
            {
                OnObjectDelete(m_selectedObject);
            }
            break;
    }
}

void AppBase::OnKeyUp(WPARAM wParam)
{
    // Ű ������ �̺�Ʈ ó�� (�ʿ�� MainApp���� �������̵�)
}

} // namespace visualnnz
