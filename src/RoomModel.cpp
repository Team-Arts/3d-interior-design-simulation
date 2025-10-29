// RoomModel.cpp
#include "RoomModel.h"
#include <d3dcompiler.h>
#include "Camera.h"

// 상수 버퍼 구조체
struct ConstantBuffer
{
    XMMATRIX World;
    XMMATRIX View;
    XMMATRIX Projection;
    XMFLOAT4 AmbientColor;
};

// 간단한 셰이더 코드
const char* roomVertexShaderCode = R"(
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float4 AmbientColor;
}

struct VS_INPUT
{
    float3 Pos : POSITION;
    float3 Normal : NORMAL;
    float2 Tex : TEXCOORD0;
    float4 Color : COLOR;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Normal : NORMAL;
    float2 Tex : TEXCOORD0;
    float4 Color : COLOR;
    float3 WorldPos : TEXCOORD1;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = float4(input.Pos, 1.0f);
    
    // 월드 위치 계산 (조명 계산용)
    output.WorldPos = mul(output.Pos, World).xyz;
    
    // 월드 변환
    output.Pos = mul(output.Pos, World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    
    // 월드 공간 법선
    output.Normal = mul(input.Normal, (float3x3)World);
    output.Normal = normalize(output.Normal);
    
    // 텍스처 좌표 및 색상 전달
    output.Tex = input.Tex;
    output.Color = input.Color;
    
    return output;
}
)";

// RoomModel.cpp 수정 - 조명을 지원하는 업데이트된 픽셀 셰이더
const char* roomPixelShaderCode = R"(
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
    float4 Color : COLOR;
    float3 WorldPos : TEXCOORD1;
};

float3 CalculateDirectionalLight(float3 normal, float3 viewDir, int lightIndex, float4 materialColor)
{
    float3 lightDir = normalize(-Lights[lightIndex].Direction.xyz);
    float3 lightColor = Lights[lightIndex].Color.rgb;
    float intensity = Lights[lightIndex].Color.a;
    
    // 디퓨즈 계산
    float diff = max(dot(normal, lightDir), 0.0);
    float3 diffuse = diff * materialColor.rgb * lightColor * intensity;
    
    // 스페큘러 계산 (RoomModel은 간소화)
    float specularPower = 32.0;
    float3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), specularPower);
    float3 specular = spec * lightColor * intensity * 0.3; // 낮은 스페큘러
    
    return diffuse + specular;
}

float3 CalculatePointLight(float3 normal, float3 fragPos, float3 viewDir, int lightIndex, float4 materialColor)
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
    float3 diffuse = diff * materialColor.rgb * lightColor * intensity;
    
    // 스페큘러 계산 (RoomModel은 간소화)
    float specularPower = 32.0;
    float3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), specularPower);
    float3 specular = spec * lightColor * intensity * 0.3; // 낮은 스페큘러
    
    // 거리에 따른 감쇠
    float distance = length(lightPos - fragPos);
    float attenFactor = 1.0 / (1.0 + attenuation * (distance * distance / (range * range)));
    
    return (diffuse + specular) * attenFactor;
}

