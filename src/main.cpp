#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <directxmath.h>
#include <d3dcompiler.h>
#include <fstream>
#include <vector>
#include <string> 
#include <sstream>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "Model.h"
#include "GltfLoader.h" // GLB 로더 헤더 추가
#include "ModelManager.h"
#include "RoomModel.h"
#include "Camera.h"  // Camera 클래스 정의를 위해 추가

// 필요한 라이브러리 링크
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// main.cpp의 전역 변수
ModelManager modelManager;
RoomModel* g_roomModel = nullptr;
LARGE_INTEGER frequency;        // 타이머 주파수
LARGE_INTEGER lastTime;         // 마지막 프레임 시간
float deltaTime = 0.0f;         // 프레임 간 시간 차이
// DirectX 11 변수
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
ID3D11DepthStencilView* g_pDepthStencilView = nullptr;
ID3D11Texture2D* g_pDepthStencilBuffer = nullptr;

// 함수 프로토타입 선언
void CreateRenderTarget();
void CleanupRenderTarget();
void CleanupDeviceD3D();
bool CreateDeviceD3D(HWND hWnd);
bool CreateDepthStencilView();

// DirectX 11 초기화 함수
bool CreateDeviceD3D(HWND hWnd)
{
    // 스왑 체인 설명 구조체 설정
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    // D3D11 디바이스 및 스왑 체인 생성
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        featureLevelArray, 2, D3D11_SDK_VERSION,
        &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel,
        &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    CreateDepthStencilView();
    return true;
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

bool CreateDepthStencilView()
{
    // 깊이 스텐실 버퍼 생성
    D3D11_TEXTURE2D_DESC depthBufferDesc;
    ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));

    depthBufferDesc.Width = 1280;
    depthBufferDesc.Height = 800;
    depthBufferDesc.MipLevels = 1;
    depthBufferDesc.ArraySize = 1;
    depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthBufferDesc.SampleDesc.Count = 1;
    depthBufferDesc.SampleDesc.Quality = 0;
    depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthBufferDesc.CPUAccessFlags = 0;
    depthBufferDesc.MiscFlags = 0;

    HRESULT result = g_pd3dDevice->CreateTexture2D(&depthBufferDesc, NULL, &g_pDepthStencilBuffer);
    if (FAILED(result))
    {
        return false;
    }

    // 깊이 스텐실 뷰 생성
    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
    ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));

    depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Texture2D.MipSlice = 0;

    result = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencilBuffer, &depthStencilViewDesc, &g_pDepthStencilView);
    if (FAILED(result))
    {
        return false;
    }

    return true;
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pDepthStencilView) { g_pDepthStencilView->Release(); g_pDepthStencilView = nullptr; }
    if (g_pDepthStencilBuffer) { g_pDepthStencilBuffer->Release(); g_pDepthStencilBuffer = nullptr; }
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

// ImGui 초기화 및 윈도우 프로시저
IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
//main.cpp 파일의 WndProc 함수를 수정합니다.
//기존 WndProc 함수에 마우스 이벤트 처리 코드를 추가합니다.

// 외부 전역 변수 선언 (main.cpp에 이미 있는 변수들)
//ModelManager modelManager; // ModelManager 인스턴스에 대한 외부 참조

