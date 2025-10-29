// DummyCharacter.cpp - 완전한 구현
#include "DummyCharacter.h"
#include "Camera.h"  // Camera 클래스 정의를 위해 추가

#include <d3dcompiler.h>
#include <vector> 
#include <cmath>

// 셰이더 코드
const char* characterVertexShaderCode = R"(
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float4 Color;
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
    float4 Color : COLOR;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;
    
    // 위치 변환
    float4 pos = float4(input.Pos, 1.0f);
    pos = mul(pos, World);
    pos = mul(pos, View);
    pos = mul(pos, Projection);
    output.Pos = pos;
    
    // 법선 변환
    output.Normal = mul(input.Normal, (float3x3)World);
    output.Normal = normalize(output.Normal);
    
    // 텍스처 좌표 및 색상 전달
    output.Tex = input.Tex;
    output.Color = Color;
    
    return output;
}
)";

const char* characterPixelShaderCode = R"(
struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Normal : NORMAL;
    float2 Tex : TEXCOORD0;
    float4 Color : COLOR;
};

float4 main(PS_INPUT input) : SV_Target
{
    // 간단한 조명 계산
    float3 lightDir = normalize(float3(0.5, 0.5, -0.5));
    float diff = max(0.2, dot(input.Normal, lightDir)); // 최소 밝기 보장
    
    // 최종 색상 = 오브젝트 색상 * 조명 계수
    float4 finalColor = input.Color * diff;
    
    return finalColor;
}
)";

// 상수 버퍼 구조체
struct CharacterConstantBuffer
{
    XMMATRIX World;
    XMMATRIX View;
    XMMATRIX Projection;
    XMFLOAT4 Color;
};

DummyCharacter::DummyCharacter() :
    position(0.0f, 0.0f, 0.0f),
    rotation(0.0f),
    pitch(0.0f),     
    height(1.8f),         // 180cm 키
    eyeHeight(1.62f),     // 눈높이 (키의 90%)
    radius(0.3f),         // 30cm 반경
    vertexBuffer(nullptr),
    indexBuffer(nullptr),
    vertexShader(nullptr),
    pixelShader(nullptr),
    inputLayout(nullptr),
    constantBuffer(nullptr),
    rasterizerState(nullptr),
    indexCount(0)
{
}

DummyCharacter::~DummyCharacter()
{
    Release();
}

// DummyCharacter.cpp - 피치 설정 함수 추가
void DummyCharacter::SetPitch(float newPitch) {
    // 피치 값 제한 (-89~89도 범위로 제한하여 뒤집어지는 것 방지)
    pitch = max(-89.0f, min(89.0f, newPitch));
}

