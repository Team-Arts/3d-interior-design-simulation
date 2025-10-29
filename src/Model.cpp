#include "Model.h"
#include "Camera.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include "WICTextureLoader11.h"  // DirectXTex의 텍스처 로더

// 상수 버퍼 구조체 
struct ConstantBuffer
{
    XMMATRIX World;
    XMMATRIX View;
    XMMATRIX Projection;
    XMFLOAT4 AmbientColor;
    XMFLOAT4 DiffuseColor;
    XMFLOAT4 SpecularColor;
    float Shininess;
    float HasTexture;
    XMFLOAT2 Padding;
};
// 조명을 지원하는 업데이트된 픽셀 셰이더
const char* pixelShaderCode = R"(
Texture2D diffuseTexture : register(t0);
SamplerState samLinear : register(s0);

// 모델용 상수 버퍼 (b0)
cbuffer ModelConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float4 AmbientColor;
    float4 DiffuseColor;
    float4 SpecularColor;
    float Shininess;
    float HasTexture;
    float2 Padding;
}

// 조명용 상수 버퍼 (b1)
cbuffer LightConstantBuffer : register(b1)
{
    struct {
        float4 Position;       // w 컴포넌트는 조명 타입(0: 방향성, 1: 점, 2: 스포트라이트)
        float4 Direction;      // 방향성 및 스포트라이트에 사용
        float4 Color;          // RGB 색상 및 강도
        float4 Factors;        // x: 반경, y: 감쇠, z: 스포트 내각, w: 스포트 외각
    } Lights[8];
    int LightCount;
    float3 LightPadding;
}

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Normal : NORMAL;
    float2 Tex : TEXCOORD0;
    float3 WorldPos : TEXCOORD1;
};

float3 CalculateDirectionalLight(float3 normal, float3 viewDir, int lightIndex)
{
    float3 lightDir = normalize(-Lights[lightIndex].Direction.xyz);
    float3 lightColor = Lights[lightIndex].Color.rgb;
    float intensity = Lights[lightIndex].Color.a;
    
    // 디퓨즈 계산
    float diff = max(dot(normal, lightDir), 0.0);
    float3 diffuse = diff * DiffuseColor.rgb * lightColor * intensity;
    
    // 스페큘러 계산
    float3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), Shininess);
    float3 specular = spec * SpecularColor.rgb * lightColor * intensity;
    
    return diffuse + specular;
}

float3 CalculatePointLight(float3 normal, float3 fragPos, float3 viewDir, int lightIndex)
{
    float3 lightPos = Lights[lightIndex].Position.xyz;
    float3 lightColor = Lights[lightIndex].Color.rgb;
    float intensity = Lights[lightIndex].Color.a;
    float range = Lights[lightIndex].Factors.x;
    float attenuation = Lights[lightIndex].Factors.y;
    
    // 물체에서 조명까지의 벡터
    float3 lightDir = normalize(lightPos - fragPos);
    
    // 디퓨즈 계산
    float diff = max(dot(normal, lightDir), 0.0);
    float3 diffuse = diff * DiffuseColor.rgb * lightColor * intensity;
    
    // 스페큘러 계산
    float3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), Shininess);
    float3 specular = spec * SpecularColor.rgb * lightColor * intensity;
    
    // 거리에 따른 감쇠
    float distance = length(lightPos - fragPos);
    float attenFactor = 1.0 / (1.0 + attenuation * (distance * distance / (range * range)));
    
    return (diffuse + specular) * attenFactor;
}

