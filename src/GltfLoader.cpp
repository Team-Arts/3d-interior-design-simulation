#include "GltfLoader.h"
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include <iostream>
#include <algorithm>
#include "WICTextureLoader11.h"

namespace tinygltf {
    bool WriteImageData(const std::string* basedir, const std::string* filename,
        const Image* image, bool embedImages,
        const FsCallbacks* fsCallbacks, const URICallbacks* uriCallbacks,
        std::string* out_uri, void* user_data) {
        // 빈 구현 - 항상 성공으로 처리
        return true;
    }
}
#ifndef TINYGLTF_IMPLEMENTATION_DEFINED
#define TINYGLTF_IMPLEMENTATION_DEFINED
#define TINYGLTF_NO_EXTERNAL_IMAGE  // 외부 이미지 기능 비활성화
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#endif
#include "tiny_gltf.h"
// 상수 버퍼 구조체 (PBR 셰이딩을 위해 확장)
struct ConstantBuffer
{
    XMMATRIX World;
    XMMATRIX View;
    XMMATRIX Projection;
    XMFLOAT4 BaseColorFactor;
    XMFLOAT3 EmissiveFactor;
    float MetallicFactor;
    float RoughnessFactor;
    float HasBaseColorTexture;
    float HasMetallicRoughnessTexture;
    float HasNormalTexture;
    float HasEmissiveTexture;
    float HasOcclusionTexture;
    XMFLOAT3 Padding;
};

// PBR 버텍스 셰이더 코드
const char* glbVertexShaderCode = R"(
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float4 BaseColorFactor;
    float3 EmissiveFactor;
    float MetallicFactor;
    float RoughnessFactor;
    float HasBaseColorTexture;
    float HasMetallicRoughnessTexture;
    float HasNormalTexture;
    float HasEmissiveTexture;
    float HasOcclusionTexture;
    float3 Padding;
}

struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;
    float4 Weights : WEIGHTS;
    uint4 Joints : JOINTS;
    float4 Tangent : TANGENT;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;
    float3 WorldPos : TEXCOORD1;
    float3 Tangent : TEXCOORD2;
    float3 Bitangent : TEXCOORD3;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;
    
    // 위치 변환
    float4 pos = float4(input.Position, 1.0f);
    output.WorldPos = mul(pos, World).xyz;
    output.Position = mul(pos, World);
    output.Position = mul(output.Position, View);
    output.Position = mul(output.Position, Projection);
    
    // 법선 변환
    output.Normal = normalize(mul(input.Normal, (float3x3)World));
    
    // 탄젠트 및 바이탄젠트 계산 (법선 매핑용)
    float3 tangent = normalize(mul(input.Tangent.xyz, (float3x3)World));
    output.Tangent = tangent;
    output.Bitangent = cross(output.Normal, tangent) * input.Tangent.w;
    
    // 텍스처 좌표 전달
    output.TexCoord = input.TexCoord;
    
    return output;
}
)";

// PBR 픽셀 셰이더 코드
const char* glbPixelShaderCode = R"(
Texture2D baseColorTexture : register(t0);
Texture2D metallicRoughnessTexture : register(t1);
Texture2D normalTexture : register(t2);
Texture2D emissiveTexture : register(t3);
Texture2D occlusionTexture : register(t4);
SamplerState samplerState : register(s0);

cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float4 BaseColorFactor;
    float3 EmissiveFactor;
    float MetallicFactor;
    float RoughnessFactor;
    float HasBaseColorTexture;
    float HasMetallicRoughnessTexture;
    float HasNormalTexture;
    float HasEmissiveTexture;
    float HasOcclusionTexture;
    float3 Padding;
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
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;
    float3 WorldPos : TEXCOORD1;
    float3 Tangent : TEXCOORD2;
    float3 Bitangent : TEXCOORD3;
};

// PBR 계산 함수들
float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265359 * denom * denom;
    
    return nom / max(denom, 0.001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / max(denom, 0.001);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// 조명 계산 함수 추가
float3 CalculateDirectionalLight(float3 normal, float3 viewDir, int lightIndex, float3 albedo, float metallic, float roughness)
{
    float3 lightDir = normalize(-Lights[lightIndex].Direction.xyz);
    float3 lightColor = Lights[lightIndex].Color.rgb;
    float intensity = Lights[lightIndex].Color.a;
    
    // 디퓨즈 계산
    float diff = max(dot(normal, lightDir), 0.0);
    float3 diffuse = diff * albedo * lightColor * intensity;
    
    // 스페큘러 계산
    float3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 16.0);
    float3 specular = spec * float3(0.04, 0.04, 0.04) * lightColor * intensity;
    
    return diffuse + specular;
}

float3 CalculatePointLight(float3 normal, float3 worldPos, float3 viewDir, int lightIndex, float3 albedo, float metallic, float roughness)
{
    float3 lightPos = Lights[lightIndex].Position.xyz;
    float3 lightColor = Lights[lightIndex].Color.rgb;
    float intensity = Lights[lightIndex].Color.a;
    float range = Lights[lightIndex].Factors.x;
    float attenuation = Lights[lightIndex].Factors.y;
    
    // 물체에서 조명까지의 벡터
    float3 lightDir = normalize(lightPos - worldPos);
    
    // 디퓨즈 계산
    float diff = max(dot(normal, lightDir), 0.0);
    float3 diffuse = diff * albedo * lightColor * intensity;
    
    // 스페큘러 계산
    float3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 16.0);
    float3 specular = spec * float3(0.04, 0.04, 0.04) * lightColor * intensity;
    
    // 거리에 따른 감쇠
    float distance = length(lightPos - worldPos);
    float attenFactor = 1.0 / (1.0 + attenuation * (distance * distance / (range * range)));
    
    return (diffuse + specular) * attenFactor;
}

float4 main(PS_INPUT input) : SV_Target
{
    // 텍스처에서 값 샘플링
    float4 baseColor = BaseColorFactor;
    if (HasBaseColorTexture > 0.5)
    {
        baseColor *= baseColorTexture.Sample(samplerState, input.TexCoord);
    }
    
    float metallic = MetallicFactor;
    float roughness = RoughnessFactor;
    if (HasMetallicRoughnessTexture > 0.5)
    {
        float4 metallicRoughnessSample = metallicRoughnessTexture.Sample(samplerState, input.TexCoord);
        roughness *= metallicRoughnessSample.g;
        metallic *= metallicRoughnessSample.b;
    }
    
    float3 normal = normalize(input.Normal);
    if (HasNormalTexture > 0.5)
    {
        // 노멀 맵에서 노멀 추출
        float3 normalSample = normalTexture.Sample(samplerState, input.TexCoord).rgb * 2.0 - 1.0;
        
        // TBN 행렬 생성
        float3 N = normal;
        float3 T = normalize(input.Tangent);
        float3 B = normalize(input.Bitangent);
        float3x3 TBN = float3x3(T, B, N);
        
        // 노멀 맵의 노멀을 월드 공간으로 변환
        normal = normalize(mul(normalSample, TBN));
    }
    
    float ambientOcclusion = 1.0;
    if (HasOcclusionTexture > 0.5)
    {
        ambientOcclusion = occlusionTexture.Sample(samplerState, input.TexCoord).r;
    }
    
    float3 emissive = EmissiveFactor;
    if (HasEmissiveTexture > 0.5)
    {
        emissive *= emissiveTexture.Sample(samplerState, input.TexCoord).rgb;
    }
    
    // 간단한 PBR 렌더링 (완전한 PBR은 복잡하므로 여기서는 간략화)
    float3 lightDir = normalize(float3(0.5, 0.5, -0.5));
    float3 viewDir = normalize(float3(0.0, 0.0, -5.0) - input.WorldPos);
    float3 halfVector = normalize(lightDir + viewDir);
    
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, baseColor.rgb, metallic);
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(normal, halfVector, roughness);
    float G = GeometrySmith(normal, viewDir, lightDir, roughness);
    float3 F = fresnelSchlick(max(dot(halfVector, viewDir), 0.0), F0);
    
    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - metallic;
    
    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0) + 0.001;
    float3 specular = numerator / denominator;
    
    float NdotL = max(dot(normal, lightDir), 0.0);
    
    // 최종 색상 계산
    float3 ambient = float3(0.03, 0.03, 0.03) * baseColor.rgb * ambientOcclusion;
    float3 color = ambient + (kD * baseColor.rgb / 3.14159265359 + specular) * NdotL + emissive;

    // 여기에 조명 계산 추가
