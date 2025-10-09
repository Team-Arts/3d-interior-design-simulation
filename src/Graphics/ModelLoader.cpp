#include "ModelLoader.h"
#include "Mesh.h"
#include "Material.h"
#include "../Scene/InteriorObject.h"
#include "../Components/MeshRenderer.h"
#include "../Components/Transform.h"

#include <filesystem>
#include <iostream>

namespace visualnnz
{
using namespace std;
using namespace DirectX::SimpleMath;

ModelLoader::ModelLoader()
    : m_modelsDirectory("asset/models/")
{
}

ModelLoader::~ModelLoader() = default;

shared_ptr<InteriorObject> ModelLoader::LoadModel(
    ComPtr<ID3D11Device> device,
    const string& filePath,
    const string& objectID)
{
    // Assimp로 파일 로드
    const aiScene* scene = m_importer.ReadFile(filePath, 
        aiProcess_Triangulate | 
        aiProcess_FlipUVs | 
        aiProcess_CalcTangentSpace |
        aiProcess_GenNormals);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        cout << "ERROR::ASSIMP:: " << m_importer.GetErrorString() << endl;
        return nullptr;
    }

    // 루트 노드부터 처리
    return ProcessAssimpNode(device, scene->mRootNode, scene, objectID);
}

shared_ptr<InteriorObject> ModelLoader::ProcessAssimpNode(
    ComPtr<ID3D11Device> device,
    aiNode* node, 
    const aiScene* scene,
    const string& objectID)
{
    auto interiorObject = make_shared<InteriorObject>();
    
    if (!interiorObject->Initialize(device, objectID, "loaded_model"))
    {
        return nullptr;
    }

    // 첫 번째 메시만 처리 (단순화)
    if (node->mNumMeshes > 0)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[0]];
        auto loadedMesh = ProcessAssimpMesh(device, mesh, scene);
        
        if (loadedMesh)
        {
            interiorObject->GetMeshRenderer()->SetMesh(loadedMesh);
        }
    }

    return interiorObject;
}

shared_ptr<Mesh> ModelLoader::ProcessAssimpMesh(
    ComPtr<ID3D11Device> device,
    aiMesh* mesh, 
    const aiScene* scene)
{
    vector<MeshVertex> vertices;
    vector<unsigned int> indices;

    // 정점 데이터 처리
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        ModelVertex modelVertex;
        
        // 위치
        modelVertex.position.x = mesh->mVertices[i].x;
        modelVertex.position.y = mesh->mVertices[i].y;
        modelVertex.position.z = mesh->mVertices[i].z;

        // 노멀
        if (mesh->HasNormals())
        {
            modelVertex.normal.x = mesh->mNormals[i].x;
            modelVertex.normal.y = mesh->mNormals[i].y;
            modelVertex.normal.z = mesh->mNormals[i].z;
        }

        // 텍스처 좌표
        if (mesh->mTextureCoords[0])
        {
            modelVertex.texCoord.x = mesh->mTextureCoords[0][i].x;
            modelVertex.texCoord.y = mesh->mTextureCoords[0][i].y;
        }
        else
        {
            modelVertex.texCoord = Vector2(0.0f, 0.0f);
        }

        // ModelVertex를 MeshVertex로 변환하여 추가
        MeshVertex meshVertex;
        meshVertex.position = modelVertex.position;
        meshVertex.normal = modelVertex.normal;
        meshVertex.texcoord = modelVertex.texCoord; // texCoord -> texcoord
        
        vertices.push_back(meshVertex);
    }

    // 인덱스 데이터 처리
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    // Mesh 객체 생성
    auto loadedMesh = make_shared<Mesh>();
    if (loadedMesh->Initialize(device, vertices, indices))
    {
        return loadedMesh;
    }

    return nullptr;
}

vector<string> ModelLoader::GetAvailableModels() const
{
    vector<string> modelFiles;
    
    try
    {
        for (const auto& entry : filesystem::directory_iterator(m_modelsDirectory))
        {
            if (entry.is_regular_file())
            {
                auto extension = entry.path().extension().string();
                if (extension == ".glb" || extension == ".gltf")
                {
                    modelFiles.push_back(entry.path().filename().string());
                }
            }
        }
    }
    catch (const filesystem::filesystem_error& ex)
    {
        cout << "Failed to scan models directory: " << ex.what() << endl;
    }

    return modelFiles;
}

} // namespace visualnnz