#include "Renderer.h"
#include <d3dcompiler.h>
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
using namespace std;

Renderer::Renderer()
{
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
    // 1. Device, Device Context, SwapChain 생성
    if (!CreateDeviceAndSwapChain(hWnd))
        return false;

    // 2. RenderTargetView 생성
    if (!CreateRenderTargetView())
        return false;

    // 3. DepthStencilView 생성 (옵션, 필요시)
    int width = GetWindowWidth(hWnd);
    int height = GetWindowHeight(hWnd);
    if (!CreateDepthStencilView(width, height))
        return false;

    // 4. 뷰포트 설정
    SetViewport(width, height);

    // 5. RenderTargetView와 DepthStencilView를 OM에 바인딩
    m_context->OMSetRenderTargets(
        1, 
        m_renderTargetView.GetAddressOf(), 
        m_depthStencilView.Get()
    );

    // 6. 렌더링 상태 생성 및 설정
    if (!CreateRenderStates())
        return false;

    // 렌더링 파이프라인 구축
    if (!CreateShaders())
        return false;
    if (!CreateBuffers())
        return false;

    return true;
}

// Device, Device Context, SwapChain 생성
bool Renderer::CreateDeviceAndSwapChain(HWND hWnd)
{
    // Device에 플래그 설정
    UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    const D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_HARDWARE;

    // 임시용 Device, Device Context
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;

    const D3D_FEATURE_LEVEL featureLevels[2] = {
        D3D_FEATURE_LEVEL_11_0, // 더 높은 버전이 먼저 오도록 설정
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

    // 4X MSAA 지원하는지 확인
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

    // SwapChain 설정
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

// RenderTargetView 생성
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

// DepthStencilView 생성
bool Renderer::CreateDepthStencilView(int width, int height)
{
    // 예시: 아직 미구현, 필요시 구현
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

// 뷰포트 설정
void Renderer::SetViewport(int width, int height)
{
    /*static int previousGuiWidth = m_guiWidth;

    if (previousGuiWidth != m_guiWidth)
    {
        previousGuiWidth = m_guiWidth;
    }*/
    D3D11_VIEWPORT m_screenViewport = {};
    m_screenViewport.Width = static_cast<float>(width);
    m_screenViewport.Height = static_cast<float>(height);
    m_screenViewport.MinDepth = 0.0f;
    m_screenViewport.MaxDepth = 1.0f;
    m_screenViewport.TopLeftX = 0.0f;
    m_screenViewport.TopLeftY = 0.0f;

    m_context->RSSetViewports(1, &m_screenViewport);
}

// 렌더링 상태 생성 및 설정
bool Renderer::CreateRenderStates()
{
    // Rasterizer State 생성
    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;   // 솔리드 렌더링
    rasterizerDesc.CullMode = D3D11_CULL_BACK;    // Backface culling
    rasterizerDesc.FrontCounterClockwise = false; // 시계방향이 앞면
    rasterizerDesc.DepthBias = 0;
    rasterizerDesc.DepthBiasClamp = 0.0f;
    rasterizerDesc.SlopeScaledDepthBias = 0.0f;
    rasterizerDesc.DepthClipEnable = true; // 깊이 클리핑 활성화
    rasterizerDesc.ScissorEnable = false;
    rasterizerDesc.MultisampleEnable = (m_numQualityLevels > 0); // MSAA 설정
    rasterizerDesc.AntialiasedLineEnable = false;

    if (FAILED(m_device->CreateRasterizerState(&rasterizerDesc, m_rasterizerState.GetAddressOf())))
    {
        cout << "CreateRasterizerState() failed." << endl;
        return false;
    }

    // Depth Stencil State 생성
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = true;                          // 깊이 테스트 활성화
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; // 깊이 쓰기 허용
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;           // 가까운 것이 우선
    depthStencilDesc.StencilEnable = false;                       // 스텐실 비활성화

    if (FAILED(m_device->CreateDepthStencilState(&depthStencilDesc, m_depthStencilState.GetAddressOf())))
    {
        cout << "CreateDepthStencilState() failed." << endl;
        return false;
    }

    // 생성한 상태들을 파이프라인에 바인딩
    m_context->RSSetState(m_rasterizerState.Get());
    m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 1);

    return true;
}

void Renderer::Shutdown()
{
    // ComPtr이 자동으로 Release를 호출하므로 비워둬도 됨
}

void Renderer::BeginScene(float r, float g, float b, float a)
{
    // 렌더 타겟과 깊이 스텐실 클리어
    float clearColor[4] = { r, g, b, a };
    m_context->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);
    m_context->ClearDepthStencilView(m_depthStencilView.Get(), 
                                    D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 
                                    1.0f, 0);
}

void Renderer::EndScene()
{
    // 백 버퍼를 프론트 버퍼로 교체
    m_swapChain->Present(1, 0); // V-Sync 활성화
}

// 현재 실행 파일의 디렉터리를 기준으로 절대 경로 생성
wstring GetExecutableDirectory()
{
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    wstring::size_type pos = wstring(buffer).find_last_of(L"\\/");
    return wstring(buffer).substr(0, pos);
}

// 셰이더 컴파일 및 생성
bool Renderer::CreateShaders()
{
    ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;

    // 현재 작업 디렉터리 출력
    wchar_t currentDir[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, currentDir);
    wcout << L"Current Directory: " << currentDir << endl;

    // 셰이더 경로 설정
    wstring vsPath = L"src/Graphics/shaders/BasicVertexShader.hlsl";
    wstring psPath = L"src/Graphics/shaders/BasicPixelShader.hlsl";

    if (fs::exists(vsPath))
    {
        wcout << L"Vertex shader file found: " << vsPath << endl;
    }
    else
    {
        wcout << L"Vertex shader file NOT found: " << vsPath << endl;
        return false;
    }
    
    // 실제 컴파일도 같은 경로 사용 (수정 부분)
    HRESULT hr = D3DCompileFromFile(
        vsPath.c_str(),  // ← 여기를 수정
        nullptr, 
        nullptr, 
        "main", 
        "vs_5_0", 
        0, 
        0, 
        vsBlob.GetAddressOf(), 
        errorBlob.GetAddressOf()
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            cout << "Vertex Shader Error: " << (char*)errorBlob->GetBufferPointer() << endl;
        }
        cout << "Failed to load vertex shader from: ";
        wcout << vsPath << endl;
        return false;
    }

    hr = D3DCompileFromFile(
        psPath.c_str(),  // ← 여기를 수정
        nullptr, 
        nullptr, 
        "main", 
        "ps_5_0", 
        0, 
        0, 
        psBlob.GetAddressOf(), 
        errorBlob.GetAddressOf()
    );
    
    if (FAILED(hr))
    {
        if (errorBlob)
        {
            cout << "Pixel Shader Error: " << (char*)errorBlob->GetBufferPointer() << endl;
        }
        cout << "Failed to load pixel shader from: ";
        wcout << psPath << endl;
        return false;
    }

    hr = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, m_vertexShader.GetAddressOf());
    if (FAILED(hr))
        return false;

    hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, m_pixelShader.GetAddressOf());
    if (FAILED(hr))
        return false;

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}};
    hr = m_device->CreateInputLayout(layoutDesc, 1, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), m_inputLayout.GetAddressOf());
    return SUCCEEDED(hr);
}

// 버퍼 생성 (예시: 삼각형)
bool Renderer::CreateBuffers()
{
    struct Vertex
    {
        float x, y, z;
    };
    Vertex vertices[] = {
        {0.0f, 0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {-0.5f, -0.5f, 0.0f}};
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices;
    HRESULT hr = m_device->CreateBuffer(&vbDesc, &vbData, m_vertexBuffer.GetAddressOf());
    if (FAILED(hr))
        return false;

    unsigned short indices[] = {0, 1, 2};
    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.ByteWidth = sizeof(indices);
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices;
    hr = m_device->CreateBuffer(&ibDesc, &ibData, m_indexBuffer.GetAddressOf());
    return SUCCEEDED(hr);
}

// 렌더링 상태 설정
void Renderer::SetRenderStates()
{
    m_context->IASetInputLayout(m_inputLayout.Get());
    UINT stride = sizeof(float) * 3;
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    m_context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
}

// 오브젝트 렌더링 함수
void Renderer::Draw(InteriorObject *interiorObject)
{
    SetRenderStates();
    m_context->DrawIndexed(3, 0, 0); // 예시: 삼각형
}

} // namespace visualnnz