float3 CalculateSpotLight(float3 normal, float3 fragPos, float3 viewDir, int lightIndex)
{
    float3 lightPos = Lights[lightIndex].Position.xyz;
    float3 lightDir = normalize(lightPos - fragPos);
    float3 spotDir = normalize(Lights[lightIndex].Direction.xyz);
    float3 lightColor = Lights[lightIndex].Color.rgb;
    float intensity = Lights[lightIndex].Color.a;
    float range = Lights[lightIndex].Factors.x;
    float attenuation = Lights[lightIndex].Factors.y;
    float innerCone = cos(Lights[lightIndex].Factors.z);
    float outerCone = cos(Lights[lightIndex].Factors.w);
    
    // 디퓨즈 계산
    float diff = max(dot(normal, lightDir), 0.0);
    float3 diffuse = diff * DiffuseColor.rgb * lightColor * intensity;
    
    // 스페큘러 계산
    float3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), Shininess);
    float3 specular = spec * SpecularColor.rgb * lightColor * intensity;
    
    // 거리에 따른 감쇠
    float distance = length(lightPos - fragPos);
    float attenFactor = 1.0 / (1.0 + attenuation * (distance * distance / (range * range)));
    
    // 스포트라이트 효과 (원뿔 내부에 있는지 확인)
    float theta = dot(lightDir, -spotDir);
    float epsilon = innerCone - outerCone;
    float spotFactor = clamp((theta - outerCone) / epsilon, 0.0, 1.0);
    
    return (diffuse + specular) * attenFactor * spotFactor;
}

float4 main(PS_INPUT input) : SV_Target
{
    // 텍스처 샘플링
    float4 texColor = float4(1.0, 1.0, 1.0, 1.0);
    if (HasTexture > 0.5)
    {
        texColor = diffuseTexture.Sample(samLinear, input.Tex);
    }
    
    // 정규화
    float3 normal = normalize(input.Normal);
    float3 viewDir = normalize(float3(0.0, 0.0, -5.0) - input.WorldPos);
    
    // 최종 조명 계산
    float3 result = AmbientColor.rgb; // 앰비언트 조명 시작점
    
    // 사용 가능한 모든 조명 처리
    for (int i = 0; i < LightCount; i++)
    {
        int lightType = int(Lights[i].Position.w);
        
        if (lightType == 0) // 방향성 조명
        {
            result += CalculateDirectionalLight(normal, viewDir, i);
        }
        else if (lightType == 1) // 점 조명
        {
            result += CalculatePointLight(normal, input.WorldPos, viewDir, i);
        }
        else if (lightType == 2) // 스포트라이트
        {
            result += CalculateSpotLight(normal, input.WorldPos, viewDir, i);
        }
    }
    
    // 최종 색상 계산
    float4 finalColor = float4(result, 1.0) * texColor;
    
    // HDR 톤 매핑: 간단한 햅번 오퍼레이터
    finalColor.rgb = finalColor.rgb / (finalColor.rgb + float3(1.0, 1.0, 1.0));
    
    return finalColor;
}
)";

// 조명을 지원하는 업데이트된 버텍스 셰이더
const char* vertexShaderCode = R"(
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float4 AmbientColor;
    float4 DiffuseColor;
    float4 SpecularColor;
    float Shininess;
    float HasTexture;
    float2 Padding;
}

struct VS_INPUT
{
    float3 Pos : POSITION;
    float3 Normal : NORMAL;
    float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Normal : NORMAL;
    float2 Tex : TEXCOORD0;
    float3 WorldPos : TEXCOORD1;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = float4(input.Pos, 1.0f);
    
    // 월드 공간에서의 위치 계산 - 픽셀 셰이더에서 조명 계산에 사용
    output.WorldPos = mul(output.Pos, World).xyz;
    
    // 투영 변환
    output.Pos = mul(output.Pos, World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    
    // 월드 공간에서의 법선 계산
    output.Normal = mul(input.Normal, (float3x3)World);
    output.Normal = normalize(output.Normal);
    
    // 텍스처 좌표 전달
    output.Tex = input.Tex;
    
    return output;
}
)";
Model::Model()
{
}

Model::~Model()
{
    Release();
}