float3 CalculateSpotLight(float3 normal, float3 fragPos, float3 viewDir, int lightIndex, float4 materialColor)
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
    float3 diffuse = diff * materialColor.rgb * lightColor * intensity;
    
    // 스페큘러 계산 (RoomModel은 간소화)
    float specularPower = 32.0;
    float3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), specularPower);
    float3 specular = spec * lightColor * intensity * 0.3; // 낮은 스페큘러
    
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
    // 정규화
    float3 normal = normalize(input.Normal);
    float3 viewDir = normalize(float3(0.0, 0.0, -5.0) - input.WorldPos);
    
    // 최종 조명 계산
    float3 result = input.Color.rgb * 0.2; // 낮은 앰비언트 시작점
    
    // 조명이 있는 경우 계산
    if (LightCount > 0)
    {
        // 사용 가능한 모든 조명 처리
        for (int i = 0; i < LightCount; i++)
        {
            int lightType = int(Lights[i].Position.w);
            
            if (lightType == 0) // 방향성 조명
            {
                result += CalculateDirectionalLight(normal, viewDir, i, input.Color);
            }
            else if (lightType == 1) // 점 조명
            {
                result += CalculatePointLight(normal, input.WorldPos, viewDir, i, input.Color);
            }
            else if (lightType == 2) // 스포트라이트
            {
                result += CalculateSpotLight(normal, input.WorldPos, viewDir, i, input.Color);
            }
        }
    }
    else
    {
        // 조명이 없는 경우 간단한 조명 계산
        float3 lightDir = normalize(float3(0.5, 0.5, -0.5));
        float diff = max(0.2, dot(normal, lightDir)); // 최소 밝기 보장
        result = input.Color.rgb * diff;
    }
    
    // 최종 색상 = 오브젝트 색상 * 조명 계수
    float4 finalColor = float4(result, input.Color.a);
    
    // HDR 톤 매핑
    finalColor.rgb = finalColor.rgb / (finalColor.rgb + float3(1.0, 1.0, 1.0));
    
    return finalColor;
}
)";


RoomModel::RoomModel()
{
    CreateRoom();
}

RoomModel::~RoomModel()
{
    Release();
}

void RoomModel::UpdateRoom()
{
    // 방 데이터 재생성
    CreateRoom();

    // 버퍼 업데이트
    if (device) {
        UpdateBuffers(device);
    }
}

bool RoomModel::UpdateBuffers(ID3D11Device* device)
{
    // 기존 버퍼 해제
    if (vertexBuffer) {
        vertexBuffer->Release();
        vertexBuffer = nullptr;
    }

    if (indexBuffer) {
        indexBuffer->Release();
        indexBuffer = nullptr;
    }

    // 새 버퍼 생성
    return CreateBuffers(device);
}


bool RoomModel::Initialize(ID3D11Device* device)
{
    this->device = device;

    if (!CreateBuffers(device)) {
        return false;
    }

    if (!CreateShaders(device)) {
        return false;
    }

    // 래스터라이저 상태 생성
    D3D11_RASTERIZER_DESC rastDesc;
    ZeroMemory(&rastDesc, sizeof(rastDesc));
    rastDesc.FillMode = D3D11_FILL_SOLID;
    rastDesc.CullMode = D3D11_CULL_FRONT;    // ← 앞면(front)만 컬링
    rastDesc.FrontCounterClockwise = FALSE;
    device->CreateRasterizerState(&rastDesc, &rasterizerState);

    D3D11_RASTERIZER_DESC wfDesc = rastDesc;  // 앞서 만든 솔리드용 desc 복사
    wfDesc.FillMode = D3D11_FILL_WIREFRAME;
    wfDesc.CullMode = D3D11_CULL_NONE;        // 양쪽 면 모두 선으로 보이도록
    device->CreateRasterizerState(&wfDesc, &wireframeRasterizerState);

    // 블렌드 상태 생성 (투명 창문용)
    D3D11_BLEND_DESC blendDesc;
    ZeroMemory(&blendDesc, sizeof(blendDesc));
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    device->CreateBlendState(&blendDesc, &blendState);
    return true;
}

