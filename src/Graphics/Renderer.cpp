#include "Renderer.h"
#include "Shader.h"
#include "../Components/MeshRenderer.h"
#include "../Components/Transform.h"
#include "../Scene/InteriorObject.h"
#include <d3dcompiler.h>
#include <directxtk/SimpleMath.h>
#include <iostream>

#if defined(_MSC_VER) && _MSC_VER < 1914
    #include <experimental/filesystem>
    namespace fs = std::experimental::filesystem;
#else
    #include <filesystem>
    namespace fs = std::filesystem;
#endif

namespace visualnnz
{
using Microsoft::WRL::ComPtr;
using namespace DirectX::SimpleMath;
using namespace std;

Renderer::Renderer()
{
    m_shader = make_unique<Shader>();
}

Renderer::~Renderer()
{
}

static int GetWindowWidth(HWND hWnd)
{
    RECT rect;
    GetClientRect(hWnd, &rect);
    return rect.right - rect.left;
}

static int GetWindowHeight(HWND hWnd)
{
    RECT rect;
    GetClientRect(hWnd, &rect);
    return rect.bottom - rect.top;
}

bool Renderer::Initialize(HWND hWnd)
{
    // 1. Device, Device Context, SwapChain ����
    if (!CreateDeviceAndSwapChain(hWnd))
        return false;

    // 2. RenderTargetView ����
    if (!CreateRenderTargetView())
        return false;

    // 3. DepthStencilView ���� (�ɼ�, �ʿ��)
    int width = GetWindowWidth(hWnd);
    int height = GetWindowHeight(hWnd);
    if (!CreateDepthStencilView(width, height))
        return false;

    // 4. ����Ʈ ����
    SetViewport(width, height);

    // 5. RenderTargetView�� DepthStencilView�� OM�� ���ε�
    m_context->OMSetRenderTargets(
        1, 
        m_renderTargetView.GetAddressOf(), 
        m_depthStencilView.Get()
    );

    // 6. ������ ���� ���� �� ����
    if (!CreateRenderStates())
        return false;

    // ������ ���������� ����
    if (!CreateShaders())
        return false;
    if (!CreateConstantBuffer())
        return false;

    // ������ ũ�� �ʱ�ȭ �߰�
    m_currentWidth = width;   // �߰�
    m_currentHeight = height; // �߰�

    return true;
}

// Device, Device Context, SwapChain ����
bool Renderer::CreateDeviceAndSwapChain(HWND hWnd)
{
    // Device�� �÷��� ����
    UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    const D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_HARDWARE;

    // �ӽÿ� Device, Device Context
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;

    const D3D_FEATURE_LEVEL featureLevels[2] = {
        D3D_FEATURE_LEVEL_11_0, // �� ���� ������ ���� ������ ����
        D3D_FEATURE_LEVEL_9_3};
    D3D_FEATURE_LEVEL featureLevel;

    if (FAILED(D3D11CreateDevice(
            nullptr,                  // Specify nullptr to use the default adapter.
            driverType,               // Create a device using the hardware graphics driver.
            0,                        // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
            createDeviceFlags,        // Set debug and Direct2D compatibility flags.
            featureLevels,            // List of feature levels this app can support.
            ARRAYSIZE(featureLevels), // Size of the list above.
            D3D11_SDK_VERSION,        // Always set this to D3D11_SDK_VERSION for
                                      // Microsoft Store apps.
            device.GetAddressOf(),    // Returns the Direct3D device created.
            &featureLevel,            // Returns feature level of device created.
            context.GetAddressOf()    // Returns the device immediate context.
            )))
    {
        cout << "D3D11CreateDevice() failed." << endl;
        return false;
    }

    if (featureLevel != D3D_FEATURE_LEVEL_11_0)
    {
        cout << "D3D Feature Level 11 unsupported." << endl;
        return false;
    }

    // 4X MSAA �����ϴ��� Ȯ��
    device->CheckMultisampleQualityLevels(
        DXGI_FORMAT_R8G8B8A8_UNORM, 
        4, 
        &m_numQualityLevels
    );

    if (m_numQualityLevels <= 0)
    {
        cout << "MSAA not supported." << endl;
    }

    if (FAILED(device.As(&m_device)))
    {
        cout << "device.AS() failed." << endl;
        return false;
    }

    if (FAILED(context.As(&m_context)))
    {
        cout << "context.As() failed." << endl;
        return false;
    }

    // SwapChain ����
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Width = 0;
    swapChainDesc.BufferDesc.Height = 0;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = hWnd;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    if (m_numQualityLevels > 0)
    {
        swapChainDesc.SampleDesc.Count = 4;
        swapChainDesc.SampleDesc.Quality = m_numQualityLevels - 1;
    }
    else
    {
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
    }

    if (FAILED(D3D11CreateDeviceAndSwapChain(
            0, // Default adapter
            driverType,
            0, // No software device
            createDeviceFlags,
            featureLevels,
            1,
            D3D11_SDK_VERSION,
            &swapChainDesc,
            m_swapChain.GetAddressOf(),
            m_device.GetAddressOf(),
            &featureLevel,
            m_context.GetAddressOf())))
    {
        cout << "D3D11CreateDeviceAndSwapChain() failed." << endl;
        return false;
    }

    return true;
}

// RenderTargetView ����
bool Renderer::CreateRenderTargetView()
{
    ComPtr<ID3D11Texture2D> backBuffer;
    m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
    if (backBuffer)
    {
        m_device->CreateRenderTargetView(backBuffer.Get(), NULL, m_renderTargetView.GetAddressOf());
    }
    else
    {
        cout << "CreateRenderTargetView() failed." << std::endl;
        return false;
    }

    return true;
}

// DepthStencilView ����
bool Renderer::CreateDepthStencilView(int width, int height)
{
    // ����: ���� �̱���, �ʿ�� ����
    D3D11_TEXTURE2D_DESC depthStencilBufferDesc = {};
    ZeroMemory(&depthStencilBufferDesc, sizeof(depthStencilBufferDesc));
    depthStencilBufferDesc.Width = width;
    depthStencilBufferDesc.Height = height;
    depthStencilBufferDesc.MipLevels = 1;
    depthStencilBufferDesc.ArraySize = 1;
    depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilBufferDesc.CPUAccessFlags = 0;
    depthStencilBufferDesc.MiscFlags = 0;
    if (m_numQualityLevels > 0)
    {
        depthStencilBufferDesc.SampleDesc.Count = 4; // how many multisamples
        depthStencilBufferDesc.SampleDesc.Quality = m_numQualityLevels - 1;
    }
    else
    {
        depthStencilBufferDesc.SampleDesc.Count = 1; // how many multisamples
        depthStencilBufferDesc.SampleDesc.Quality = 0;
    }

    if (FAILED(m_device->CreateTexture2D(
        &depthStencilBufferDesc, 
        0, 
        m_depthStencilBuffer.GetAddressOf()
    )))
    {
        std::cout << "CreateTexture2D() failed." << std::endl;
    }
    if (FAILED(m_device->CreateDepthStencilView(
        m_depthStencilBuffer.Get(), 
        0, 
        m_depthStencilView.GetAddressOf()
    )))
    {
        std::cout << "CreateDepthStencilView() failed." << std::endl;
    }

    return true;
}

// ��� ���� ���� (����, ��, �������� ��Ŀ�)
bool Renderer::CreateConstantBuffer()
{
    // ��� ���� ����ü ���Ǵ� ����� �߰� �ʿ�
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.ByteWidth = sizeof(BasicVertexConstantData);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    HRESULT hr = m_device->CreateBuffer(&cbDesc, nullptr, m_vertexConstantBuffer.GetAddressOf());
    return SUCCEEDED(hr);
}

// ����Ʈ ����
void Renderer::SetViewport(int width, int height)
{
    /*static int previousGuiWidth = m_guiWidth;

    if (previousGuiWidth != m_guiWidth)
    {
        previousGuiWidth = m_guiWidth;
    }*/
    ZeroMemory(&m_screenViewport, sizeof(D3D11_VIEWPORT));
    m_screenViewport.Width = static_cast<float>(width);
    m_screenViewport.Height = static_cast<float>(height);
    m_screenViewport.MinDepth = 0.0f;
    m_screenViewport.MaxDepth = 1.0f;
    m_screenViewport.TopLeftX = 0.0f;
    m_screenViewport.TopLeftY = 0.0f;

    m_context->RSSetViewports(1, &m_screenViewport);
}

// ������ ���� ���� �� ����
bool Renderer::CreateRenderStates()
{
    // Rasterizer State ����
    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;   // �ָ��� ������
    rasterizerDesc.CullMode = D3D11_CULL_BACK;    // Backface culling
    rasterizerDesc.FrontCounterClockwise = false; // �ð������ �ո�
    rasterizerDesc.DepthBias = 0;
    rasterizerDesc.DepthBiasClamp = 0.0f;
    rasterizerDesc.SlopeScaledDepthBias = 0.0f;
    rasterizerDesc.DepthClipEnable = true; // ���� Ŭ���� Ȱ��ȭ
    rasterizerDesc.ScissorEnable = false;
    rasterizerDesc.MultisampleEnable = (m_numQualityLevels > 0); // MSAA ����
    rasterizerDesc.AntialiasedLineEnable = false;

    if (FAILED(m_device->CreateRasterizerState(&rasterizerDesc, m_rasterizerState.GetAddressOf())))
    {
        cout << "CreateRasterizerState() failed." << endl;
        return false;
    }

    // Depth Stencil State ����
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = true;                          // ���� �׽�Ʈ Ȱ��ȭ
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; // ���� ���� ���
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;           // ����� ���� �켱
    depthStencilDesc.StencilEnable = false;                       // ���ٽ� ��Ȱ��ȭ

    if (FAILED(m_device->CreateDepthStencilState(&depthStencilDesc, m_depthStencilState.GetAddressOf())))
    {
        cout << "CreateDepthStencilState() failed." << endl;
        return false;
    }

    // ������ ���µ��� ���������ο� ���ε�
    m_context->RSSetState(m_rasterizerState.Get());
    m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 1);