bool Model::LoadObjModel(const std::string& filename, ID3D11Device* device)
{
    // OBJ 파일에서 로드할 데이터
    std::vector<XMFLOAT3> positions;
    std::vector<XMFLOAT3> normals;
    std::vector<XMFLOAT2> texCoords;

    std::map<std::string, std::vector<uint32_t>> materialIndices; // 재질별 인덱스
    std::string currentMaterialName = "default";

    // 기본 재질 추가
    Material defaultMaterial;
    defaultMaterial.Name = "default";
    materials[defaultMaterial.Name] = defaultMaterial;

    // OBJ 파일 읽기
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open OBJ file: " << filename << std::endl;
        return false;
    }

    // 파일 경로에서 디렉토리 경로 추출
    std::string directory = GetDirectoryFromPath(filename);
    std::string mtlFilePath;

    std::string line;
    std::vector<uint32_t> posIndices, normalIndices, texCoordIndices;

    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string token;
        iss >> token;

        if (token == "v") // 정점 위치
        {
            XMFLOAT3 position;
            iss >> position.x >> position.y >> position.z;
            positions.push_back(position);
        }
        else if (token == "vn") // 정점 법선
        {
            XMFLOAT3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        else if (token == "vt") // 텍스처 좌표
        {
            XMFLOAT2 texCoord;
            iss >> texCoord.x >> texCoord.y;
            texCoord.y = 1.0f - texCoord.y; // DirectX 좌표계로 변환
            texCoords.push_back(texCoord);
        }
        else if (token == "f") // 면
        {
            std::string faceVertex;
            int vertexCount = 0;

            std::vector<uint32_t> facePositions, faceTexCoords, faceNormals;

            while (iss >> faceVertex)
            {
                vertexCount++;

                // '/'로 분리된 인덱스 처리
                std::replace(faceVertex.begin(), faceVertex.end(), '/', ' ');
                std::istringstream faceData(faceVertex);

                uint32_t posIndex, texCoordIndex, normalIndex;
                faceData >> posIndex;

                if (faceData.peek() != EOF) {
                    faceData >> texCoordIndex;
                }
                else {
                    texCoordIndex = 1; // 기본값
                }

                if (faceData.peek() != EOF) {
                    faceData >> normalIndex;
                }
                else {
                    normalIndex = 1; // 기본값
                }

                // OBJ 인덱스는 1부터 시작하므로 0부터 시작하도록 조정
                facePositions.push_back(posIndex - 1);
                faceTexCoords.push_back(texCoordIndex - 1);
                faceNormals.push_back(normalIndex - 1);
            }

            // 면을 삼각형으로 분할 (삼각형화)
            for (int i = 1; i < vertexCount - 1; i++)
            {
                // 첫 번째 정점
                posIndices.push_back(facePositions[0]);
                texCoordIndices.push_back(faceTexCoords[0]);
                normalIndices.push_back(faceNormals[0]);
                materialIndices[currentMaterialName].push_back(materialIndices[currentMaterialName].size());

                // 두 번째 정점
                posIndices.push_back(facePositions[i]);
                texCoordIndices.push_back(faceTexCoords[i]);
                normalIndices.push_back(faceNormals[i]);
                materialIndices[currentMaterialName].push_back(materialIndices[currentMaterialName].size());

                // 세 번째 정점
                posIndices.push_back(facePositions[i + 1]);
                texCoordIndices.push_back(faceTexCoords[i + 1]);
                normalIndices.push_back(faceNormals[i + 1]);
                materialIndices[currentMaterialName].push_back(materialIndices[currentMaterialName].size());
            }
        }
        else if (token == "mtllib") // 재질 라이브러리
        {
            iss >> mtlFilePath;
            mtlFilePath = directory + mtlFilePath;
            modelInfo.MtlFilePath = mtlFilePath;
        }
        else if (token == "usemtl") // 재질 사용
        {
            iss >> currentMaterialName;

            // 해당 재질이 없으면 기본 재질 사용
            if (materials.find(currentMaterialName) == materials.end())
            {
                Material newMaterial;
                newMaterial.Name = currentMaterialName;
                materials[currentMaterialName] = newMaterial;
            }
        }
    }

    file.close();

    // texCoords나 normals가 없는 경우 기본값 추가
    if (texCoords.empty()) {
        texCoords.push_back(XMFLOAT2(0.0f, 0.0f));
    }
    if (normals.empty()) {
        normals.push_back(XMFLOAT3(0.0f, 1.0f, 0.0f));
    }

    // MTL 파일 로드
    if (!mtlFilePath.empty())
    {
        LoadMaterialFromMTL(mtlFilePath, device);
    }

    // 메시 생성
    for (const auto& matPair : materialIndices)
    {
        Mesh mesh;
        mesh.MaterialName = matPair.first;

        // 해당 재질의 인덱스 수 저장
        mesh.IndexCount = static_cast<UINT>(matPair.second.size());

        // 버텍스 및 인덱스 생성
        for (size_t i = 0; i < matPair.second.size(); i++)
        {
            uint32_t idx = matPair.second[i];

            Vertex vertex;
            vertex.Position = positions[posIndices[idx]];

            // 텍스처 좌표 (있는 경우)
            if (texCoordIndices[idx] < texCoords.size()) {
                vertex.TexCoord = texCoords[texCoordIndices[idx]];
            }
            else {
                vertex.TexCoord = XMFLOAT2(0.0f, 0.0f);
            }

            // 법선 (있는 경우)
            if (normalIndices[idx] < normals.size()) {
                vertex.Normal = normals[normalIndices[idx]];
            }
            else {
                vertex.Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
            }

            mesh.Vertices.push_back(vertex);
            mesh.Indices.push_back(static_cast<uint32_t>(i));
        }

        // 버퍼 생성
        CreateBuffers(device, mesh);

        // 메시 추가
        meshes.push_back(mesh);
    }

    // 모델 정보 설정
    modelInfo.Name = filename.substr(filename.find_last_of("/\\") + 1);
    modelInfo.FilePath = filename;

    // 모델의 바운딩 박스 계산 및 자동 크기 조정
    if (!positions.empty())
    {
        XMFLOAT3 minPos = positions[0];
        XMFLOAT3 maxPos = positions[0];

        for (const auto& pos : positions)
        {
            minPos.x = min(minPos.x, pos.x);
            minPos.y = min(minPos.y, pos.y);
            minPos.z = min(minPos.z, pos.z);

            maxPos.x = max(maxPos.x, pos.x);
            maxPos.y = max(maxPos.y, pos.y);
            maxPos.z = max(maxPos.z, pos.z);
        }

        // 모델 중심 계산
        XMFLOAT3 center;
        center.x = (minPos.x + maxPos.x) * 0.5f;
        center.y = (minPos.y + maxPos.y) * 0.5f;
        center.z = (minPos.z + maxPos.z) * 0.5f;

        // 모델 크기 계산
        float sizeX = maxPos.x - minPos.x;
        float sizeY = maxPos.y - minPos.y;
        float sizeZ = maxPos.z - minPos.z;
        float maxSize = max(max(sizeX, sizeY), sizeZ);

        // 모델 위치와 크기 조정 - 직접 중심에 배치하도록 수정
        modelInfo.Position = XMFLOAT3(0.0f, 0.0f, 0.0f); // 원점에 배치
        float scale = 2.0f / maxSize; // 적절한 크기로 조정
        modelInfo.Scale = XMFLOAT3(scale, scale, scale);

        // 모델 데이터를 원점 기준으로 수정
        for (auto& mesh : meshes)
        {
            for (auto& vertex : mesh.Vertices)
            {
                vertex.Position.x -= center.x;
                vertex.Position.y -= center.y;
                vertex.Position.z -= center.z;
            }

            // 버퍼 재생성
            if (mesh.VertexBuffer)
            {
                mesh.VertexBuffer->Release();
                mesh.VertexBuffer = nullptr;
            }

            if (mesh.IndexBuffer)
            {
                mesh.IndexBuffer->Release();
                mesh.IndexBuffer = nullptr;
            }

            // 버퍼 생성
            CreateBuffers(device, mesh);
        }
    }

    // 셰이더 생성
    if (!CreateShaders(device))
    {
        return false;
    }

    // 래스터라이저 상태 생성
    D3D11_RASTERIZER_DESC rastDesc;
    ZeroMemory(&rastDesc, sizeof(rastDesc));
    rastDesc.FillMode = D3D11_FILL_SOLID;
    rastDesc.CullMode = D3D11_CULL_NONE; // 양면 렌더링
    rastDesc.FrontCounterClockwise = FALSE;
    device->CreateRasterizerState(&rastDesc, &rasterizerState);

    // 샘플러 상태 생성
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    HRESULT hr = device->CreateSamplerState(&sampDesc, &samplerState);
    if (FAILED(hr))
    {
        OutputDebugStringA("Failed to create sampler state.\n");
        return false;
    }

    OutputDebugStringA(("Model loaded: " + filename + "\n").c_str());
    OutputDebugStringA(("Mesh count: " + std::to_string(meshes.size()) + "\n").c_str());

    return true;
}