void RoomModel::CreateRoom()
{
    vertices.clear();
    indices.clear();

    // 방의 크기 및 위치 정의
    float w = roomWidth / 2.0f;
    float h = roomHeight / 2.0f;
    float d = roomDepth / 2.0f;

    // 방의 8개 모서리 정의
    XMFLOAT3 p1(-w, -h, -d); // 바닥 사각형의 네 모서리
    XMFLOAT3 p2(w, -h, -d);
    XMFLOAT3 p3(w, -h, d);
    XMFLOAT3 p4(-w, -h, d);
    XMFLOAT3 p5(-w, h, -d); // 천장 사각형의 네 모서리
    XMFLOAT3 p6(w, h, -d);
    XMFLOAT3 p7(w, h, d);
    XMFLOAT3 p8(-w, h, d);

    // 바닥 - 바닥 색상 사용
    AddWall(vertices, indices, p1, p2, p3, p4, floorColor);

    // 천장 - 천장 색상 사용
    AddWall(vertices, indices, p8, p7, p6, p5, ceilingColor);

    // 뒷벽 - 벽 색상 사용
    AddWall(vertices, indices, p1, p5, p6, p2, wallColor);

    // 왼쪽 벽 - 벽 색상 사용
    AddWall(vertices, indices, p4, p8, p5, p1, wallColor);

    // 오른쪽 벽 - 벽 색상 사용 + 창문 (hasWindow가 true인 경우)
    AddWall(vertices, indices, p2, p6, p7, p3, wallColor, hasWindow);

    // 앞벽 - 벽 색상 사용
    AddWall(vertices, indices, p3, p7, p8, p4, wallColor);
    indexCount = static_cast<UINT>(indices.size());
}

