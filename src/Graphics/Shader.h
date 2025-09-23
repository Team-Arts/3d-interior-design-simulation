#pragma once

#include <d3d11.h>
#include <string>
#include <wrl.h>

namespace visualnnz
{
using Microsoft::WRL::ComPtr;

class Shader
{
public:
    Shader();
    ~Shader();

    bool Initialize(ComPtr<ID3D11Device> device, const std::wstring &vsPath, const std::wstring &psPath);
    void SetActive(ComPtr<ID3D11DeviceContext> context);

private:
    ComPtr<ID3D11VertexShader> m_vertexShader;
    ComPtr<ID3D11PixelShader> m_pixelShader;
    ComPtr<ID3D11InputLayout> m_inputLayout;
};
} // namespace visualnnz
