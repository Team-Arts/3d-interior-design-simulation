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
    // TODO(YourName): HLSL ���̴� ���� ������ �� ���̴� ��ü ����
    return true;
}

void Shader::SetActive(ComPtr<ID3D11DeviceContext> context)
{
    // TODO(YourName): ���������ο� ���̴� ���ε�
}
} // namespace visualnnz
