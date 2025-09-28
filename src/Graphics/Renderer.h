#pragma once

#include <d3d11.h>
#include <wrl.h>

namespace visualnnz
{
using Microsoft::WRL::ComPtr;

// ���� ����
class InteriorObject;

class Renderer
{
public:
    Renderer();
    ~Renderer();

    bool Initialize(HWND hWnd);
    void Shutdown();

    // ������ ����Ŭ �Լ�
    void BeginScene(float r, float g, float b, float a);
    void EndScene();

    // �׸��� �Լ�
    void Draw(InteriorObject *interiorObject);

    // Direct3D ��ġ �� ���ؽ�Ʈ ������ - ���ɻ� inline���� ����
    inline ComPtr<ID3D11Device> GetDevice() const { return m_device; }
    inline ComPtr<ID3D11DeviceContext> GetDeviceContext() const { return m_context; }

private:
    // �ʱ�ȭ ���� �Լ�
    bool CreateDeviceAndSwapChain(HWND hWnd);
    bool CreateRenderTargetView();
    bool CreateDepthStencilView(int width, int height);
    void SetViewport(int width, int height);
    bool CreateRenderStates();

    // ���̴� �� ���� ���� �Լ�
    bool CreateShaders();
    bool CreateBuffers();
    void SetRenderStates();

    // ������ ũ�� ���� ó��
    bool OnWindowResize(int width, int height);

    UINT m_numQualityLevels = 0;
    
    // ���� ������ ũ�� ����
    int m_currentWidth = 0;
    int m_currentHeight = 0;

    // Direct3D �ٽ� �������̽�
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_context;
    ComPtr<IDXGISwapChain> m_swapChain;
    ComPtr<ID3D11RenderTargetView> m_renderTargetView;

    // DepthBuffer ���� �������̽�
    ComPtr<ID3D11Texture2D> m_depthStencilBuffer;
    ComPtr<ID3D11DepthStencilView> m_depthStencilView;
    ComPtr<ID3D11DepthStencilState> m_depthStencilState;

    // ���̴�, �Է� ���̾ƿ�, ����
    ComPtr<ID3D11VertexShader> m_vertexShader;
    ComPtr<ID3D11PixelShader> m_pixelShader;
    ComPtr<ID3D11InputLayout> m_inputLayout;
    ComPtr<ID3D11Buffer> m_vertexBuffer;
    ComPtr<ID3D11Buffer> m_indexBuffer;

    ComPtr<ID3D11RasterizerState> m_rasterizerState;

    D3D11_VIEWPORT m_screenViewport;
};
} // namespace visualnnz