// 조명 관리자의 조명 데이터가 있는 경우
if (LightCount > 0) {
    // 기본 방향성 조명 계산 부분은 제거 (조명 관리자로 대체)
    for (int i = 0; i < LightCount; i++) {
        int lightType = int(Lights[i].Position.w);
        
        if (lightType == 0) // 방향성 조명
        {
            // 방향성 조명의 방향과 색상 가져오기
            float3 dynLightDir = normalize(-Lights[i].Direction.xyz);
            float3 dynLightColor = Lights[i].Color.rgb;
            float dynIntensity = Lights[i].Color.a;
            
            // 방향성 조명에 대한 하프 벡터 계산
            float3 dynHalfVector = normalize(dynLightDir + viewDir);
            
            // 수정된 PBR 계산
            float dynNDF = DistributionGGX(normal, dynHalfVector, roughness);
            float dynG = GeometrySmith(normal, viewDir, dynLightDir, roughness);
            float3 dynF = fresnelSchlick(max(dot(dynHalfVector, viewDir), 0.0), F0);
            
            float3 dynKS = dynF;
            float3 dynKD = float3(1.0, 1.0, 1.0) - dynKS;
            dynKD *= 1.0 - metallic;
            
            float3 dynNumerator = dynNDF * dynG * dynF;
            float dynDenominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, dynLightDir), 0.0) + 0.001;
            float3 dynSpecular = dynNumerator / dynDenominator;
            
            float dynNdotL = max(dot(normal, dynLightDir), 0.0);
            
            // 조명 기여분 추가
            color += (dynKD * baseColor.rgb / 3.14159265359 + dynSpecular) * dynNdotL * dynLightColor * dynIntensity;
        }
        else if (lightType == 1) // 점 조명
        {
            // 점 조명 계산
            float3 lightPos = Lights[i].Position.xyz;
            float3 lightColor = Lights[i].Color.rgb;
            float intensity = Lights[i].Color.a;
            float range = Lights[i].Factors.x;
            float attenuation = Lights[i].Factors.y;
            
            // 거리 및 방향 계산
            float3 dirToLight = lightPos - input.WorldPos;
            float distance = length(dirToLight);
            float3 ptLightDir = normalize(dirToLight);
            
            // 거리에 기반한 감쇠
            float attFactor = 1.0 / (1.0 + attenuation * (distance * distance / (range * range)));
            
            // 점 조명에 대한 하프 벡터 계산
            float3 ptHalfVector = normalize(ptLightDir + viewDir);
            
            // 점 조명에 대한 PBR 계산
            float ptNDF = DistributionGGX(normal, ptHalfVector, roughness);
            float ptG = GeometrySmith(normal, viewDir, ptLightDir, roughness);
            float3 ptF = fresnelSchlick(max(dot(ptHalfVector, viewDir), 0.0), F0);
            
            float3 ptKS = ptF;
            float3 ptKD = float3(1.0, 1.0, 1.0) - ptKS;
            ptKD *= 1.0 - metallic;
            
            float3 ptNumerator = ptNDF * ptG * ptF;
            float ptDenominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, ptLightDir), 0.0) + 0.001;
            float3 ptSpecular = ptNumerator / ptDenominator;
            
            float ptNdotL = max(dot(normal, ptLightDir), 0.0);
            
            // 조명 기여분 추가 (감쇠 적용)
            color += (ptKD * baseColor.rgb / 3.14159265359 + ptSpecular) * ptNdotL * lightColor * intensity * attFactor;
        }
        // 스포트라이트 코드는 필요시 추가
    }
}
else {
    // 조명 관리자가 없는 경우 기본 조명 계산 사용
    color += (kD * baseColor.rgb / 3.14159265359 + specular) * NdotL;
}

// 이미시브 추가
color += emissive;


    // 감마 보정
    color = color / (color + float3(1.0, 1.0, 1.0));
    color = pow(color, float3(1.0/2.2, 1.0/2.2, 1.0/2.2));
    
    return float4(color, baseColor.a);
}
)";

GltfLoader::GltfLoader()
{
}

GltfLoader::~GltfLoader()
{
    Release();
}

