#include "Mesh.h"
#include <iostream>

namespace visualnnz
{
using namespace std;
using Microsoft::WRL::ComPtr;

Mesh::Mesh() : m_indexCount(0), m_vertexCount(0)
{
}

Mesh::~Mesh()
{
}

bool Mesh::Initialize(ComPtr<ID3D11Device> device, const vector<MeshVertex>& vertices, const vector<uint32_t>& indices)
{
    m_vertexCount = static_cast<UINT>(vertices.size());
    m_indexCount = static_cast<UINT>(indices.size());

    // Vertex Buffer 생성
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.ByteWidth = sizeof(MeshVertex) * m_vertexCount;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices.data();

    HRESULT hr = device->CreateBuffer(&vbDesc, &vbData, m_vertexBuffer.GetAddressOf());
    if (FAILED(hr))
    {
        cout << "Failed to create vertex buffer." << endl;
        return false;
    }

    // Index Buffer 생성
    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.ByteWidth = sizeof(uint32_t) * m_indexCount;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices.data();

    hr = device->CreateBuffer(&ibDesc, &ibData, m_indexBuffer.GetAddressOf());
    if (FAILED(hr))
    {
        cout << "Failed to create index buffer." << endl;
        return false;
    }

    return true;
}

void Mesh::Render(ComPtr<ID3D11DeviceContext> context)
{
    UINT stride = sizeof(MeshVertex);
    UINT offset = 0;
    
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    context->DrawIndexed(m_indexCount, 0, 0);
}

vector<MeshVertex> Mesh::CreateCubeVertices()
{
    return {
        // Front face
        {Vector3(-1.0f, -1.0f, 1.0f), Vector3(0.0f, 0.0f, 1.0f), Vector2(0.0f, 1.0f)},
        {Vector3(1.0f, -1.0f, 1.0f), Vector3(0.0f, 0.0f, 1.0f), Vector2(1.0f, 1.0f)},
        {Vector3(1.0f, 1.0f, 1.0f), Vector3(0.0f, 0.0f, 1.0f), Vector2(1.0f, 0.0f)},
        {Vector3(-1.0f, 1.0f, 1.0f), Vector3(0.0f, 0.0f, 1.0f), Vector2(0.0f, 0.0f)},

        // Back face
        {Vector3(-1.0f, -1.0f, -1.0f), Vector3(0.0f, 0.0f, -1.0f), Vector2(1.0f, 1.0f)},
        {Vector3(-1.0f, 1.0f, -1.0f), Vector3(0.0f, 0.0f, -1.0f), Vector2(1.0f, 0.0f)},
        {Vector3(1.0f, 1.0f, -1.0f), Vector3(0.0f, 0.0f, -1.0f), Vector2(0.0f, 0.0f)},
        {Vector3(1.0f, -1.0f, -1.0f), Vector3(0.0f, 0.0f, -1.0f), Vector2(0.0f, 1.0f)},

        // Top face
        {Vector3(-1.0f, 1.0f, -1.0f), Vector3(0.0f, 1.0f, 0.0f), Vector2(0.0f, 1.0f)},
        {Vector3(-1.0f, 1.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f), Vector2(0.0f, 0.0f)},
        {Vector3(1.0f, 1.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f), Vector2(1.0f, 0.0f)},
        {Vector3(1.0f, 1.0f, -1.0f), Vector3(0.0f, 1.0f, 0.0f), Vector2(1.0f, 1.0f)},

        // Bottom face
        {Vector3(-1.0f, -1.0f, -1.0f), Vector3(0.0f, -1.0f, 0.0f), Vector2(1.0f, 1.0f)},
        {Vector3(1.0f, -1.0f, -1.0f), Vector3(0.0f, -1.0f, 0.0f), Vector2(0.0f, 1.0f)},
        {Vector3(1.0f, -1.0f, 1.0f), Vector3(0.0f, -1.0f, 0.0f), Vector2(0.0f, 0.0f)},
        {Vector3(-1.0f, -1.0f, 1.0f), Vector3(0.0f, -1.0f, 0.0f), Vector2(1.0f, 0.0f)},

        // Right face
        {Vector3(1.0f, -1.0f, -1.0f), Vector3(1.0f, 0.0f, 0.0f), Vector2(1.0f, 1.0f)},
        {Vector3(1.0f, 1.0f, -1.0f), Vector3(1.0f, 0.0f, 0.0f), Vector2(1.0f, 0.0f)},
        {Vector3(1.0f, 1.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f)},
        {Vector3(1.0f, -1.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f), Vector2(0.0f, 1.0f)},

        // Left face
        {Vector3(-1.0f, -1.0f, -1.0f), Vector3(-1.0f, 0.0f, 0.0f), Vector2(0.0f, 1.0f)},
        {Vector3(-1.0f, -1.0f, 1.0f), Vector3(-1.0f, 0.0f, 0.0f), Vector2(1.0f, 1.0f)},
        {Vector3(-1.0f, 1.0f, 1.0f), Vector3(-1.0f, 0.0f, 0.0f), Vector2(1.0f, 0.0f)},
        {Vector3(-1.0f,  1.0f, -1.0f), Vector3(-1.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f)}
    };
}

vector<uint32_t> Mesh::CreateCubeIndices()
{
    return {
        // Front face
        0, 1, 2, 0, 2, 3,
        // Back face
        4, 5, 6, 4, 6, 7,
        // Top face
        8, 9, 10, 8, 10, 11,
        // Bottom face
        12, 13, 14, 12, 14, 15,
        // Right face
        16, 17, 18, 16, 18, 19,
        // Left face
        20, 21, 22, 20, 22, 23
    };
}

vector<MeshVertex> Mesh::CreateTetrahedronVertices()
{
    const float a = 1.0f;
    return {
        // 사면체의 4개 정점
        {Vector3(0.0f,  a,  0.0f), Vector3(0.577f,  0.577f,  0.577f), Vector2(0.5f, 0.0f)}, // Top
        {Vector3(-a, -a,  a), Vector3(-0.577f, -0.577f, 0.577f), Vector2(0.0f, 1.0f)}, // Front left
        {Vector3(a, -a,  a), Vector3(0.577f, -0.577f,  0.577f), Vector2(1.0f, 1.0f)}, // Front right
        {Vector3(0.0f, -a, -a), Vector3(0.0f, -0.577f, -0.577f), Vector2(0.5f, 1.0f)} // Back
    };
}

vector<uint32_t> Mesh::CreateTetrahedronIndices()
{
    return {
        // 4개의 삼각형면으로 구성
        0, 1, 2,  // Front face
        0, 3, 1,  // Left face
        0, 2, 3,  // Right face
        1, 3, 2   // Bottom face
    };
}

} // namespace visualnnz