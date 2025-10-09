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

// ���� ����
class InteriorObject;
class Shader;

class Renderer
{
public:
    Renderer();
    ~Renderer();

    bool Initialize(HWND hWnd);
    void Shutdown();

    // ������ ����Ŭ �Լ�
    void BeginScene(float *clearColor);
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
    bool CreateConstantBuffer();
    void UpdateConstantBuffer(const Matrix &worldMatrix);

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

    // ���� ���ҽ�
    ComPtr<ID3D11Buffer> m_vertexBuffer;
    ComPtr<ID3D11Buffer> m_indexBuffer;
    ComPtr<ID3D11Buffer> m_vertexConstantBuffer;

    ComPtr<ID3D11RasterizerState> m_rasterizerState;

    D3D11_VIEWPORT m_screenViewport;

    // Renderer���� �����ϴ� ���
    unique_ptr<Shader> m_shader;

    // ��� ���� ������ ����ü
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
    //    Vector4 indexColor;       // ��ŷ(Picking)�� ���
    //};
};
} // namespace visualnnz