bool Model::LoadMaterialFromMTL(const std::string& mtlFilePath, ID3D11Device* device)
{
    std::ifstream file(mtlFilePath);
    if (!file.is_open())
    {
        std::cerr << "Failed to open MTL file: " << mtlFilePath << std::endl;
        return false;
    }

    std::string directory = GetDirectoryFromPath(mtlFilePath);
    std::string currentMaterialName;

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string token;
        iss >> token;

        if (token == "newmtl") // 새로운 재질
        {
            iss >> currentMaterialName;

            // 해당 재질이 없으면 추가
            if (materials.find(currentMaterialName) == materials.end())
            {
                Material newMaterial;
                newMaterial.Name = currentMaterialName;
                materials[currentMaterialName] = newMaterial;
            }
        }
        else if (token == "Ka") // 앰비언트 색상
        {
            if (!currentMaterialName.empty())
            {
                iss >> materials[currentMaterialName].Ambient.x >> materials[currentMaterialName].Ambient.y >> materials[currentMaterialName].Ambient.z;
            }
        }
        else if (token == "Kd") // 디퓨즈 색상
        {
            if (!currentMaterialName.empty())
            {
                iss >> materials[currentMaterialName].Diffuse.x >> materials[currentMaterialName].Diffuse.y >> materials[currentMaterialName].Diffuse.z;
            }
        }
        else if (token == "Ks") // 스페큘러 색상
        {
            if (!currentMaterialName.empty())
            {
                iss >> materials[currentMaterialName].Specular.x >> materials[currentMaterialName].Specular.y >> materials[currentMaterialName].Specular.z;
            }
        }
        else if (token == "Ns") // 스페큘러 지수 (광택)
        {
            if (!currentMaterialName.empty())
            {
                iss >> materials[currentMaterialName].Shininess;
            }
        }
        else if (token == "map_Kd") // 디퓨즈 텍스처 맵
        {
            if (!currentMaterialName.empty())
            {
                std::string texturePath;
                iss >> texturePath;
                texturePath = directory + texturePath;
                materials[currentMaterialName].DiffuseMapPath = texturePath;

                // 텍스처 로드
                LoadTexture(texturePath, device, &materials[currentMaterialName].DiffuseMap);
            }
        }
    }

    file.close();
    return true;
}