bool GltfLoader::LoadGlbModel(const std::string& filename, ID3D11Device* device)
{
    // tinygltf 설정
    tinygltf::TinyGLTF loader;
    // 이미지 쓰기 기능 비활성화
    
    tinygltf::Model model;
    std::string err;
    std::string warn;

    // 파일 확장자 검사
    std::string ext = filename.substr(filename.find_last_of(".") + 1);
    bool isGlb = (ext == "glb");

    // GLB 또는 GLTF 파일 로드
    bool ret = false;
    if (isGlb) {
        ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
    }
    else {
        ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
    }

    if (!ret) {
        std::cerr << "Failed to load GLTF/GLB file: " << filename << std::endl;
        if (!err.empty()) std::cerr << "Error: " << err << std::endl;
        return false;
    }

    if (!warn.empty()) {
        std::cout << "Warning: " << warn << std::endl;
    }

    // 모델 정보 설정
    modelInfo.Name = filename.substr(filename.find_last_of("/\\") + 1);
    modelInfo.FilePath = filename;

    // GLTF 모델 처리
    if (!ProcessGltfModel(model, device)) {
        return false;
    }

    // 셰이더 생성
    if (!CreateShaders(device)) {
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
    if (FAILED(hr)) {
        std::cerr << "Failed to create sampler state." << std::endl;
        return false;
    }


    
  

    return true;
}

bool GltfLoader::ProcessGltfModel(const tinygltf::Model& model, ID3D11Device* device)
{
    // 씬 정보 가져오기
    int defaultScene = model.defaultScene > -1 ? model.defaultScene : 0;
    // 모델에 씬이 없는 경우 처리
    if (model.scenes.empty()) {
        

        // 대체 방법: 모든 노드를 루트 노드로 처리
        for (size_t i = 0; i < model.nodes.size(); i++) {
            rootNodes.push_back(i);
        }
    }
    else {
        // 루트 노드 설정
        if (defaultScene >= 0 && defaultScene < model.scenes.size()) {
            const auto& scene = model.scenes[defaultScene];
            for (int nodeIdx : scene.nodes) {
                rootNodes.push_back(nodeIdx);
            }
        }

        // 루트 노드가 없는 경우 처리
        if (rootNodes.empty() && !model.nodes.empty()) {
            rootNodes.push_back(0);
        }
    }

    // 노드 처리
    nodes.resize(model.nodes.size());
    for (size_t i = 0; i < model.nodes.size(); i++) {
        const auto& node = model.nodes[i];

        // 노드 기본 정보 설정
        nodes[i].Name = node.name;
        nodes[i].Children = node.children;
        nodes[i].MeshIndex = node.mesh;

        // 노드 변환 행렬 계산
        XMMATRIX transform = XMMatrixIdentity();

        // 행렬 직접 사용
        if (node.matrix.size() == 16) {
            std::vector<float> m(16);
            for (int j = 0; j < 16; j++) {
                m[j] = static_cast<float>(node.matrix[j]);
            }

            // OpenGL(열 우선)에서 DirectX(행 우선)로 변환
            nodes[i].LocalTransform = XMMatrixSet(
                m[0], m[1], m[2], m[3],
                m[4], m[5], m[6], m[7],
                m[8], m[9], m[10], m[11],
                m[12], m[13], m[14], m[15]
            );
        }
        else {
            // 기본값 설정
            XMVECTOR T = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
            XMVECTOR R = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
            XMVECTOR S = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);

            // 제공된 값이 있다면 사용
            if (node.translation.size() == 3) {
                T = XMVectorSet(
                    static_cast<float>(node.translation[0]),
                    static_cast<float>(node.translation[1]),
                    static_cast<float>(node.translation[2]),
                    0.0f);
            }

            if (node.rotation.size() == 4) {
                R = XMVectorSet(
                    static_cast<float>(node.rotation[0]),
                    static_cast<float>(node.rotation[1]),
                    static_cast<float>(node.rotation[2]),
                    static_cast<float>(node.rotation[3]));
            }

            if (node.scale.size() == 3) {
                S = XMVectorSet(
                    static_cast<float>(node.scale[0]),
                    static_cast<float>(node.scale[1]),
                    static_cast<float>(node.scale[2]),
                    0.0f);
            }

            // S * R * T 순서로 변환 행렬 구성
            XMMATRIX scaleMatrix = XMMatrixScalingFromVector(S);
            XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(R);
            XMMATRIX translationMatrix = XMMatrixTranslationFromVector(T);

            nodes[i].LocalTransform = scaleMatrix * rotationMatrix * translationMatrix;
        }
    }

    // 머티리얼 처리
    for (size_t i = 0; i < model.materials.size(); i++) {
        const auto& material = model.materials[i];
        PbrMaterial pbrMaterial;

        pbrMaterial.Name = material.name.empty() ? "material_" + std::to_string(i) : material.name;

        // PBR 메탈릭-러프니스 정보
        const auto& pbrInfo = material.pbrMetallicRoughness;

        // 기본 색상
        if (pbrInfo.baseColorFactor.size() == 4) {
            pbrMaterial.BaseColorFactor = XMFLOAT4(
                (float)pbrInfo.baseColorFactor[0],
                (float)pbrInfo.baseColorFactor[1],
                (float)pbrInfo.baseColorFactor[2],
                (float)pbrInfo.baseColorFactor[3]
            );
        }

        // 메탈릭 및 러프니스 인자
        pbrMaterial.MetallicFactor = (float)pbrInfo.metallicFactor;
        pbrMaterial.RoughnessFactor = (float)pbrInfo.roughnessFactor;

        // 이미시브 인자
        if (material.emissiveFactor.size() == 3) {
            pbrMaterial.EmissiveFactor = XMFLOAT3(
                (float)material.emissiveFactor[0],
                (float)material.emissiveFactor[1],
                (float)material.emissiveFactor[2]
            );
        }

        // 텍스처 처리
        // 베이스 컬러 텍스처
        if (pbrInfo.baseColorTexture.index >= 0) {
            int texIndex = pbrInfo.baseColorTexture.index;
            const auto& texture = model.textures[texIndex];
            if (texture.source >= 0 && texture.source < model.images.size()) {
                const auto& image = model.images[texture.source];
                LoadTextureFromBuffer(image, device, &pbrMaterial.BaseColorTexture);
                pbrMaterial.BaseColorTexturePath = image.uri;
            }
        }

        // 메탈릭-러프니스 텍스처
        if (pbrInfo.metallicRoughnessTexture.index >= 0) {
            int texIndex = pbrInfo.metallicRoughnessTexture.index;
            const auto& texture = model.textures[texIndex];
            if (texture.source >= 0 && texture.source < model.images.size()) {
                const auto& image = model.images[texture.source];
                LoadTextureFromBuffer(image, device, &pbrMaterial.MetallicRoughnessTexture);
                pbrMaterial.MetallicRoughnessTexturePath = image.uri;
            }
        }

        // 노멀 텍스처
        if (material.normalTexture.index >= 0) {
            int texIndex = material.normalTexture.index;
            const auto& texture = model.textures[texIndex];
            if (texture.source >= 0 && texture.source < model.images.size()) {
                const auto& image = model.images[texture.source];
                LoadTextureFromBuffer(image, device, &pbrMaterial.NormalTexture);
                pbrMaterial.NormalTexturePath = image.uri;
            }
        }

        // 이미시브 텍스처
        if (material.emissiveTexture.index >= 0) {
            int texIndex = material.emissiveTexture.index;
            const auto& texture = model.textures[texIndex];
            if (texture.source >= 0 && texture.source < model.images.size()) {
                const auto& image = model.images[texture.source];
                LoadTextureFromBuffer(image, device, &pbrMaterial.EmissiveTexture);
                pbrMaterial.EmissiveTexturePath = image.uri;
            }
        }

        // 오클루전 텍스처
        if (material.occlusionTexture.index >= 0) {
            int texIndex = material.occlusionTexture.index;
            const auto& texture = model.textures[texIndex];
            if (texture.source >= 0 && texture.source < model.images.size()) {
                const auto& image = model.images[texture.source];
                LoadTextureFromBuffer(image, device, &pbrMaterial.OcclusionTexture);
                pbrMaterial.OcclusionTexturePath = image.uri;
            }
        }

        // 재질 맵에 추가
        materials[pbrMaterial.Name] = pbrMaterial;
    }

    // 메시 처리
    meshes.resize(model.meshes.size());
    for (size_t i = 0; i < model.meshes.size(); i++) {
        const auto& gltfMesh = model.meshes[i];
        auto& mesh = meshes[i];

        mesh.Name = gltfMesh.name.empty() ? "mesh_" + std::to_string(i) : gltfMesh.name;
        mesh.Primitives.resize(gltfMesh.primitives.size());

        // 각 프리미티브 처리
        for (size_t j = 0; j < gltfMesh.primitives.size(); j++) {
            const auto& primitive = gltfMesh.primitives[j];
            auto& meshPrimitive = mesh.Primitives[j];

            // 머티리얼 설정
            int materialIndex = primitive.material;
            if (materialIndex >= 0 && materialIndex < model.materials.size()) {
                const auto& material = model.materials[materialIndex];
                meshPrimitive.MaterialName = material.name.empty() ?
                    "material_" + std::to_string(materialIndex) : material.name;
            }
            else {
                meshPrimitive.MaterialName = "default";
            }

            // 정점 데이터 처리
            // 위치, 법선, 텍스처 좌표, 탄젠트 등의 속성 처리

            // 인덱스 버퍼 처리
            if (primitive.indices >= 0) {
                const auto& accessor = model.accessors[primitive.indices];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];

                const unsigned char* data = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
                int indexCount = accessor.count;
                meshPrimitive.IndexCount = indexCount;

                // 인덱스 타입에 따라 추출
                if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                    const uint16_t* indices = reinterpret_cast<const uint16_t*>(data);
                    for (int k = 0; k < indexCount; k++) {
                        meshPrimitive.Indices.push_back(indices[k]);
                    }
                }
                else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                    const uint32_t* indices = reinterpret_cast<const uint32_t*>(data);
                    for (int k = 0; k < indexCount; k++) {
                        meshPrimitive.Indices.push_back(indices[k]);
                    }
                }
                else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                    const uint8_t* indices = reinterpret_cast<const uint8_t*>(data);
                    for (int k = 0; k < indexCount; k++) {
                        meshPrimitive.Indices.push_back(indices[k]);
                    }
                }
            }

            // 위치 데이터 처리
            auto positionIt = primitive.attributes.find("POSITION");
            if (positionIt != primitive.attributes.end()) {
                const auto& accessor = model.accessors[positionIt->second];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];

                const unsigned char* data = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
                int stride = accessor.ByteStride(bufferView) ? accessor.ByteStride(bufferView) : sizeof(float) * 3;
                int vertexCount = accessor.count;

                // 정점 데이터 준비
                meshPrimitive.Vertices.resize(vertexCount);

                for (int k = 0; k < vertexCount; k++) {
                    const float* position = reinterpret_cast<const float*>(data + k * stride);
                    meshPrimitive.Vertices[k].Position = XMFLOAT3(position[0], position[1], position[2]);
                }
            }

            // 법선 데이터 처리
            auto normalIt = primitive.attributes.find("NORMAL");
            if (normalIt != primitive.attributes.end()) {
                const auto& accessor = model.accessors[normalIt->second];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];

                const unsigned char* data = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
                int stride = accessor.ByteStride(bufferView) ? accessor.ByteStride(bufferView) : sizeof(float) * 3;

                for (int k = 0; k < accessor.count; k++) {
                    const float* normal = reinterpret_cast<const float*>(data + k * stride);
                    meshPrimitive.Vertices[k].Normal = XMFLOAT3(normal[0], normal[1], normal[2]);
                }
            }

            // 텍스처 좌표 처리
            auto texcoordIt = primitive.attributes.find("TEXCOORD_0");
            if (texcoordIt != primitive.attributes.end()) {
                const auto& accessor = model.accessors[texcoordIt->second];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];

                const unsigned char* data = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
                int stride = accessor.ByteStride(bufferView) ? accessor.ByteStride(bufferView) : sizeof(float) * 2;

                for (int k = 0; k < accessor.count; k++) {
                    const float* texcoord = reinterpret_cast<const float*>(data + k * stride);
                    meshPrimitive.Vertices[k].TexCoord = XMFLOAT2(texcoord[0], 1.0f - texcoord[1]); // 텍스처 좌표 Y축 반전
                }
            }

            // 탄젠트 처리
            auto tangentIt = primitive.attributes.find("TANGENT");
            if (tangentIt != primitive.attributes.end()) {
                const auto& accessor = model.accessors[tangentIt->second];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];

                const unsigned char* data = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
                int stride = accessor.ByteStride(bufferView) ? accessor.ByteStride(bufferView) : sizeof(float) * 4;

                for (int k = 0; k < accessor.count; k++) {
                    const float* tangent = reinterpret_cast<const float*>(data + k * stride);
                    meshPrimitive.Vertices[k].Tangent = XMFLOAT4(tangent[0], tangent[1], tangent[2], tangent[3]);
                }
            }

            // 버퍼 생성
            CreateBuffers(device, meshPrimitive);
        }
    }

    // 애니메이션 처리 (간략화)
    for (size_t i = 0; i < model.animations.size(); i++) {
        const auto& gltfAnimation = model.animations[i];
        Animation animation;

        animation.Name = gltfAnimation.name.empty() ? "animation_" + std::to_string(i) : gltfAnimation.name;

        for (const auto& channel : gltfAnimation.channels) {
            AnimationChannel animChannel;
            animChannel.NodeIndex = channel.target_node;

            // 타겟 경로 설정
            if (channel.target_path == "translation") {
                animChannel.Path = AnimationChannel::TRANSLATION;
            }
            else if (channel.target_path == "rotation") {
                animChannel.Path = AnimationChannel::ROTATION;
            }
            else if (channel.target_path == "scale") {
                animChannel.Path = AnimationChannel::SCALE;
            }
            else if (channel.target_path == "weights") {
                animChannel.Path = AnimationChannel::WEIGHTS;
            }
            else {
                continue; // 알 수 없는 타겟 경로
            }

            // 샘플러 처리
            const auto& sampler = gltfAnimation.samplers[channel.sampler];

            // 시간 데이터 처리
            if (sampler.input >= 0 && sampler.input < model.accessors.size()) {
                const auto& accessor = model.accessors[sampler.input];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];

                const unsigned char* data = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
                int stride = accessor.ByteStride(bufferView) ? accessor.ByteStride(bufferView) : sizeof(float);

                for (int k = 0; k < accessor.count; k++) {
                    const float* time = reinterpret_cast<const float*>(data + k * stride);
                    animChannel.Times.push_back(*time);

                    
                    // min 
                    if (*time < animation.StartTime) {
                        animation.StartTime = *time;
                    }

                    // max 
                    if (*time > animation.EndTime) {
                        animation.EndTime = *time;
                    }
                }
            }

            // 값 데이터 처리
            if (sampler.output >= 0 && sampler.output < model.accessors.size()) {
                const auto& accessor = model.accessors[sampler.output];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];

                const unsigned char* data = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
                int components = accessor.type == TINYGLTF_TYPE_VEC3 ? 3 : (accessor.type == TINYGLTF_TYPE_VEC4 ? 4 : 1);
                int stride = accessor.ByteStride(bufferView) ? accessor.ByteStride(bufferView) : sizeof(float) * components;

                for (int k = 0; k < accessor.count; k++) {
                    const float* value = reinterpret_cast<const float*>(data + k * stride);

                    if (components == 1) {
                        animChannel.Values.push_back(XMFLOAT4(value[0], 0.0f, 0.0f, 0.0f));
                    }
                    else if (components == 3) {
                        animChannel.Values.push_back(XMFLOAT4(value[0], value[1], value[2], 0.0f));
                    }
                    else if (components == 4) {
                        animChannel.Values.push_back(XMFLOAT4(value[0], value[1], value[2], value[3]));
                    }
                }
            }

            animation.Channels.push_back(animChannel);
        }

        animations.push_back(animation);
    }

    // 모델 로드 후 바운딩 박스 계산
    BoundingBox box = CalculateBoundingBox();

    // 모델을 원점에 중심 맞춤
    modelInfo.Position = XMFLOAT3(-box.center.x, -box.center.y, -box.center.z);

    // 적절한 크기로 조정
    float maxDim = max(max(box.max.x - box.min.x, box.max.y - box.min.y), box.max.z - box.min.z);
    if (maxDim > 0) {
        float scale = 2.0f / maxDim; // 표준 크기로 조정
        modelInfo.Scale = XMFLOAT3(scale, scale, scale);
    }

    // 모델 크기 자동 조정
    AutoResizeModel();

    return true;
}

