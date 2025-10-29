#pragma once
#include "LightManager.h"
#include <d3d11.h>
#include <directxmath.h>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include "Camera.h"

using namespace DirectX;

class Model
{
public:
    // 버텍스 구조체
    struct Vertex
    {
        XMFLOAT3 Position;
        XMFLOAT3 Normal;
        XMFLOAT2 TexCoord;
    };

    // 재질 구조체
    struct Material
    {
        std::string Name;
        XMFLOAT3 Ambient = { 0.2f, 0.2f, 0.2f };
        XMFLOAT3 Diffuse = { 0.8f, 0.8f, 0.8f };
        XMFLOAT3 Specular = { 1.0f, 1.0f, 1.0f };
        float Shininess = 32.0f;
        std::string DiffuseMapPath;
        ID3D11ShaderResourceView* DiffuseMap = nullptr;
    };

    // 메시 구조체 - 재질별로 분리된 메시 
    struct Mesh
    {
        std::string MaterialName;
        std::vector<Vertex> Vertices;
        std::vector<uint32_t> Indices;
        ID3D11Buffer* VertexBuffer = nullptr;
        ID3D11Buffer* IndexBuffer = nullptr;
        UINT IndexCount = 0;
    };

    // 모델 정보 구조체
    struct ModelInfo
    {
        std::string Name;
        std::string FilePath;
        std::string MtlFilePath;
        bool Visible = true;
        XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
        XMFLOAT3 Rotation = { 0.0f, 0.0f, 0.0f };
        XMFLOAT3 Scale = { 1.0f, 1.0f, 1.0f };
    };

    // 생성자 및 소멸자
    Model();
    ~Model();

    // OBJ 모델 로드 함수
    bool LoadObjModel(const std::string& filename, ID3D11Device* device);

    // MTL 파일 로드 함수
    bool LoadMaterialFromMTL(const std::string& mtlFilePath, ID3D11Device* device);

    // 텍스처 로드 함수
    bool LoadTexture(const std::string& texturePath, ID3D11Device* device, ID3D11ShaderResourceView** textureView);

    // Model.h의 Render 함수 선언 수정
    void Model::Render(ID3D11DeviceContext* deviceContext, const Camera& camera, LightManager* lightManager);

    // 모델 정보 getter/setter
    ModelInfo& GetModelInfo() { return modelInfo; }

    // 재질 정보 getter
    const std::map<std::string, Material>& GetMaterials() const { return materials; }

    // 리소스 해제
    void Release();

private:
    // 래스터라이저 상태
    ID3D11RasterizerState* rasterizerState = nullptr;

    // 버텍스 및 인덱스 버퍼 생성 함수
    bool CreateBuffers(ID3D11Device* device, Mesh& mesh);

    // 월드 변환 행렬 계산
    XMMATRIX CalculateWorldMatrix();

    // 셰이더 생성 함수
    bool CreateShaders(ID3D11Device* device);

    // 경로에서 디렉토리 추출
    std::string GetDirectoryFromPath(const std::string& filePath);

    // 메시 컬렉션
    std::vector<Mesh> meshes;

    // 재질 맵
    std::map<std::string, Material> materials;

    // 셰이더와 관련 리소스
    ID3D11VertexShader* vertexShader = nullptr;
    ID3D11PixelShader* pixelShader = nullptr;
    ID3D11InputLayout* inputLayout = nullptr;
    ID3D11Buffer* constantBuffer = nullptr;
    ID3D11SamplerState* samplerState = nullptr;

    // 모델 정보
    ModelInfo modelInfo;
};