void RoomModel::AddWall(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
    XMFLOAT3 p1, XMFLOAT3 p2, XMFLOAT3 p3, XMFLOAT3 p4,
    XMFLOAT4 color, bool isWindow)
{
    // 벽의 법선 계산 (p1, p2, p3 삼각형 사용)
    XMVECTOR v1 = XMLoadFloat3(&p2) - XMLoadFloat3(&p1);
    XMVECTOR v2 = XMLoadFloat3(&p3) - XMLoadFloat3(&p2);
    XMVECTOR normal = XMVector3Cross(v1, v2);
    normal = XMVector3Normalize(normal);

    XMFLOAT3 normalFloat;
    XMStoreFloat3(&normalFloat, normal);

    // 기본 텍스처 좌표
    XMFLOAT2 tc1(0.0f, 1.0f);
    XMFLOAT2 tc2(1.0f, 1.0f);
    XMFLOAT2 tc3(1.0f, 0.0f);
    XMFLOAT2 tc4(0.0f, 0.0f);

    uint32_t baseIndex = static_cast<uint32_t>(vertices.size());

    // 창문인 경우 처리
    if (isWindow) {
        // 창문의 크기와 위치 정의 (벽 중앙에 위치)
        float windowWidth = (roomWidth * 0.6f) / 2.0f; // 벽 너비의 60%
        float windowHeight = (roomHeight * 0.4f) / 2.0f; // 벽 높이의 40%

        // 중심점 계산
        XMVECTOR center = (XMLoadFloat3(&p1) + XMLoadFloat3(&p2) + XMLoadFloat3(&p3) + XMLoadFloat3(&p4)) / 4.0f;
        XMFLOAT3 centerPoint;
        XMStoreFloat3(&centerPoint, center);

        // 창문 모서리 계산
        XMFLOAT3 wp1(centerPoint.x - windowWidth, centerPoint.y - windowHeight, centerPoint.z);
        XMFLOAT3 wp2(centerPoint.x + windowWidth, centerPoint.y - windowHeight, centerPoint.z);
        XMFLOAT3 wp3(centerPoint.x + windowWidth, centerPoint.y + windowHeight, centerPoint.z);
        XMFLOAT3 wp4(centerPoint.x - windowWidth, centerPoint.y + windowHeight, centerPoint.z);

        // 창문은 반투명 파란색
        XMFLOAT4 windowColor(0.6f, 0.8f, 1.0f, 0.5f);

        // 창문 자체 추가 (반투명)
        vertices.push_back({ wp1, normalFloat, XMFLOAT2(0.2f, 0.7f), windowColor });
        vertices.push_back({ wp2, normalFloat, XMFLOAT2(0.8f, 0.7f), windowColor });
        vertices.push_back({ wp3, normalFloat, XMFLOAT2(0.8f, 0.3f), windowColor });
        vertices.push_back({ wp4, normalFloat, XMFLOAT2(0.2f, 0.3f), windowColor });


        // 창문 주변 벽 부분 추가 (아래, 위, 왼쪽, 오른쪽)
        // 아래 부분
        vertices.push_back({ p1, normalFloat, tc1, color });
        vertices.push_back({ p2, normalFloat, tc2, color });
        vertices.push_back({ wp2, normalFloat, XMFLOAT2(0.8f, 0.7f), color });
        vertices.push_back({ wp1, normalFloat, XMFLOAT2(0.2f, 0.7f), color });

        indices.push_back(baseIndex);
        indices.push_back(baseIndex + 1);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 3);

        baseIndex += 4;

        // 위 부분
        vertices.push_back({ wp4, normalFloat, XMFLOAT2(0.2f, 0.3f), color });
        vertices.push_back({ wp3, normalFloat, XMFLOAT2(0.8f, 0.3f), color });
        vertices.push_back({ p3, normalFloat, tc3, color });
        vertices.push_back({ p4, normalFloat, tc4, color });

        indices.push_back(baseIndex);
        indices.push_back(baseIndex + 1);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 3);

        baseIndex += 4;

        // 왼쪽 부분
        vertices.push_back({ p1, normalFloat, tc1, color });
        vertices.push_back({ wp1, normalFloat, XMFLOAT2(0.2f, 0.7f), color });
        vertices.push_back({ wp4, normalFloat, XMFLOAT2(0.2f, 0.3f), color });
        vertices.push_back({ p4, normalFloat, tc4, color });

        indices.push_back(baseIndex);
        indices.push_back(baseIndex + 1);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 3);

        baseIndex += 4;

        // 오른쪽 부분
        vertices.push_back({ p2, normalFloat, tc2, color });
        vertices.push_back({ p3, normalFloat, tc3, color });
        vertices.push_back({ wp3, normalFloat, XMFLOAT2(0.8f, 0.3f), color });
        vertices.push_back({ wp2, normalFloat, XMFLOAT2(0.8f, 0.7f), color });

        indices.push_back(baseIndex);
        indices.push_back(baseIndex + 1);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 3);

        baseIndex += 4;

        // 창문 자체 추가 (반투명)
        vertices.push_back({ wp1, normalFloat, XMFLOAT2(0.2f, 0.7f), windowColor });
        vertices.push_back({ wp2, normalFloat, XMFLOAT2(0.8f, 0.7f), windowColor });
        vertices.push_back({ wp3, normalFloat, XMFLOAT2(0.8f, 0.3f), windowColor });
        vertices.push_back({ wp4, normalFloat, XMFLOAT2(0.2f, 0.3f), windowColor });

        indices.push_back(baseIndex);
        indices.push_back(baseIndex + 1);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 3);
    }
    else {
        // 일반 벽면 추가
        vertices.push_back({ p1, normalFloat, tc1, color });
        vertices.push_back({ p2, normalFloat, tc2, color });
        vertices.push_back({ p3, normalFloat, tc3, color });
        vertices.push_back({ p4, normalFloat, tc4, color });

        indices.push_back(baseIndex);
        indices.push_back(baseIndex + 1);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 3);
    }
}

bool RoomModel::CreateBuffers(ID3D11Device* device)
{
    // 버텍스 버퍼 생성
    D3D11_BUFFER_DESC vbDesc;
    ZeroMemory(&vbDesc, sizeof(vbDesc));
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * vertices.size());
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA vbData;
    ZeroMemory(&vbData, sizeof(vbData));
    vbData.pSysMem = vertices.data();

    HRESULT hr = device->CreateBuffer(&vbDesc, &vbData, &vertexBuffer);
    if (FAILED(hr)) {
        return false;
    }

    // 인덱스 버퍼 생성
    D3D11_BUFFER_DESC ibDesc;
    ZeroMemory(&ibDesc, sizeof(ibDesc));
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * indices.size());
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA ibData;
    ZeroMemory(&ibData, sizeof(ibData));
    ibData.pSysMem = indices.data();

    hr = device->CreateBuffer(&ibDesc, &ibData, &indexBuffer);
    if (FAILED(hr)) {
        return false;
    }

    return true;
}