void GltfLoader::AutoResizeModel()
{
    // 모든 메시의 모든 버텍스를 검사하여 바운딩 박스 계산
    BoundingBox box = CalculateBoundingBox();

    // 매우 큰 모델인 경우에만 크기 조정 (비정상적인 크기 방지)
    float maxDimension = max(max(
        abs(box.max.x - box.min.x),
        abs(box.max.y - box.min.y)),
        abs(box.max.z - box.min.z));

    if (maxDimension > 10.0f) {
        // 적절한 크기로 스케일 조정
        float scale = 1.0f / (maxDimension / 10.0f);
        modelInfo.Scale = XMFLOAT3(scale, scale, scale);

        
    }

    // 중심 조정 (선택적)
    modelInfo.Position = XMFLOAT3(
        -box.center.x,
        -box.center.y,
        -box.center.z);
}

bool GltfLoader::LoadTextureFromBuffer(const tinygltf::Image& image, ID3D11Device* device, ID3D11ShaderResourceView** textureView)
{
    // 이미지 데이터가 있는지 확인
    if (image.width <= 0 || image.height <= 0 || image.image.empty()) {
        return false;
    }
    
    // 텍스처 생성에 필요한 데이터 준비
    UINT width = static_cast<UINT>(image.width);
    UINT height = static_cast<UINT>(image.height);
    
    // RGB 또는 RGBA 데이터로 텍스처 생성
    std::vector<uint8_t> textureData;
    UINT rowPitch;
    
    if (image.component == 3) {
        // RGB를 RGBA로 변환
        textureData.resize(width * height * 4);
        const uint8_t* srcData = static_cast<const uint8_t*>(image.image.data());
        
        for (size_t i = 0; i < width * height; i++) {
            textureData[i * 4 + 0] = srcData[i * 3 + 0]; // R
            textureData[i * 4 + 1] = srcData[i * 3 + 1]; // G
            textureData[i * 4 + 2] = srcData[i * 3 + 2]; // B
            textureData[i * 4 + 3] = 255;               // A
        }
        
        rowPitch = width * 4;
    }
    else if (image.component == 4) {
        // RGBA 데이터 그대로 사용
        textureData = image.image;
        rowPitch = width * 4;
    }
    else {
        // 지원하지 않는 형식
        return false;
    }
    
    // DirectX::CreateWICTextureFromMemory 함수 사용
    ID3D11Resource* texture = nullptr;
    HRESULT hr = DirectX::CreateWICTextureFromMemoryEx(
        device,
        textureData.data(),
        textureData.size(),
        0, // maxsize (0 = 제한 없음)
        D3D11_USAGE_DEFAULT,
        D3D11_BIND_SHADER_RESOURCE,
        0, // cpuAccessFlags
        0, // miscFlags
        DirectX::WIC_LOADER_DEFAULT, // loadFlags
        &texture,
        textureView
    );
    
    if (SUCCEEDED(hr) && texture) {
        texture->Release();  // 셰이더 리소스 뷰만 필요하므로 텍스처 리소스는 해제
        return true;
    }
    
    // 만약 WIC 함수가 실패하면, 기본적인 D3D11 방식으로 시도
    if (FAILED(hr)) {
        // 텍스처 설명 구조체 설정
        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width = width;
        textureDesc.Height = height;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        textureDesc.CPUAccessFlags = 0;
        textureDesc.MiscFlags = 0;
        
        // 초기 데이터 설정
        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = textureData.data();
        initData.SysMemPitch = rowPitch;
        initData.SysMemSlicePitch = 0; // 3D 텍스처가 아니므로 무시됨
        
        // 텍스처 생성
        ID3D11Texture2D* texture2D = nullptr;
        hr = device->CreateTexture2D(&textureDesc, &initData, &texture2D);
        
        if (SUCCEEDED(hr) && texture2D) {
            // 셰이더 리소스 뷰 생성
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = textureDesc.Format;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.MipLevels = 1;
            
            hr = device->CreateShaderResourceView(texture2D, &srvDesc, textureView);
            texture2D->Release(); // 셰이더 리소스 뷰만 필요하므로 텍스처 리소스는 해제
            
            if (SUCCEEDED(hr)) {
                return true;
            }
        }
    }
    
    std::cerr << "Failed to create texture from memory." << std::endl;
    return false;
}
bool GltfLoader::LoadTexture(const std::string& texturePath, ID3D11Device* device, ID3D11ShaderResourceView** textureView)
{
    if (texturePath.empty()) {
        return false;
    }

    // 텍스처 로드 - DirectX::CreateWICTextureFromFile 함수 사용
    std::wstring wTexturePath(texturePath.begin(), texturePath.end());

    ID3D11Resource* texture = nullptr;
    HRESULT hr = DirectX::CreateWICTextureFromFile(
        device,
        wTexturePath.c_str(),
        &texture,
        textureView
    );

    if (SUCCEEDED(hr) && texture) {
        texture->Release();  // 셰이더 리소스 뷰만 필요하므로 텍스처 리소스는 해제
        return true;
    }

    std::cerr << "Failed to load texture: " << texturePath << std::endl;
    return false;
}