bool Model::LoadTexture(const std::string& texturePath, ID3D11Device* device, ID3D11ShaderResourceView** textureView)
{
    if (texturePath.empty())
    {
        return false;
    }

    // 텍스처 로드
    HRESULT hr = DirectX::CreateWICTextureFromFile(device, std::wstring(texturePath.begin(), texturePath.end()).c_str(), nullptr, textureView);
    if (FAILED(hr))
    {
        std::cerr << "Failed to load texture: " << texturePath << std::endl;
        return false;
    }

    return true;
}

bool Model::CreateBuffers(ID3D11Device* device, Mesh& mesh)
{
    // 버텍스 버퍼 생성
    D3D11_BUFFER_DESC vbDesc;
    ZeroMemory(&vbDesc, sizeof(vbDesc));
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * mesh.Vertices.size());
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA vbData;
    ZeroMemory(&vbData, sizeof(vbData));
    vbData.pSysMem = mesh.Vertices.data();

    HRESULT hr = device->CreateBuffer(&vbDesc, &vbData, &mesh.VertexBuffer);
    if (FAILED(hr))
    {
        return false;
    }

    // 인덱스 버퍼 생성
    D3D11_BUFFER_DESC ibDesc;
    ZeroMemory(&ibDesc, sizeof(ibDesc));
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * mesh.Indices.size());
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA ibData;
    ZeroMemory(&ibData, sizeof(ibData));
    ibData.pSysMem = mesh.Indices.data();

    hr = device->CreateBuffer(&ibDesc, &ibData, &mesh.IndexBuffer);
    if (FAILED(hr))
    {
        return false;
    }

    return true;
}

