#pragma once

#include <d3d11.h>
#include <wrl.h>

namespace visualnnz
{
using Microsoft::WRL::ComPtr;

// 전방 선언
class InteriorObject;

class Renderer
{
public:
    Renderer();
    ~Renderer();

    bool Initialize(HWND hWnd);
    void Shutdown();

    // 렌더링 사이클 함수
    void BeginScene(float r, float g, float b, float a);
    void EndScene();

    // 그리기 함수
    void Draw(InteriorObject *interiorObject);

    // Direct3D 장치 및 컨텍스트 접근자 - 성능상 inline으로 구현
    inline ComPtr<ID3D11Device> GetDevice() const { return m_device; }
    inline ComPtr<ID3D11DeviceContext> GetDeviceContext() const { return m_context; }

private:
    // 초기화 관련 함수
    bool CreateDeviceAndSwapChain(HWND hWnd);
    bool CreateRenderTargetView();
    bool CreateDepthStencilView(int width, int height);
    void SetViewport(int width, int height);
    bool CreateRenderStates();

    // 셰이더 및 버퍼 관련 함수
    bool CreateShaders();
    bool CreateBuffers();
    void SetRenderStates();

    // 윈도우 크기 변경 처리
    bool OnWindowResize(int width, int height);

    UINT m_numQualityLevels = 0;
    
    // 현재 윈도우 크기 추적
    int m_currentWidth = 0;
    int m_currentHeight = 0;

    // Direct3D 핵심 인터페이스
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_context;
    ComPtr<IDXGISwapChain> m_swapChain;
    ComPtr<ID3D11RenderTargetView> m_renderTargetView;

    // DepthBuffer 관련 인터페이스
    ComPtr<ID3D11Texture2D> m_depthStencilBuffer;
    ComPtr<ID3D11DepthStencilView> m_depthStencilView;
    ComPtr<ID3D11DepthStencilState> m_depthStencilState;

    // 셰이더, 입력 레이아웃, 버퍼
    ComPtr<ID3D11VertexShader> m_vertexShader;
    ComPtr<ID3D11PixelShader> m_pixelShader;
    ComPtr<ID3D11InputLayout> m_inputLayout;
    ComPtr<ID3D11Buffer> m_vertexBuffer;
    ComPtr<ID3D11Buffer> m_indexBuffer;

    ComPtr<ID3D11RasterizerState> m_rasterizerState;

    D3D11_VIEWPORT m_screenViewport;
};
} // namespace visualnnz