bool GltfLoader::CreateBuffers(ID3D11Device* device, MeshPrimitive& primitive)
{
    // 버텍스 버퍼 생성
    D3D11_BUFFER_DESC vbDesc;
    ZeroMemory(&vbDesc, sizeof(vbDesc));
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * primitive.Vertices.size());
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA vbData;
    ZeroMemory(&vbData, sizeof(vbData));
    vbData.pSysMem = primitive.Vertices.data();

    HRESULT hr = device->CreateBuffer(&vbDesc, &vbData, &primitive.VertexBuffer);
    if (FAILED(hr)) {
        return false;
    }

    // 인덱스 버퍼가 있는 경우에만 생성
    if (!primitive.Indices.empty()) {
        D3D11_BUFFER_DESC ibDesc;
        ZeroMemory(&ibDesc, sizeof(ibDesc));
        ibDesc.Usage = D3D11_USAGE_DEFAULT;
        ibDesc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * primitive.Indices.size());
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ibDesc.CPUAccessFlags = 0;

        D3D11_SUBRESOURCE_DATA ibData;
        ZeroMemory(&ibData, sizeof(ibData));
        ibData.pSysMem = primitive.Indices.data();

        hr = device->CreateBuffer(&ibDesc, &ibData, &primitive.IndexBuffer);
        if (FAILED(hr)) {
            return false;
        }
    }

    return true;
}

bool GltfLoader::CreateShaders(ID3D11Device* device)
{
    // 셰이더 컴파일 및 생성
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    // 정점 셰이더 컴파일
    HRESULT hr = D3DCompile(glbVertexShaderCode, strlen(glbVertexShaderCode), "VS", nullptr, nullptr, "main", "vs_4_0", 0, 0, &vsBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        return false;
    }

    // 픽셀 셰이더 컴파일
    hr = D3DCompile(glbPixelShaderCode, strlen(glbPixelShaderCode), "PS", nullptr, nullptr, "main", "ps_4_0", 0, 0, &psBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        if (vsBlob) vsBlob->Release();
        return false;
    }

    // 셰이더 생성
    hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
    if (FAILED(hr)) {
        vsBlob->Release();
        psBlob->Release();
        return false;
    }

    hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);
    if (FAILED(hr)) {
        vsBlob->Release();
        psBlob->Release();
        return false;
    }

    // 입력 레이아웃 설정 - GLB에서 사용하는 모든 정점 속성 포함
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "JOINTS", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 64, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    hr = device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);
    vsBlob->Release();
    psBlob->Release();
    if (FAILED(hr)) {
        return false;
    }

    // 상수 버퍼 생성
    D3D11_BUFFER_DESC cbDesc;
    ZeroMemory(&cbDesc, sizeof(cbDesc));
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.ByteWidth = sizeof(ConstantBuffer);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    hr = device->CreateBuffer(&cbDesc, nullptr, &constantBuffer);
    if (FAILED(hr)) {
        return false;
    }

    return true;
}

