#pragma once
#include <directxmath.h>
#include <d3d11.h>
#include <string>
#include <vector>

// 전방 선언(forward declaration)
class Camera;

using namespace DirectX;

class DummyCharacter {
public:
    struct Vertex {
        XMFLOAT3 Position;
        XMFLOAT3 Normal;
        XMFLOAT2 TexCoord;
    };

    DummyCharacter();
    ~DummyCharacter();

    bool Initialize(ID3D11Device* device);
    void Render(ID3D11DeviceContext* deviceContext, const Camera& camera);
    void Release();

    // 위치, 회전 설정
    // 피치 관련 메소드 추가
    void SetPitch(float pitch);
    float GetPitch() const { return pitch; }

    void SetPosition(const XMFLOAT3& position);
    XMFLOAT3 GetPosition() const { return position; }
    void SetRotation(float yaw);
    float GetRotation() const { return rotation; }

    // 1인칭 시점 카메라 위치 계산
    XMFLOAT3 GetFirstPersonCameraPosition() const;
    // GetFirstPersonCameraRotation 
    XMFLOAT3 GetFirstPersonCameraRotation() const;

    // 캐릭터 이동
    void MoveForward(float distance);
    void MoveRight(float distance);

    // 간단한 충돌 체크
    bool CheckCollision(const XMFLOAT3& newPosition, float roomWidth, float roomHeight, float roomDepth);

private:
    // 캐릭터 데이터
    XMFLOAT3 position;     // 캐릭터 위치
    float rotation;        // Y축 회전 
    float pitch;     // X축 회전 (pitch) - 추가
    float height;          // 캐릭터 키 (1.8m = 180cm)
    float eyeHeight;       // 눈높이 (키의 약 90%)
    float radius;          // 캐릭터 충돌 반경

    // 캐릭터 메시 데이터
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // 캐릭터 메시 (간단한 실린더)
    ID3D11Buffer* vertexBuffer;
    ID3D11Buffer* indexBuffer;
    UINT indexCount;

    // 렌더링 리소스
    ID3D11VertexShader* vertexShader;
    ID3D11PixelShader* pixelShader;
    ID3D11InputLayout* inputLayout;
    ID3D11Buffer* constantBuffer;
    ID3D11RasterizerState* rasterizerState;

    // 캐릭터 메시 생성
    void CreateCharacterMesh();
    bool CreateBuffers(ID3D11Device* device);
    bool CreateShaders(ID3D11Device* device);
};
