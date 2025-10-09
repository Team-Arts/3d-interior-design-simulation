#include "MeshRenderer.h"

namespace visualnnz
{
using namespace std;

MeshRenderer::MeshRenderer()
{
}

MeshRenderer::~MeshRenderer()
{
}

bool MeshRenderer::Initialize(ComPtr<ID3D11Device> device)
{
    // MeshRenderer 초기화 로직
    return true;
}

void MeshRenderer::SetMesh(shared_ptr<Mesh> mesh)
{
    m_mesh = mesh;
}

void MeshRenderer::SetShader(shared_ptr<Shader> shader)
{
    m_shader = shader;
}

void MeshRenderer::Render(ComPtr<ID3D11DeviceContext> context)
{
    if (m_mesh && m_shader)
    {
        m_shader->SetActive(context);
        m_mesh->Render(context);
    }
}

} // namespace visualnnz