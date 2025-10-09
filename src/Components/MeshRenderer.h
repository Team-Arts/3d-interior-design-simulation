#pragma once

#include "../Graphics/Mesh.h"
#include "../Graphics/Shader.h"
#include <memory>

namespace visualnnz
{
using std::shared_ptr;

class MeshRenderer
{
public:
    MeshRenderer();
    ~MeshRenderer();

    bool Initialize(ComPtr<ID3D11Device> device);
    void SetMesh(shared_ptr<Mesh> mesh);
    void SetShader(shared_ptr<Shader> shader);
    
    void Render(ComPtr<ID3D11DeviceContext> context);

private:
    shared_ptr<Mesh> m_mesh;
    shared_ptr<Shader> m_shader;
};
}
