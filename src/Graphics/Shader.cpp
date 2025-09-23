#include "Shader.h"

namespace visualnnz
{
using Microsoft::WRL::ComPtr;

Shader::Shader()
{
}

Shader::~Shader()
{
}

bool Shader::Initialize(ComPtr<ID3D11Device> device, const std::wstring &vsPath, const std::wstring &psPath)
{
    // TODO(YourName): HLSL 셰이더 파일 컴파일 및 셰이더 객체 생성
    return true;
}

void Shader::SetActive(ComPtr<ID3D11DeviceContext> context)
{
    // TODO(YourName): 파이프라인에 셰이더 바인딩
}
} // namespace visualnnz
