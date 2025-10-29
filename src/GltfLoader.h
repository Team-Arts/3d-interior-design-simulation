#pragma once
#include <d3d11.h>
#include <directxmath.h>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include "Camera.h"
#include "Model.h"
#include "Common.h"
// 구현 매크로 없이 tinygltf를 포함 
#include "tiny_gltf.h"


using namespace DirectX;

// GLB 모델 관련 구조체 및 클래스 정의 
class GltfLoader
{
public:
    // 버텍스 구조체 (GLB 포맷에 맞게 확장)
    struct Vertex
    {
        XMFLOAT3 Position;
        XMFLOAT3 Normal;
        XMFLOAT2 TexCoord;
        XMFLOAT4 Weights;    // 스킨 애니메이션을 위한 가중치
        XMUINT4 Joints;      // 스킨 애니메이션을 위한 조인트 인덱스
        XMFLOAT4 Tangent;    // PBR 재질을 위한 탄젠트
    };

    // PBR 재질 구조체
    struct PbrMaterial
    {
        std::string Name;
        XMFLOAT4 BaseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
        float MetallicFactor = 1.0f;
        float RoughnessFactor = 1.0f;
        XMFLOAT3 EmissiveFactor = { 0.0f, 0.0f, 0.0f };

        // 텍스처 맵
        std::string BaseColorTexturePath;
        std::string MetallicRoughnessTexturePath;
        std::string NormalTexturePath;
        std::string EmissiveTexturePath;
        std::string OcclusionTexturePath;

        // 텍스처 리소스 뷰
        ID3D11ShaderResourceView* BaseColorTexture = nullptr;
        ID3D11ShaderResourceView* MetallicRoughnessTexture = nullptr;
        ID3D11ShaderResourceView* NormalTexture = nullptr;
        ID3D11ShaderResourceView* EmissiveTexture = nullptr;
        ID3D11ShaderResourceView* OcclusionTexture = nullptr;
    };

    // 메시 프리미티브 구조체
    struct MeshPrimitive
    {
        std::string MaterialName;
        std::vector<Vertex> Vertices;
        std::vector<uint32_t> Indices;
        ID3D11Buffer* VertexBuffer = nullptr;
        ID3D11Buffer* IndexBuffer = nullptr;
        UINT IndexCount = 0;
    };

    // 노드 구조체 (계층 구조 지원)
    struct Node
    {
        std::string Name;
        XMMATRIX LocalTransform;
        std::vector<int> Children;
        int MeshIndex = -1; // -1은 메시가 없음을 의미
    };

    // 메시 구조체
    struct Mesh
    {
        std::string Name;
        std::vector<MeshPrimitive> Primitives;
        std::vector<Model::Vertex> Vertices;
        std::vector<uint32_t> Indices;
        ID3D11Buffer* VertexBuffer = nullptr;
        ID3D11Buffer* IndexBuffer = nullptr;
        UINT IndexCount = 0;
        std::string MaterialName;
    };

    // 애니메이션 관련 구조체
    struct AnimationChannel
    {
        int NodeIndex;
        enum TargetPath { TRANSLATION, ROTATION, SCALE, WEIGHTS } Path;
        std::vector<float> Times;
        std::vector<XMFLOAT4> Values; // XMFLOAT4는 모든 애니메이션 데이터 타입을 저장할 수 있음
    };

    struct Animation
    {
        std::string Name;
        std::vector<AnimationChannel> Channels;
        float StartTime = 0.0f;
        float EndTime = 0.0f;
    };

    // 모델 정보 구조체
    struct ModelInfo
    {
        std::string Name;
        std::string FilePath;
        bool Visible = true;
        XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
        XMFLOAT3 Rotation = { 0.0f, 0.0f, 0.0f };
        XMFLOAT3 Scale = { 1.0f, 1.0f, 1.0f };
    };


public:
    GltfLoader();
    ~GltfLoader();

   
    // GLB 모델 로드 함수
    bool LoadGlbModel(const std::string& filename, ID3D11Device* device);

    // 조명 지원 렌더링 함수 추가
    void Render(ID3D11DeviceContext* deviceContext, const Camera& camera, LightManager* lightManager);


    // 모델 렌더링 함수
    void Render(ID3D11DeviceContext* deviceContext, const Camera& camera);

    // 애니메이션 업데이트 함수
    void UpdateAnimation(float deltaTime);

    // 모델 정보 getter/setter
    ModelInfo& GetModelInfo() { return modelInfo; }

    // 재질 정보 getter
    const std::map<std::string, PbrMaterial>& GetMaterials() const { return materials; }

    // 애니메이션 관련 함수들
    int GetCurrentAnimationIndex() const;
    float GetCurrentAnimationTime() const;
    const std::vector<Animation>& GetAnimations() const;
    void SetCurrentAnimation(int index);
    bool IsAnimationPlaying() const;
    void PlayAnimation();
    void PauseAnimation();
    void SetAnimationSpeed(float speed);
    float GetAnimationSpeed() const;

    // 리소스 해제
    void Release();

private:
    void GltfLoader::AutoResizeModel();
    BoundingBox CalculateBoundingBox() const;

    // 노드 렌더링 함수
    void RenderNode(ID3D11DeviceContext* deviceContext, const Camera& camera,
        int nodeIndex, XMMATRIX parentTransform);

    // GLB 모델 처리 함수
    bool ProcessGltfModel(const tinygltf::Model& model, ID3D11Device* device);

    // 텍스처 로드 함수
    bool LoadTexture(const std::string& texturePath, ID3D11Device* device, ID3D11ShaderResourceView** textureView);
    bool LoadTextureFromBuffer(const tinygltf::Image& image, ID3D11Device* device, ID3D11ShaderResourceView** textureView);

    // 버퍼 생성 함수
    bool CreateBuffers(ID3D11Device* device, MeshPrimitive& primitive);

    // 셰이더 생성 함수
    bool CreateShaders(ID3D11Device* device);

    // 노드 변환 행렬 계산
    XMMATRIX CalculateNodeTransform(int nodeIndex);

    // 월드 변환 행렬 계산
    XMMATRIX CalculateWorldMatrix();

private:
    // 모델 데이터
    std::vector<Mesh> meshes;
    std::vector<Node> nodes;
    std::map<std::string, PbrMaterial> materials;
    std::vector<Animation> animations;

    // 루트 노드 인덱스
    std::vector<int> rootNodes;

    // 현재 애니메이션 상태
    int currentAnimationIndex = -1;
    float currentAnimationTime = 0.0f;
    bool animationPlaying = false;  // 애니메이션 재생 상태
    float animationSpeed = 1.0f;    // 애니메이션 재생 속도

    // 셰이더와 관련 리소스
    ID3D11VertexShader* vertexShader = nullptr;
    ID3D11PixelShader* pixelShader = nullptr;
    ID3D11InputLayout* inputLayout = nullptr;
    ID3D11Buffer* constantBuffer = nullptr;
    ID3D11SamplerState* samplerState = nullptr;
    ID3D11RasterizerState* rasterizerState = nullptr;

    // 모델 정보
    ModelInfo modelInfo;
};