bool DummyCharacter::Initialize(ID3D11Device* device)
{
    // 캐릭터 메시 생성
    CreateCharacterMesh();

    // 버퍼 생성
    if (!CreateBuffers(device)) {
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
    rastDesc.CullMode = D3D11_CULL_BACK;
    rastDesc.FrontCounterClockwise = FALSE;
    device->CreateRasterizerState(&rastDesc, &rasterizerState);

    return true;
}

XMFLOAT3 DummyCharacter::GetFirstPersonCameraPosition() const {
    // 캐릭터 위치에서 눈높이만큼 위로 올린 위치
    return XMFLOAT3(position.x, position.y + eyeHeight, position.z);
}

XMFLOAT3 DummyCharacter::GetFirstPersonCameraRotation() const {
    // 기본적으로 캐릭터 요(yaw) 회전만 사용하고 피치와 롤은 0으로 설정
    return XMFLOAT3(pitch, rotation, 0.0f);
}

void DummyCharacter::SetPosition(const XMFLOAT3& newPosition) {
    position = newPosition;
}

void DummyCharacter::SetRotation(float yaw) {
    // 회전 값을 0~360도 범위로 정규화
    rotation = fmodf(yaw, 360.0f);
    if (rotation < 0) rotation += 360.0f;
}

void DummyCharacter::MoveForward(float distance) {
    // 캐릭터 요(yaw) 각도를 라디안으로 변환
    float yawRadian = XMConvertToRadians(rotation);

    // 전진 방향 계산 (요 각도 기준)
    position.x += distance * sinf(yawRadian);
    position.z += distance * cosf(yawRadian);
}

void DummyCharacter::MoveRight(float distance) {
    // 캐릭터 요(yaw) 각도를 라디안으로 변환 
    float yawRadian = XMConvertToRadians(rotation);

    // 오른쪽 방향 계산 (요 각도 기준)
    position.x += distance * cosf(yawRadian);
    position.z -= distance * sinf(yawRadian);
}

bool DummyCharacter::CheckCollision(const XMFLOAT3& newPosition, float roomWidth, float roomHeight, float roomDepth) {
    // 방 경계와의 충돌 체크 (여백 포함)
    float halfWidth = roomWidth / 2.0f - radius - 0.1f; // 0.1m 여백 추가
    float halfDepth = roomDepth / 2.0f - radius - 0.1f;

    if (newPosition.x < -halfWidth || newPosition.x > halfWidth ||
        newPosition.z < -halfDepth || newPosition.z > halfDepth) {
        return true; // 충돌 발생
    }

    return false; // 충돌 없음
}

void DummyCharacter::CreateCharacterMesh() {
    // 기존 데이터 초기화
    vertices.clear();
    indices.clear();

    // 실린더 설정
    const int segments = 16;
    const float topRadius = radius; // 상단 반경
    const float bottomRadius = radius; // 하단 반경
    const float topHeight = height; // 상단 높이
    const float bottomHeight = 0.0f; // 하단 높이

    // 상단 중심점
    vertices.push_back({ XMFLOAT3(0, topHeight, 0), XMFLOAT3(0, 1, 0), XMFLOAT2(0.5f, 0.5f) });

    // 상단 테두리 정점
    for (int i = 0; i < segments; i++) {
        float angle = XM_2PI * i / segments;
        float x = topRadius * cosf(angle);
        float z = topRadius * sinf(angle);

        vertices.push_back({
            XMFLOAT3(x, topHeight, z),
            XMFLOAT3(0, 1, 0),
            XMFLOAT2(0.5f + 0.5f * cosf(angle), 0.5f + 0.5f * sinf(angle))
            });
    }

    // 하단 테두리 정점
    for (int i = 0; i < segments; i++) {
        float angle = XM_2PI * i / segments;
        float x = bottomRadius * cosf(angle);
        float z = bottomRadius * sinf(angle);

        vertices.push_back({
            XMFLOAT3(x, bottomHeight, z),
            XMFLOAT3(0, -1, 0),
            XMFLOAT2(0.5f + 0.5f * cosf(angle), 0.5f + 0.5f * sinf(angle))
            });
    }

    // 하단 중심점
    vertices.push_back({ XMFLOAT3(0, bottomHeight, 0), XMFLOAT3(0, -1, 0), XMFLOAT2(0.5f, 0.5f) });

    // 상단 면 인덱스
    for (int i = 0; i < segments; i++) {
        indices.push_back(0); // 상단 중심점
        indices.push_back(1 + (i + 1) % segments); // 다음 정점
        indices.push_back(1 + i); // 현재 정점
    }

    // 실린더 옆면 인덱스
    for (int i = 0; i < segments; i++) {
        indices.push_back(1 + i); // 상단 현재
        indices.push_back(1 + (i + 1) % segments); // 상단 다음
        indices.push_back(1 + segments + i); // 하단 현재

        indices.push_back(1 + segments + i); // 하단 현재
        indices.push_back(1 + (i + 1) % segments); // 상단 다음
        indices.push_back(1 + segments + (i + 1) % segments); // 하단 다음
    }

    // 하단 면 인덱스
    int bottomCenter = vertices.size() - 1;
    for (int i = 0; i < segments; i++) {
        indices.push_back(bottomCenter); // 하단 중심점
        indices.push_back(1 + segments + i); // 현재 정점
        indices.push_back(1 + segments + (i + 1) % segments); // 다음 정점
    }

    // 측면 법선 계산
    for (int i = 0; i < segments; i++) {
        int topIndex = 1 + i;
        int bottomIndex = 1 + segments + i;

        // 정점 위치 가져오기
        XMFLOAT3 topPos = vertices[topIndex].Position;
        XMFLOAT3 bottomPos = vertices[bottomIndex].Position;

        // 방향 벡터 계산
        XMVECTOR direction = XMVectorSet(
            topPos.x - bottomPos.x,
            topPos.y - bottomPos.y,
            topPos.z - bottomPos.z,
            0.0f
        );

        // 법선 벡터 계산 (방향 벡터 정규화)
        XMVECTOR normal = XMVector3Normalize(direction);

        // 법선 벡터 저장
        XMFLOAT3 normalFloat;
        XMStoreFloat3(&normalFloat, normal);

        vertices[topIndex].Normal = normalFloat;
        vertices[bottomIndex].Normal = normalFloat;
    }

    // 인덱스 카운트 저장
    indexCount = static_cast<UINT>(indices.size());
}


bool DummyCharacter::CreateBuffers(ID3D11Device* device) {
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
bool DummyCharacter::CreateShaders(ID3D11Device* device) {
    // 셰이더 컴파일 및 생성
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    // 정점 셰이더 컴파일
    HRESULT hr = D3DCompile(characterVertexShaderCode, strlen(characterVertexShaderCode), "VS", nullptr, nullptr, "main", "vs_4_0", 0, 0, &vsBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        return false;
    }

    // 픽셀 셰이더 컴파일
    hr = D3DCompile(characterPixelShaderCode, strlen(characterPixelShaderCode), "PS", nullptr, nullptr, "main", "ps_4_0", 0, 0, &psBlob, &errorBlob);
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
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }
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
    cbDesc.ByteWidth = sizeof(CharacterConstantBuffer);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    hr = device->CreateBuffer(&cbDesc, nullptr, &constantBuffer);
    if (FAILED(hr)) {
        return false;
    }

    return true;
}

void DummyCharacter::Render(ID3D11DeviceContext* deviceContext, const Camera& camera) {
    // 셰이더 설정
    deviceContext->VSSetShader(vertexShader, nullptr, 0);
    deviceContext->PSSetShader(pixelShader, nullptr, 0);
    deviceContext->IASetInputLayout(inputLayout);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 상수 버퍼 업데이트
    CharacterConstantBuffer cb;

    // 월드 행렬: 캐릭터 위치와 회전 적용
    XMMATRIX rotationMatrix = XMMatrixRotationY(XMConvertToRadians(rotation));
    XMMATRIX translationMatrix = XMMatrixTranslation(position.x, position.y, position.z);
    XMMATRIX worldMatrix = rotationMatrix * translationMatrix;

    // 행렬 설정
    cb.World = XMMatrixTranspose(worldMatrix);
    cb.View = XMMatrixTranspose(camera.GetViewMatrix());
    cb.Projection = XMMatrixTranspose(camera.GetProjectionMatrix());

    // 캐릭터 색상 (피부톤 또는 의류 색상)
    cb.Color = XMFLOAT4(0.8f, 0.6f, 0.5f, 1.0f);

    deviceContext->UpdateSubresource(constantBuffer, 0, nullptr, &cb, 0, 0);
    deviceContext->VSSetConstantBuffers(0, 1, &constantBuffer);
    deviceContext->PSSetConstantBuffers(0, 1, &constantBuffer);

    // 버텍스 및 인덱스 버퍼 설정
    UINT stride = sizeof(Vertex);  // 클래스의 Vertex 구조체 사용
    UINT offset = 0;
    deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

    // 래스터라이저 상태 설정
    deviceContext->RSSetState(rasterizerState);

    // 그리기
    deviceContext->DrawIndexed(indexCount, 0, 0);
}

void DummyCharacter::Release() {
    if (vertexBuffer) { vertexBuffer->Release(); vertexBuffer = nullptr; }
    if (indexBuffer) { indexBuffer->Release(); indexBuffer = nullptr; }
    if (vertexShader) { vertexShader->Release(); vertexShader = nullptr; }
    if (pixelShader) { pixelShader->Release(); pixelShader = nullptr; }
    if (inputLayout) { inputLayout->Release(); inputLayout = nullptr; }
    if (constantBuffer) { constantBuffer->Release(); constantBuffer = nullptr; }
    if (rasterizerState) { rasterizerState->Release(); rasterizerState = nullptr; }
}
