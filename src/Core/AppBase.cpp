#include <algorithm>
#include <tchar.h>

#include "AppBase.h" // �ڽ��� ��� ����

namespace visualnnz
{
using namespace std;

// RegisterClassEx()���� ��� �Լ��� ���� ����� ���� ���� ������
// Ŭ������ ��� �Լ����� ���������� �޽����� ó���� �� �ֵ��� �����ݴϴ�.
// AppBase *g_appBase = nullptr;

AppBase::AppBase()
    : m_screenWidth(1280),
      m_screenHeight(960)
{
    // TODO(visualnnz): AppBase �ʱ�ȭ ����

}

AppBase::~AppBase()
{
}

bool AppBase::Initialize(HINSTANCE hInstance, int nCmdShow)
{
    m_hInstance = hInstance;
    const TCHAR *className = _T("3D Interior Design Simulation");

    // 1. ������ Ŭ���� ���
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = AppBase::WndProc; // �޽��� ó�� �Լ� ����
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
    // TODO(YourName): �ػ�(width, height)�� �ܺο��� ������ �� �ֵ��� ����
    int screenWidth = 1280;
    int screenHeight = 720;

    m_mainWindow = CreateWindowEx(
        WS_EX_APPWINDOW,
        className,
        _T("3D Interior Simulation"), // â ����
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        screenWidth,
        screenHeight,
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

    return true;
}

void AppBase::Shutdown()
{
    DestroyWindow(m_mainWindow);
    // ������ Ŭ���� ��� ���� ��
}

int AppBase::Run()
{
    // TODO(YourName): ������ Ÿ�̸� ���� (QueryPerformanceCounter)
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

LRESULT CALLBACK AppBase::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    AppBase *app = nullptr;

    if (message == WM_NCCREATE)
    {
        // CreateWindowEx�� ������ ���ڷ� �ѱ� this �����͸� �޾ƿ� ����
        CREATESTRUCT *createStruct = reinterpret_cast<CREATESTRUCT *>(lParam);
        app = reinterpret_cast<AppBase *>(createStruct->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        app->m_mainWindow = hWnd;
    }
    else
    {
        // ����� this �����͸� �ٽ� �ҷ���
        app = reinterpret_cast<AppBase *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (app)
    {
        // TODO(YourName): ImGui �޽��� ó���� ����
        // ex) ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam);

        switch (message)
        {
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
        }
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}
} // namespace visualnnz
