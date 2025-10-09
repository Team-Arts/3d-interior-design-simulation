#pragma once

#include <string>
#include <vector>
#include <memory>
#include <d3d11.h>
#include <wrl.h>
#include <directxtk/SimpleMath.h>

// Assimp 헤더 (GLB/GLTF 로딩용)
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace visualnnz
{
using Microsoft::WRL::ComPtr;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector2;
using std::shared_ptr;
using std::string;
using std::vector;

struct ModelVertex
{
    Vector3 position;
    Vector3 normal;
    Vector2 texCoord;
};

class Mesh;
class Material;
class InteriorObject;

class ModelLoader
{
public:
    ModelLoader();
    ~ModelLoader();

    // GLB/GLTF 파일 로드
    shared_ptr<InteriorObject> LoadModel(
        ComPtr<ID3D11Device> device,
        const string& filePath,
        const string& objectID);

    // asset/models/ 디렉터리의 모든 GLB 파일 목록 가져오기
    vector<string> GetAvailableModels() const;

private:
    // Assimp를 이용한 모델 처리
    shared_ptr<InteriorObject> ProcessAssimpNode(
        ComPtr<ID3D11Device> device,
        aiNode* node, 
        const aiScene* scene,
        const string& objectID);
        
    shared_ptr<Mesh> ProcessAssimpMesh(
        ComPtr<ID3D11Device> device,
        aiMesh* mesh, 
        const aiScene* scene);
        
    shared_ptr<Material> LoadMaterial(
        ComPtr<ID3D11Device> device,
        aiMaterial* mat);

    Assimp::Importer m_importer;
    string m_modelsDirectory;
};

} // namespace visualnnz