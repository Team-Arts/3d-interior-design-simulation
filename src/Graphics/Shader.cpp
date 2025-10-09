#include "Shader.h"
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

Shader::Shader()
{
}

Shader::~Shader()
{
}

bool Shader::Initialize(ComPtr<ID3D11Device> device, const wstring &vsPath, const wstring &psPath)
{
    ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;

    // 현재 작업 디렉터리 출력
    wchar_t currentDir[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, currentDir);
    wcout << L"Current Directory: " << currentDir << endl;

    // 셰이더 경로 설정
    // wstring vsPath = L"src/Graphics/shaders/BasicVertexShader.hlsl";
    // wstring psPath = L"src/Graphics/shaders/BasicPixelShader.hlsl";

    if (fs::exists(vsPath))
    {
        wcout << L"Vertex shader file found: " << vsPath << endl;
    }
    else
    {
        wcout << L"Vertex shader file NOT found: " << vsPath << endl;
        return false;
    }

    // Vertex Shader 컴파일
    HRESULT hr = D3DCompileFromFile(
        vsPath.c_str(),
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE, // Include handler 추가
        "main",
        "vs_5_0",
        0,
        0,
        vsBlob.GetAddressOf(),
        errorBlob.GetAddressOf());

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            cout << "Vertex Shader Error: " << (char *)errorBlob->GetBufferPointer() << endl;
        }
        cout << "Failed to load vertex shader from: ";
        wcout << vsPath << endl;
        return false;
    }

    // Pixel Shader 컴파일
    hr = D3DCompileFromFile(
        psPath.c_str(),
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE, // Include handler 추가
        "main",
        "ps_5_0",
        0,
        0,
        psBlob.GetAddressOf(),
        errorBlob.GetAddressOf());

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            cout << "Pixel Shader Error: " << (char *)errorBlob->GetBufferPointer() << endl;
        }
        cout << "Failed to load pixel shader from: ";
        wcout << psPath << endl;
        return false;
    }

    hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, m_vertexShader.GetAddressOf());
    if (FAILED(hr))
        return false;

    hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, m_pixelShader.GetAddressOf());
    if (FAILED(hr))
        return false;

    // 입력 레이아웃 생성
    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    hr = device->CreateInputLayout(
        layoutDesc,
        3,
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        m_inputLayout.GetAddressOf()
    );

    return SUCCEEDED(hr);
}

void Shader::SetActive(ComPtr<ID3D11DeviceContext> context)
{
    context->IASetInputLayout(m_inputLayout.Get());
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
}

} // namespace visualnnz