bool RoomModel::CreateShaders(ID3D11Device* device)
{
    // 셰이더 컴파일 및 생성
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    // 정점 셰이더 컴파일
    HRESULT hr = D3DCompile(roomVertexShaderCode, strlen(roomVertexShaderCode), "VS", nullptr, nullptr, "main", "vs_4_0", 0, 0, &vsBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        return false;
    }

    // 픽셀 셰이더 컴파일
    hr = D3DCompile(roomPixelShaderCode, strlen(roomPixelShaderCode), "PS", nullptr, nullptr, "main", "ps_4_0", 0, 0, &psBlob, &errorBlob);
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

    // 입력 레이아웃 생성
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 }
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

// RoomModel.cpp 수정 - Render 함수 수정
void RoomModel::Render(ID3D11DeviceContext* deviceContext, const Camera& camera, LightManager* lightManager)
{
    // 셰이더 설정
    deviceContext->VSSetShader(vertexShader, nullptr, 0);
    deviceContext->PSSetShader(pixelShader, nullptr, 0);
    deviceContext->IASetInputLayout(inputLayout);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 상수 버퍼 업데이트
    ConstantBuffer cb;
    cb.World = XMMatrixTranspose(XMMatrixIdentity());
    cb.View = XMMatrixTranspose(camera.GetViewMatrix());
    cb.Projection = XMMatrixTranspose(camera.GetProjectionMatrix());
    cb.AmbientColor = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);

    deviceContext->UpdateSubresource(constantBuffer, 0, nullptr, &cb, 0, 0);
    deviceContext->VSSetConstantBuffers(0, 1, &constantBuffer);
    deviceContext->PSSetConstantBuffers(0, 1, &constantBuffer);

    // 조명 정보 설정 (lightManager가 있는 경우)
    if (lightManager)
    {
        lightManager->UpdateLightBuffer(deviceContext);
        lightManager->SetLightBuffer(deviceContext);
    }

    // 버텍스 및 인덱스 버퍼 설정
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

    // 래스터라이저 상태 설정
    deviceContext->RSSetState(rasterizerState);

    // 블렌드 상태 설정 (창문의 투명도를 위해)
    float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    deviceContext->OMSetBlendState(blendState, blendFactor, 0xffffffff);

    // 그리기
    deviceContext->DrawIndexed(indexCount, 0, 0);

    deviceContext->RSSetState(wireframeRasterizerState);
    deviceContext->DrawIndexed(indexCount, 0, 0);
    // (c) 이후 다른 모델을 위해 기본 상태 복원
    deviceContext->RSSetState(rasterizerState);
}

void RoomModel::Release()
{
    if (vertexBuffer) { vertexBuffer->Release(); vertexBuffer = nullptr; }
    if (indexBuffer) { indexBuffer->Release(); indexBuffer = nullptr; }
    if (vertexShader) { vertexShader->Release(); vertexShader = nullptr; }
    if (pixelShader) { pixelShader->Release(); pixelShader = nullptr; }
    if (inputLayout) { inputLayout->Release(); inputLayout = nullptr; }
    if (constantBuffer) { constantBuffer->Release(); constantBuffer = nullptr; }
    if (rasterizerState) { rasterizerState->Release(); rasterizerState = nullptr; }
    if (blendState) { blendState->Release(); blendState = nullptr; }
    wireframeRasterizerState->Release();
    wireframeRasterizerState = nullptr;
}