// Render 함수 수정하여 계층 구조를 올바르게 렌더링
void GltfLoader::Render(ID3D11DeviceContext* deviceContext, const Camera& camera)
{
    if (!modelInfo.Visible || meshes.empty()) {
        return;
    }

    // 셰이더 설정
    deviceContext->VSSetShader(vertexShader, nullptr, 0);
    deviceContext->PSSetShader(pixelShader, nullptr, 0);
    deviceContext->IASetInputLayout(inputLayout);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 샘플러 상태 설정
    deviceContext->PSSetSamplers(0, 1, &samplerState);

    // 전역 월드 변환 행렬
    XMMATRIX globalWorldMatrix = CalculateWorldMatrix();

    // 루트 노드부터 시작하여 계층적으로 렌더링
    for (int rootNodeIdx : rootNodes) {
        RenderNode(deviceContext, camera, rootNodeIdx, globalWorldMatrix);
    }
}

// GltfLoader.cpp에 추가 - constantBuffer 사용 버전
void GltfLoader::Render(ID3D11DeviceContext* deviceContext, const Camera& camera, LightManager* lightManager)
{
    if (!modelInfo.Visible || meshes.empty()) {
        return;
    }

    // 셰이더 설정
    deviceContext->VSSetShader(vertexShader, nullptr, 0);
    deviceContext->PSSetShader(pixelShader, nullptr, 0);
    deviceContext->IASetInputLayout(inputLayout);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 샘플러 상태 설정
    deviceContext->PSSetSamplers(0, 1, &samplerState);

    // 전역 월드 변환 행렬
    XMMATRIX globalWorldMatrix = CalculateWorldMatrix();

    // 조명 정보 설정 (조명 관리자가 있을 경우)
    if (lightManager) {
        lightManager->UpdateLightBuffer(deviceContext);
        lightManager->SetLightBuffer(deviceContext);

        // 디버깅용 로그
        OutputDebugStringA("GLB 모델에 조명 정보가 적용되었습니다.\n");
        OutputDebugStringA(("조명 개수: " + std::to_string(lightManager->GetLightCount()) + "\n").c_str());
    }

    // 루트 노드부터 시작하여 계층적으로 렌더링
    for (int rootNodeIdx : rootNodes) {
        RenderNode(deviceContext, camera, rootNodeIdx, globalWorldMatrix);
    }
}