    return true;
}

void Renderer::Shutdown()
{
    // ComPtr�� �ڵ����� Release�� ȣ���ϹǷ� ����ֵ� ��
}

void Renderer::BeginScene(float *clearColor)
{
    // ���� Ÿ�ٰ� ���� ���ٽ� Ŭ����
    m_context->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);
    m_context->ClearDepthStencilView(m_depthStencilView.Get(), 
                                    D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 
                                    1.0f, 0);
}

void Renderer::EndScene()
{
    // �� ���۸� ����Ʈ ���۷� ��ü
    m_swapChain->Present(1, 0); // V-Sync Ȱ��ȭ
}

// ���̴� ������ �� ����
bool Renderer::CreateShaders()
{
    ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;

    // ���� �۾� ���͸� ���
    wchar_t currentDir[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, currentDir);
    wcout << L"Current Directory: " << currentDir << endl;

    // ���̴� ��� ����
    wstring vsPath = L"src/Graphics/shaders/BasicVertexShader.hlsl";
    wstring psPath = L"src/Graphics/shaders/BasicPixelShader.hlsl";

    if (!m_shader->Initialize(m_device, vsPath, psPath))
    {
        cout << "Shader initialization failed." << endl;
        return false;
    }

    return true;
}

// InteriorObject �������� ���� Draw()
void Renderer::Draw(InteriorObject *interiorObject)
{
    if (!interiorObject) return;
    
    MeshRenderer* meshRenderer = interiorObject->GetMeshRenderer();
    Transform* transform = interiorObject->GetTransform();
    
    if (meshRenderer && transform)
    {
        // ��� ���� ������Ʈ (modelMatrix)
        UpdateConstantBuffer(transform->GetModelMatrix());
        
        // ������
        meshRenderer->Render(m_context);
    }
}