// 기존 WndProc 함수를 다음과 같이 수정합니다:
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_KEYDOWN:
        // 'V' 키로 시점 전환
        if (wParam == 'V') {
            modelManager.ToggleCameraMode();
            return 0;
        }
        break;
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)  // Alt 키 메뉴 비활성화
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

        // 마우스 이벤트 처리 추가
    case WM_LBUTTONDOWN:
    {
        // ImGui가 마우스를 사용하지 않는 경우에만 처리
        if (!ImGui::GetIO().WantCaptureMouse)
        {
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);
            modelManager.OnMouseDown(xPos, yPos, hWnd);
        }
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        // ImGui가 마우스를 사용하지 않는 경우에만 처리
        if (!ImGui::GetIO().WantCaptureMouse)
        {
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);
            modelManager.OnMouseMove(xPos, yPos, hWnd);
        }
        return 0;
    }
    case WM_LBUTTONUP:
    {
        // ImGui가 마우스를 사용하지 않는 경우에만 처리
        if (!ImGui::GetIO().WantCaptureMouse)
        {
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);
            modelManager.OnMouseUp(xPos, yPos);
        }
        return 0;
    }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void InitImGui(HWND hwnd)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // 키보드 제어 활성화

    // ImGui 스타일 설정은 ModelManager에서 향상된 UI 스타일을 적용하므로 여기서는 기본 스타일만 설정
    ImGui::StyleColorsDark();

    // 플랫폼/렌더러 바인딩
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // 한글 폰트 로드 (대부분의 Windows 시스템에 설치된 맑은 고딕 사용)
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\malgun.ttf", 16.0f, NULL, io.Fonts->GetGlyphRangesKorean());

    // 한글 폰트가 없는 경우 기본 폰트 사용
    if (io.Fonts->Fonts.empty()) {
        io.Fonts->AddFontDefault();
    }

    // ImGui 폰트 텍스처 업데이트
    ImGui_ImplDX11_CreateDeviceObjects();
}

int main(int, char**)
{
    // 윈도우 생성
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"DirectX 3D Viewer", nullptr };
    RegisterClassEx(&wc);

    HWND hwnd = CreateWindow(wc.lpszClassName, L"3D 모델 뷰어 (OBJ 및 GLB)", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

    // 모델 관리자 생성
    //ModelManager modelManager;
    

    // 윈도우 생성 후:
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&lastTime);

    // DirectX 초기화
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // 방 모델 초기화
    auto roomModel = std::make_shared<RoomModel>();
    if (!roomModel->Initialize(g_pd3dDevice)) {
        // 오류 처리
        OutputDebugStringA("RoomModel 초기화 실패\n");
    }
    else {
        modelManager.SetRoomModel(roomModel);
        OutputDebugStringA("RoomModel 초기화 성공\n");
    }

    // 조명 관리자 초기화
    modelManager.InitLightManager(g_pd3dDevice);

    // 윈도우 표시
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // ImGui 초기화
    InitImGui(hwnd);

    // 뷰포트 설정
    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(viewport));
    viewport.Width = 1280.0f;
    viewport.Height = 800.0f;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    g_pd3dDeviceContext->RSSetViewports(1, &viewport);

    OutputDebugStringA("더미 캐릭터 초기화 시작...\n");
    // 더미 캐릭터 초기화
    modelManager.InitializeDummyCharacter(g_pd3dDevice);
    OutputDebugStringA("더미 캐릭터 초기화 완료\n");

    // 메인 루프
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        // 현재 시간 측정
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);

        // 델타 타임 계산 (초 단위)
        deltaTime = (float)(currentTime.QuadPart - lastTime.QuadPart) / (float)frequency.QuadPart;
        lastTime = currentTime;

        // 너무 큰 델타 타임 방지 (예: 디버그 일시 중지 후)
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        // ImGui 프레임 시작
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // ModelManager에 델타 타임 전달 - 향상된 UI 사용
        modelManager.RenderEnhancedUI(hwnd, g_pd3dDevice, deltaTime);

        // 렌더링
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, g_pDepthStencilView);
        float clearColor[4] = { 0.95f, 0.92f, 0.85f, 1.0f }; // 어두운 회색 배경
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clearColor);
        g_pd3dDeviceContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        // 방 및 모델 렌더링
        modelManager.RenderModels(g_pd3dDeviceContext);

        // ImGui 렌더링
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);
    }

    // 정리
    modelManager.Release();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    // main.cpp
    if (roomModel) {
        modelManager.SetRoomModel(roomModel);
    }

    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}