// 노드 렌더링 함수 추가
void GltfLoader::RenderNode(ID3D11DeviceContext* deviceContext, const Camera& camera,
    int nodeIndex, XMMATRIX parentTransform)
{
    if (nodeIndex < 0 || nodeIndex >= nodes.size()) {
        return;
    }

    const Node& node = nodes[nodeIndex];

    // 노드 변환 행렬과 부모 변환 행렬 결합
    XMMATRIX localTransform = node.LocalTransform;
    XMMATRIX worldTransform = XMMatrixMultiply(localTransform, parentTransform);

    
    // 노드 변환 행렬 결합 - 부모 변환을 "오른쪽에서" 곱하여 올바른 계층 구조 유지
    XMMATRIX nodeTransform = XMMatrixMultiply(node.LocalTransform, parentTransform);

    // 디버그용 스케일 검사 (비정상적인 크기 확인)
    XMVECTOR scale, rotation, translation;
    XMMatrixDecompose(&scale, &rotation, &translation, nodeTransform);

    // 올바른 방법으로 scale 값 추출
    XMFLOAT3 scaleFloat;
    XMStoreFloat3(&scaleFloat, scale);
    float scaleX = scaleFloat.x;
    float scaleY = scaleFloat.y;
    float scaleZ = scaleFloat.z;

    XMStoreFloat3(&XMFLOAT3(scaleX, scaleY, scaleZ), scale);

    // 스케일이 비정상적으로 큰 경우 고정
    const float MAX_SCALE = 100.0f;
    if (scaleX > MAX_SCALE || scaleY > MAX_SCALE || scaleZ > MAX_SCALE) {
        // 스케일 제한
        scaleX = min(scaleX, MAX_SCALE);
        scaleY = min(scaleY, MAX_SCALE);
        scaleZ = min(scaleZ, MAX_SCALE);

        // 새 스케일로 변환 행렬 재구성
        XMFLOAT3 newScale(scaleX, scaleY, scaleZ);
        scale = XMLoadFloat3(&newScale);
        nodeTransform = XMMatrixTransformation(
            XMVectorZero(), XMQuaternionIdentity(),
            scale, XMVectorZero(), rotation, translation);
    }


    // 현재 노드에 메시가 있으면 렌더링
    if (node.MeshIndex >= 0 && node.MeshIndex < meshes.size()) {
        const auto& mesh = meshes[node.MeshIndex];

        for (const auto& primitive : mesh.Primitives) {
            if (!primitive.VertexBuffer || !primitive.IndexBuffer) {
                continue;
            }
            if (primitive.IndexCount == 0 || primitive.Vertices.size() == 0) {
                continue;
            }
            // 재질 가져오기
            PbrMaterial* material = nullptr;
            auto it = materials.find(primitive.MaterialName);
            if (it != materials.end()) {
                material = &it->second;
            }
            else {
                // 기본 재질
                static PbrMaterial defaultMaterial;
                defaultMaterial.Name = "default";
                material = &defaultMaterial;
            }

            // 상수 버퍼 업데이트
            ConstantBuffer cb;
            cb.World = XMMatrixTranspose(worldTransform);
            cb.View = XMMatrixTranspose(camera.GetViewMatrix());
            cb.Projection = XMMatrixTranspose(camera.GetProjectionMatrix());

            // PBR 재질 정보 설정
            cb.BaseColorFactor = material->BaseColorFactor;
            cb.EmissiveFactor = material->EmissiveFactor;
            cb.MetallicFactor = material->MetallicFactor;
            cb.RoughnessFactor = material->RoughnessFactor;

            // 텍스처 유무 설정
            cb.HasBaseColorTexture = (material->BaseColorTexture != nullptr) ? 1.0f : 0.0f;
            cb.HasMetallicRoughnessTexture = (material->MetallicRoughnessTexture != nullptr) ? 1.0f : 0.0f;
            cb.HasNormalTexture = (material->NormalTexture != nullptr) ? 1.0f : 0.0f;
            cb.HasEmissiveTexture = (material->EmissiveTexture != nullptr) ? 1.0f : 0.0f;
            cb.HasOcclusionTexture = (material->OcclusionTexture != nullptr) ? 1.0f : 0.0f;

            deviceContext->UpdateSubresource(constantBuffer, 0, nullptr, &cb, 0, 0);
            deviceContext->VSSetConstantBuffers(0, 1, &constantBuffer);
            deviceContext->PSSetConstantBuffers(0, 1, &constantBuffer);

            // 텍스처 설정
            ID3D11ShaderResourceView* textures[5] = {
                material->BaseColorTexture,
                material->MetallicRoughnessTexture,
                material->NormalTexture,
                material->EmissiveTexture,
                material->OcclusionTexture
            };
            deviceContext->PSSetShaderResources(0, 5, textures);

            // 버텍스 및 인덱스 버퍼 설정
            UINT stride = sizeof(Vertex);
            UINT offset = 0;
            deviceContext->IASetVertexBuffers(0, 1, &primitive.VertexBuffer, &stride, &offset);
            deviceContext->IASetIndexBuffer(primitive.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

            // 래스터라이저 상태 설정
            deviceContext->RSSetState(rasterizerState);

            // 그리기
            deviceContext->DrawIndexed(primitive.IndexCount, 0, 0);
        }
    }

    // 자식 노드들 재귀적으로 렌더링
    for (int childIndex : node.Children) {
        RenderNode(deviceContext, camera, childIndex, worldTransform);
    }
}

//void GltfLoader::UpdateAnimation(float deltaTime)
//{
//    if (animations.empty() || currentAnimationIndex < 0 || currentAnimationIndex >= animations.size()) {
//        return;
//    }
//
//    // 현재 애니메이션
//    const auto& animation = animations[currentAnimationIndex];
//
//    // 애니메이션 시간 업데이트
//    currentAnimationTime += deltaTime;
//
//    // 애니메이션 순환
//    if (currentAnimationTime > animation.EndTime) {
//        currentAnimationTime = fmodf(currentAnimationTime - animation.StartTime, animation.EndTime - animation.StartTime) + animation.StartTime;
//    }
//
//    // 각 채널에 대해 현재 시간에 맞는 애니메이션 적용
//    for (const auto& channel : animation.Channels) {
//        if (channel.NodeIndex < 0 || channel.NodeIndex >= nodes.size() || channel.Times.empty()) {
//            continue;
//        }
//
//        // 시간값에 따른 인덱스 찾기
//        size_t nextKeyframe = 0;
//        while (nextKeyframe < channel.Times.size() && channel.Times[nextKeyframe] <= currentAnimationTime) {
//            nextKeyframe++;
//        }
//
//        size_t prevKeyframe = nextKeyframe > 0 ? nextKeyframe - 1 : 0;
//        if (nextKeyframe >= channel.Times.size()) {
//            nextKeyframe = prevKeyframe;
//        }
//
//        // 두 키프레임 사이의 보간 계수 계산
//        float t = 0.0f;
//        if (nextKeyframe > prevKeyframe) {
//            t = (currentAnimationTime - channel.Times[prevKeyframe]) / (channel.Times[nextKeyframe] - channel.Times[prevKeyframe]);
//        }
//
//        // 애니메이션 타입에 따른 적용
//        Node& node = nodes[channel.NodeIndex];
//
//        switch (channel.Path) {
//        case AnimationChannel::TRANSLATION: {
//            XMVECTOR prevT = XMLoadFloat4(&channel.Values[prevKeyframe]);
//            XMVECTOR nextT = XMLoadFloat4(&channel.Values[nextKeyframe]);
//            XMVECTOR translation = XMVectorLerp(prevT, nextT, t);
//
//            XMFLOAT3 pos;
//            XMStoreFloat3(&pos, translation);
//
//            // 노드의 로컬 변환 행렬을 업데이트
//            // (간략화됨 - 실제 구현에서는 기존 스케일과 회전을 유지해야 함)
//            node.LocalTransform = XMMatrixTranslation(pos.x, pos.y, pos.z);
//            break;
//        }
//        case AnimationChannel::ROTATION: {
//            XMVECTOR prevR = XMLoadFloat4(&channel.Values[prevKeyframe]);
//            XMVECTOR nextR = XMLoadFloat4(&channel.Values[nextKeyframe]);
//            XMVECTOR rotation = XMQuaternionSlerp(prevR, nextR, t);
//
//            // 노드의 로컬 변환 행렬을 업데이트
//            node.LocalTransform = XMMatrixRotationQuaternion(rotation);
//            break;
//        }
//        case AnimationChannel::SCALE: {
//            XMVECTOR prevS = XMLoadFloat4(&channel.Values[prevKeyframe]);
//            XMVECTOR nextS = XMLoadFloat4(&channel.Values[nextKeyframe]);
//            XMVECTOR scale = XMVectorLerp(prevS, nextS, t);
//
//            XMFLOAT3 s;
//            XMStoreFloat3(&s, scale);
//
//            // 노드의 로컬 변환 행렬을 업데이트
//            node.LocalTransform = XMMatrixScaling(s.x, s.y, s.z);
//            break;
//        }
//                                    // 'WEIGHTS' 타입은 모프 타깃 애니메이션에 사용되지만,
//                                    // 이 예제에서는 구현을 생략합니다.
//        }
//    }
//}

// GltfLoader.cpp의 CalculateNodeTransform 함수를 수정
XMMATRIX GltfLoader::CalculateNodeTransform(int nodeIndex)
{
    if (nodeIndex < 0 || nodeIndex >= nodes.size()) {
        return XMMatrixIdentity();
    }

    const Node& node = nodes[nodeIndex];
    XMMATRIX localTransform = node.LocalTransform;

    // 부모 노드가 있는 경우 부모의 변환을 포함
    for (size_t i = 0; i < nodes.size(); i++) {
        const Node& potentialParent = nodes[i];

        // 현재 노드가 이 노드의 자식인지 확인
        auto it = std::find(potentialParent.Children.begin(), potentialParent.Children.end(), nodeIndex);
        if (it != potentialParent.Children.end()) {
            // 부모 변환 계산하여 적용
            XMMATRIX parentTransform = CalculateNodeTransform(i);
            return XMMatrixMultiply(localTransform, parentTransform);
        }
    }

    // 부모가 없으면 로컬 변환만 반환
    return localTransform;
}

XMMATRIX GltfLoader::CalculateWorldMatrix()
{
    // 월드 행렬 계산 (모델의 전역 변환)
    XMMATRIX scale = XMMatrixScaling(modelInfo.Scale.x, modelInfo.Scale.y, modelInfo.Scale.z);
    XMMATRIX rotation = XMMatrixRotationRollPitchYaw(
        XMConvertToRadians(modelInfo.Rotation.x),
        XMConvertToRadians(modelInfo.Rotation.y),
        XMConvertToRadians(modelInfo.Rotation.z));
    XMMATRIX translation = XMMatrixTranslation(modelInfo.Position.x, modelInfo.Position.y, modelInfo.Position.z);

    // SRT 순서대로 변환 적용 (크기 -> 회전 -> 이동)
    return scale * rotation * translation;
}

void GltfLoader::Release()
{
    // 메시 프리미티브 버퍼 해제
    for (auto& mesh : meshes) {
        for (auto& primitive : mesh.Primitives) {
            if (primitive.VertexBuffer) { primitive.VertexBuffer->Release(); primitive.VertexBuffer = nullptr; }
            if (primitive.IndexBuffer) { primitive.IndexBuffer->Release(); primitive.IndexBuffer = nullptr; }
        }
    }

    // 재질 텍스처 해제
    for (auto& material : materials) {
        if (material.second.BaseColorTexture) { material.second.BaseColorTexture->Release(); material.second.BaseColorTexture = nullptr; }
        if (material.second.MetallicRoughnessTexture) { material.second.MetallicRoughnessTexture->Release(); material.second.MetallicRoughnessTexture = nullptr; }
        if (material.second.NormalTexture) { material.second.NormalTexture->Release(); material.second.NormalTexture = nullptr; }
        if (material.second.EmissiveTexture) { material.second.EmissiveTexture->Release(); material.second.EmissiveTexture = nullptr; }
        if (material.second.OcclusionTexture) { material.second.OcclusionTexture->Release(); material.second.OcclusionTexture = nullptr; }
    }

    // 셰이더 및 관련 리소스 해제
    if (vertexShader) { vertexShader->Release(); vertexShader = nullptr; }
    if (pixelShader) { pixelShader->Release(); pixelShader = nullptr; }
    if (inputLayout) { inputLayout->Release(); inputLayout = nullptr; }
    if (constantBuffer) { constantBuffer->Release(); constantBuffer = nullptr; }
    if (rasterizerState) { rasterizerState->Release(); rasterizerState = nullptr; }
    if (samplerState) { samplerState->Release(); samplerState = nullptr; }

    // 메시, 노드, 애니메이션 데이터 초기화
    meshes.clear();
    nodes.clear();
    materials.clear();
    animations.clear();
    rootNodes.clear();
}
// GltfLoader.cpp에 추가할 애니메이션 관련 함수들

// 현재 애니메이션 인덱스 가져오기
int GltfLoader::GetCurrentAnimationIndex() const
{
    return currentAnimationIndex;
}

// 현재 애니메이션 시간 가져오기
float GltfLoader::GetCurrentAnimationTime() const
{
    return currentAnimationTime;
}

// 애니메이션 목록 가져오기
const std::vector<GltfLoader::Animation>& GltfLoader::GetAnimations() const
{
    return animations;
}

// 현재 애니메이션 설정
void GltfLoader::SetCurrentAnimation(int index)
{
    if (index >= 0 && index < animations.size()) {
        currentAnimationIndex = index;
        currentAnimationTime = animations[index].StartTime;
    }
}

// 애니메이션 재생 여부 확인
bool GltfLoader::IsAnimationPlaying() const
{
    return animationPlaying;
}

// 애니메이션 재생 시작
void GltfLoader::PlayAnimation()
{
    animationPlaying = true;
}

// 애니메이션 일시 정지
void GltfLoader::PauseAnimation()
{
    animationPlaying = false;
}

// 애니메이션 속도 설정
void GltfLoader::SetAnimationSpeed(float speed)
{
    animationSpeed = speed > 0.0f ? speed : 1.0f;
}

// 애니메이션 속도 가져오기
float GltfLoader::GetAnimationSpeed() const
{
    return animationSpeed;
}

// UpdateAnimation 함수 수정 (animationPlaying 및 animationSpeed 적용)
void GltfLoader::UpdateAnimation(float deltaTime)
{
    if (animations.empty() || currentAnimationIndex < 0 || currentAnimationIndex >= animations.size() || !animationPlaying) {
        return;
    }

    // 애니메이션 속도 적용
    deltaTime *= animationSpeed;

    // 현재 애니메이션
    const auto& animation = animations[currentAnimationIndex];

    // 애니메이션 시간 업데이트
    currentAnimationTime += deltaTime;

    // 애니메이션 순환
    if (currentAnimationTime > animation.EndTime) {
        currentAnimationTime = fmodf(currentAnimationTime - animation.StartTime, animation.EndTime - animation.StartTime) + animation.StartTime;
    }

    // 각 채널에 대해 현재 시간에 맞는 애니메이션 적용
    for (const auto& channel : animation.Channels) {
        if (channel.NodeIndex < 0 || channel.NodeIndex >= nodes.size() || channel.Times.empty()) {
            continue;
        }

        // 시간값에 따른 인덱스 찾기
        size_t nextKeyframe = 0;
        while (nextKeyframe < channel.Times.size() && channel.Times[nextKeyframe] <= currentAnimationTime) {
            nextKeyframe++;
        }

        size_t prevKeyframe = nextKeyframe > 0 ? nextKeyframe - 1 : 0;
        if (nextKeyframe >= channel.Times.size()) {
            nextKeyframe = prevKeyframe;
        }

        // 두 키프레임 사이의 보간 계수 계산
        float t = 0.0f;
        if (nextKeyframe > prevKeyframe) {
            t = (currentAnimationTime - channel.Times[prevKeyframe]) / (channel.Times[nextKeyframe] - channel.Times[prevKeyframe]);
        }

        // 애니메이션 타입에 따른 적용
        Node& node = nodes[channel.NodeIndex];

        switch (channel.Path) {
        case AnimationChannel::TRANSLATION: {
            XMVECTOR prevT = XMLoadFloat4(&channel.Values[prevKeyframe]);
            XMVECTOR nextT = XMLoadFloat4(&channel.Values[nextKeyframe]);
            XMVECTOR translation = XMVectorLerp(prevT, nextT, t);

            XMFLOAT3 pos;
            XMStoreFloat3(&pos, translation);

            // 노드의 로컬 변환 행렬을 업데이트
            // (간략화됨 - 실제 구현에서는 기존 스케일과 회전을 유지해야 함)
            node.LocalTransform = XMMatrixTranslation(pos.x, pos.y, pos.z);
            break;
        }
        case AnimationChannel::ROTATION: {
            XMVECTOR prevR = XMLoadFloat4(&channel.Values[prevKeyframe]);
            XMVECTOR nextR = XMLoadFloat4(&channel.Values[nextKeyframe]);
            XMVECTOR rotation = XMQuaternionSlerp(prevR, nextR, t);

            // 노드의 로컬 변환 행렬을 업데이트
            node.LocalTransform = XMMatrixRotationQuaternion(rotation);
            break;
        }
        case AnimationChannel::SCALE: {
            XMVECTOR prevS = XMLoadFloat4(&channel.Values[prevKeyframe]);
            XMVECTOR nextS = XMLoadFloat4(&channel.Values[nextKeyframe]);
            XMVECTOR scale = XMVectorLerp(prevS, nextS, t);

            XMFLOAT3 s;
            XMStoreFloat3(&s, scale);

            // 노드의 로컬 변환 행렬을 업데이트
            node.LocalTransform = XMMatrixScaling(s.x, s.y, s.z);
            break;
        }
                                    // 'WEIGHTS' 타입은 모프 타깃 애니메이션에 사용되지만, 여기서는 구현 생략
        }
    }
}
// GltfLoader.cpp에 추가
BoundingBox GltfLoader::CalculateBoundingBox() const
{
    BoundingBox box = { XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX), XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX) };
    bool hasVertices = false;

    // 모든 메시의 모든 버텍스를 검사하여 바운딩 박스 계산
    for (const auto& mesh : meshes) {
        for (const auto& primitive : mesh.Primitives) {
            for (const auto& vertex : primitive.Vertices) {
                // 최소/최대 경계 갱신
                box.min.x = min(box.min.x, vertex.Position.x);
                box.min.y = min(box.min.y, vertex.Position.y);
                box.min.z = min(box.min.z, vertex.Position.z);

                box.max.x = max(box.max.x, vertex.Position.x);
                box.max.y = max(box.max.y, vertex.Position.y);
                box.max.z = max(box.max.z, vertex.Position.z);

                hasVertices = true;
            }
        }
    }

    // 버텍스가 없는 경우 기본 박스 반환
    if (!hasVertices) {
        box.min = XMFLOAT3(-1.0f, -1.0f, -1.0f);
        box.max = XMFLOAT3(1.0f, 1.0f, 1.0f);
    }

    // 중심과 반지름 계산
    box.center.x = (box.min.x + box.max.x) * 0.5f;
    box.center.y = (box.min.y + box.max.y) * 0.5f;
    box.center.z = (box.min.z + box.max.z) * 0.5f;

    float dx = box.max.x - box.min.x;
    float dy = box.max.y - box.min.y;
    float dz = box.max.z - box.min.z;
    box.radius = sqrt(dx * dx + dy * dy + dz * dz) * 0.5f;

    return box;
}

