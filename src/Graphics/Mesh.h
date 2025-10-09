#pragma once

#include <d3d11.h>
#include <directxtk/SimpleMath.h>
#include <wrl.h>
#include <vector>

namespace visualnnz
{
using Microsoft::WRL::ComPtr;
using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;
using std::vector;

struct MeshVertex
{
    Vector3 position;
    Vector3 normal; // Normal
    Vector2 texcoord; // Texture coordinates
};

class Mesh
{
public:
    Mesh();
    ~Mesh();

    bool Initialize(ComPtr<ID3D11Device> device, 
        const vector<MeshVertex>& vertices, 
        const vector<uint32_t>& indices
    );
    void Render(ComPtr<ID3D11DeviceContext> context);

    // 기본 도형 생성 함수들(예제)
    static vector<MeshVertex> CreateCubeVertices();
    static vector<uint32_t> CreateCubeIndices();
    
    static vector<MeshVertex> CreateTetrahedronVertices();
    static vector<uint32_t> CreateTetrahedronIndices();

private:
    ComPtr<ID3D11Buffer> m_vertexBuffer;
    ComPtr<ID3D11Buffer> m_indexBuffer;
    UINT m_indexCount;
    UINT m_vertexCount;
};
}