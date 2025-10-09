#pragma once

#include <d3d11.h>
#include <directxtk/SimpleMath.h>
#include <wrl.h>
#include <memory>

namespace visualnnz
{
using Microsoft::WRL::ComPtr;
using DirectX::SimpleMath::Matrix;
using std::unique_ptr;

// 전방 선언
class InteriorObject;
class Shader;

class Renderer
{
public:
    Renderer();
    ~Renderer();

    bool Initialize(HWND hWnd);
    void Shutdown();

    // 렌더링 사이클 함수
    void BeginScene(float *clearColor);
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
    bool CreateConstantBuffer();
    void UpdateConstantBuffer(const Matrix &worldMatrix);

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

    // 버퍼 리소스
    ComPtr<ID3D11Buffer> m_vertexBuffer;
    ComPtr<ID3D11Buffer> m_indexBuffer;
    ComPtr<ID3D11Buffer> m_vertexConstantBuffer;

    ComPtr<ID3D11RasterizerState> m_rasterizerState;

    D3D11_VIEWPORT m_screenViewport;

    // Renderer에서 관리하는 모듈
    unique_ptr<Shader> m_shader;

    // 상수 버퍼 데이터 구조체
    struct BasicVertexConstantData
    {
        Matrix modelMatrix;
        Matrix invTranspose;
        Matrix viewMatrix;
        Matrix projMatrix;
    };

    BasicVertexConstantData m_vertexConstantData;

    //struct BasicPixelConstantData
    //{
    //    Vector3 eyeWorld;         // 12
    //    bool useTexture;          // bool 1 + 3 padding
    //    Material material;        // 48
    //    Light lights[MAX_LIGHTS]; // 48 * MAX_LIGHTS
    //    Vector4 indexColor;       // 피킹(Picking)에 사용
    //};
};
} // namespace visualnnz