bool Model::CreateShaders(ID3D11Device* device)
{
    // 셰이더 컴파일 및 생성
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    // 정점 셰이더 컴파일
    HRESULT hr = D3DCompile(vertexShaderCode, strlen(vertexShaderCode), "VS", nullptr, nullptr, "main", "vs_4_0", 0, 0, &vsBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        return false;
    }

    // 픽셀 셰이더 컴파일
    hr = D3DCompile(pixelShaderCode, strlen(pixelShaderCode), "PS", nullptr, nullptr, "main", "ps_4_0", 0, 0, &psBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        if (vsBlob) vsBlob->Release();
        return false;
    }

    // 셰이더 생성
    hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
    if (FAILED(hr))
    {
        vsBlob->Release();
        psBlob->Release();
        return false;
    }

    hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);
    if (FAILED(hr))
    {
        vsBlob->Release();
        psBlob->Release();
        return false;
    }

    // 입력 레이아웃 생성
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    hr = device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);
    vsBlob->Release();
    psBlob->Release();
    if (FAILED(hr))
    {
        return false;
    }

    // 상수 버퍼 생성
    D3D11_BUFFER_DESC cbDesc;
    ZeroMemory(&cbDesc, sizeof(cbDesc));
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.ByteWidth = sizeof(ConstantBuffer);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    hr = device->CreateBuffer(&cbDesc, nullptr, &constantBuffer);
    if (FAILED(hr))
    {
        return false;
    }

    return true;
}