// Constant Buffer ������Ʈ
void Renderer::UpdateConstantBuffer(const Matrix& modelMatrix)
{
    // �� ��İ� �������� ��� ���� (ī�޶� ���� �� ���� �ʿ�)
    Matrix viewMatrix = Matrix::CreateLookAt(Vector3(0, 0, -5), Vector3(0, 0, 0), Vector3(0, 1, 0));
    Matrix projMatrix = Matrix::CreatePerspectiveFieldOfView(
        DirectX::XM_PI / 4.0f,  // 45�� �þ߰�
        static_cast<float>(m_currentWidth) / static_cast<float>(m_currentHeight), // ���� ��Ⱦ�� ���
        0.1f, 100.0f           // Near, Far ���
    );
    
    BasicVertexConstantData cbData;

    // invTranspose ��� (��� ��ȯ��)
    Matrix invTranspose = modelMatrix.Invert().Transpose();

    cbData.modelMatrix = modelMatrix.Transpose();  // HLSL�� �� �켱�̹Ƿ� ��ġ
    cbData.invTranspose = invTranspose.Transpose();
    cbData.viewMatrix = viewMatrix.Transpose();
    cbData.projMatrix = projMatrix.Transpose();
    
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    m_context->Map(m_vertexConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    memcpy(mappedResource.pData, &cbData, sizeof(BasicVertexConstantData));
    m_context->Unmap(m_vertexConstantBuffer.Get(), 0);
    
    m_context->VSSetConstantBuffers(0, 1, m_vertexConstantBuffer.GetAddressOf());
}

// ������ ũ�� ���� ó��
bool Renderer::OnWindowResize(int width, int height)
{
    if (width == m_currentWidth && height == m_currentHeight)
        return true; // ũ�Ⱑ ������� �ʾ����� ����

    m_currentWidth = width;
    m_currentHeight = height;

    // ���� ���� Ÿ�� ����
    m_renderTargetView.Reset();
    m_depthStencilView.Reset();
    m_depthStencilBuffer.Reset();

    // ����ü�� ���� ũ�� ����
    HRESULT hr = m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr))
    {
        cout << "SwapChain ResizeBuffers failed." << endl;
        return false;
    }

    // ���� Ÿ�� �� �����
    if (!CreateRenderTargetView())
        return false;

    // ���� ���ٽ� �� �����
    if (!CreateDepthStencilView(width, height))
        return false;

    // ����Ʈ �缳��
    SetViewport(width, height);

    // ���� Ÿ�� ����ε�
    m_context->OMSetRenderTargets(
        1, 
        m_renderTargetView.GetAddressOf(), 
        m_depthStencilView.Get()
    );

    return true;
}

} // namespace visualnnz
