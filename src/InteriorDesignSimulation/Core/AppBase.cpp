#include <algorithm>
#include <tchar.h>

#include "AppBase.h" // 자신의 헤더 파일

namespace visualnnz
{
using namespace std;

// RegisterClassEx()에서 멤버 함수를 직접 등록할 수가 없기 때문에
// 클래스의 멤버 함수에서 간접적으로 메시지를 처리할 수 있도록 도와줍니다.
// AppBase *g_appBase = nullptr;

AppBase::AppBase()
    : m_screenWidth(1280),
      m_screenHeight(960)
{
    // TODO(visualnnz): AppBase 초기화 로직

}

AppBase::~AppBase()
{
}

bool AppBase::Initialize(HINSTANCE hInstance, int nCmdShow)
{
    m_hInstance = hInstance;
    const TCHAR *className = _T("3D Interior Design Simulation");

    // 1. 윈도우 클래스 등록
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = AppBase::WndProc; // 메시지 처리 함수 연결
    wc.hInstance = m_hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = className;

    if (!RegisterClassEx(&wc))
    {
        MessageBox(nullptr, _T("Window Registration Failed!"), _T("Error"), MB_ICONEXCLAMATION | MB_OK);
        return false;
    }

    // 2. 윈도우 생성
    // TODO(YourName): 해상도(width, height)를 외부에서 설정할 수 있도록 수정
    int screenWidth = 1280;
    int screenHeight = 720;

    m_mainWindow = CreateWindowEx(
        WS_EX_APPWINDOW,
        className,
        _T("3D Interior Simulation"), // 창 제목
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        screenWidth,
        screenHeight,
        nullptr,
        nullptr,
        m_hInstance,
        this // 마지막 인자로 this를 넘겨 WndProc과 연결
    );

    if (!m_mainWindow)
    {
        cout << "CreateWindowEx() failed." << endl;
        return false;
    }

    // 3. 윈도우 표시
    ShowWindow(m_mainWindow, nCmdShow);
    UpdateWindow(m_mainWindow);

    return true;
}

void AppBase::Shutdown()
{
    DestroyWindow(m_mainWindow);
    // 윈도우 클래스 등록 해제 등
}

int AppBase::Run()
{
    // TODO(YourName): 고정밀 타이머 구현 (QueryPerformanceCounter)
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
        // CreateWindowEx의 마지막 인자로 넘긴 this 포인터를 받아와 저장
        CREATESTRUCT *createStruct = reinterpret_cast<CREATESTRUCT *>(lParam);
        app = reinterpret_cast<AppBase *>(createStruct->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        app->m_mainWindow = hWnd;
    }
    else
    {
        // 저장된 this 포인터를 다시 불러옴
        app = reinterpret_cast<AppBase *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (app)
    {
        // TODO(YourName): ImGui 메시지 처리기 연결
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