// Model.cpp 수정 - Render 함수 수정
void Model::Render(ID3D11DeviceContext* deviceContext, const Camera& camera, LightManager* lightManager)
{
    if (!modelInfo.Visible || meshes.empty())
        return;

    // 셰이더 설정
    deviceContext->VSSetShader(vertexShader, nullptr, 0);
    deviceContext->PSSetShader(pixelShader, nullptr, 0);
    deviceContext->IASetInputLayout(inputLayout);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 샘플러 상태 설정
    deviceContext->PSSetSamplers(0, 1, &this->samplerState);

    // 각 메시별로 렌더링
    for (const auto& mesh : meshes)
    {
        if (!mesh.VertexBuffer || !mesh.IndexBuffer)
            continue;

        // 재질 가져오기
        Material* material = nullptr;
        auto it = materials.find(mesh.MaterialName);
        if (it != materials.end())
        {
            material = &it->second;
        }
        else
        {
            material = &materials["default"];
        }

        // 상수 버퍼 업데이트
        ConstantBuffer cb;
        cb.World = XMMatrixTranspose(CalculateWorldMatrix());
        cb.View = XMMatrixTranspose(camera.GetViewMatrix());
        cb.Projection = XMMatrixTranspose(camera.GetProjectionMatrix());

        // 재질 정보 설정
        cb.AmbientColor = XMFLOAT4(material->Ambient.x, material->Ambient.y, material->Ambient.z, 1.0f);
        cb.DiffuseColor = XMFLOAT4(material->Diffuse.x, material->Diffuse.y, material->Diffuse.z, 1.0f);
        cb.SpecularColor = XMFLOAT4(material->Specular.x, material->Specular.y, material->Specular.z, 1.0f);
        cb.Shininess = material->Shininess;

        // 텍스처 유무 설정
        cb.HasTexture = (material->DiffuseMap != nullptr) ? 1.0f : 0.0f;

        deviceContext->UpdateSubresource(constantBuffer, 0, nullptr, &cb, 0, 0);
        deviceContext->VSSetConstantBuffers(0, 1, &constantBuffer);
        deviceContext->PSSetConstantBuffers(0, 1, &constantBuffer);

        // 텍스처 설정
        if (material->DiffuseMap)
        {
            deviceContext->PSSetShaderResources(0, 1, &material->DiffuseMap);
        }

        // 조명 정보 설정
         if (lightManager)
         {
            lightManager->UpdateLightBuffer(deviceContext);
            lightManager->SetLightBuffer(deviceContext);
         }
        // 버텍스 및 인덱스 버퍼 설정
        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        deviceContext->IASetVertexBuffers(0, 1, &mesh.VertexBuffer, &stride, &offset);
        deviceContext->IASetIndexBuffer(mesh.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

        // 래스터라이저 상태 설정
        deviceContext->RSSetState(rasterizerState);

        // 그리기
        deviceContext->DrawIndexed(mesh.IndexCount, 0, 0);
    }
}

XMMATRIX Model::CalculateWorldMatrix()
{
    // 월드 행렬 계산 - 순서가 중요합니다 (Scale -> Rotation -> Translation)
    XMMATRIX scale = XMMatrixScaling(modelInfo.Scale.x, modelInfo.Scale.y, modelInfo.Scale.z);
    XMMATRIX rotation = XMMatrixRotationRollPitchYaw(
        XMConvertToRadians(modelInfo.Rotation.x),
        XMConvertToRadians(modelInfo.Rotation.y),
        XMConvertToRadians(modelInfo.Rotation.z));
    XMMATRIX translation = XMMatrixTranslation(modelInfo.Position.x, modelInfo.Position.y, modelInfo.Position.z);

    // SRT 순서대로 행렬 곱셈
    return scale * rotation * translation;
}

std::string Model::GetDirectoryFromPath(const std::string& filePath)
{
    size_t pos = filePath.find_last_of("/\\");
    if (pos != std::string::npos)
    {
        return filePath.substr(0, pos + 1);
    }
    return "";
}

void Model::Release()
{
    // 메시 버퍼 해제
    for (auto& mesh : meshes)
    {
        if (mesh.VertexBuffer) { mesh.VertexBuffer->Release(); mesh.VertexBuffer = nullptr; }
        if (mesh.IndexBuffer) { mesh.IndexBuffer->Release(); mesh.IndexBuffer = nullptr; }
    }

    // 재질 텍스처 해제
    for (auto& material : materials)
    {
        if (material.second.DiffuseMap) { material.second.DiffuseMap->Release(); material.second.DiffuseMap = nullptr; }
    }

    // 셰이더 및 관련 리소스 해제
    if (vertexShader) { vertexShader->Release(); vertexShader = nullptr; }
    if (pixelShader) { pixelShader->Release(); pixelShader = nullptr; }
    if (inputLayout) { inputLayout->Release(); inputLayout = nullptr; }
    if (constantBuffer) { constantBuffer->Release(); constantBuffer = nullptr; }
    if (rasterizerState) { rasterizerState->Release(); rasterizerState = nullptr; }
    if (samplerState) { samplerState->Release(); samplerState = nullptr; }
}
