#pragma once
#include <d3d11.h>
#include <directxmath.h>
#include <vector>
#include <memory>
#include "Camera.h"
#include "LightManager.h"
using namespace DirectX;




class RoomModel
{
public:
    // 정점 구조체 


    struct Vertex
    {
        XMFLOAT3 Position;
        XMFLOAT3 Normal;
        XMFLOAT2 TexCoord;
        XMFLOAT4 Color;  // 색상 정보 추가
    };

    // 생성자 및 소멸자
    RoomModel();
    ~RoomModel();

    // 초기화 함수
    bool Initialize(ID3D11Device* device);

    // Render 함수 수정 - 조명 관리자 추가
    void Render(ID3D11DeviceContext* deviceContext, const Camera& camera, LightManager* lightManager = nullptr);


    // 리소스 해제
    void Release();

    // 속성 getter/setter
    float GetRoomWidth() const { return roomWidth; }
    float GetRoomHeight() const { return roomHeight; }
    float GetRoomDepth() const { return roomDepth; }
    XMFLOAT4 GetFloorColor() const { return floorColor; }
    XMFLOAT4 GetCeilingColor() const { return ceilingColor; }
    XMFLOAT4 GetWallColor() const { return wallColor; }
    XMFLOAT4 GetWindowColor() const { return windowColor; }
    bool GetHasWindow() const { return hasWindow; }

    void SetRoomWidth(float width) { roomWidth = width; UpdateRoom(); }
    void SetRoomHeight(float height) { roomHeight = height; UpdateRoom(); }
    void SetRoomDepth(float depth) { roomDepth = depth; UpdateRoom(); }
    void SetFloorColor(const XMFLOAT4& color) { floorColor = color; UpdateRoom(); }
    void SetCeilingColor(const XMFLOAT4& color) { ceilingColor = color; UpdateRoom(); }
    void SetWallColor(const XMFLOAT4& color) { wallColor = color; UpdateRoom(); }
    void SetWindowColor(const XMFLOAT4& color) { windowColor = color; UpdateRoom(); }
    void SetHasWindow(bool hasWin) { hasWindow = hasWin; UpdateRoom(); }

    // 방 업데이트 - 속성 변경 시 호출
    void UpdateRoom();

private:
    // RoomModel.h 에…
    ID3D11RasterizerState* wireframeRasterizerState = nullptr;
    // 벽 생성 도우미 함수
    void CreateRoom();
    void AddWall(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
        XMFLOAT3 p1, XMFLOAT3 p2, XMFLOAT3 p3, XMFLOAT3 p4,
        XMFLOAT4 color, bool isWindow = false);

    // 버퍼 생성 함수
    bool CreateBuffers(ID3D11Device* device);
    bool CreateShaders(ID3D11Device* device);
    bool UpdateBuffers(ID3D11Device* device);

    // 버퍼 및 셰이더
    ID3D11Buffer* vertexBuffer = nullptr;
    ID3D11Buffer* indexBuffer = nullptr;
    ID3D11VertexShader* vertexShader = nullptr;
    ID3D11PixelShader* pixelShader = nullptr;
    ID3D11InputLayout* inputLayout = nullptr;
    ID3D11Buffer* constantBuffer = nullptr;
    ID3D11RasterizerState* rasterizerState = nullptr;
    ID3D11BlendState* blendState = nullptr;

    // 방 속성
    float roomWidth = 20.0f;
    float roomHeight = 10.0f;
    float roomDepth = 20.0f;
    XMFLOAT4 floorColor = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);     // 바닥 색상
    XMFLOAT4 ceilingColor = XMFLOAT4(0.95f, 0.95f, 0.95f, 1.0f); // 천장 색상
    XMFLOAT4 wallColor = XMFLOAT4(0.85f, 0.85f, 0.85f, 1.0f);    // 벽 색상
    XMFLOAT4 windowColor = XMFLOAT4(0.6f, 0.8f, 1.0f, 0.5f);     // 창문 색상
    bool hasWindow = false;                                       // 창문 유무

    // 디바이스 참조 저장
    ID3D11Device* device = nullptr;

    // 정점 및 인덱스 데이터
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    UINT indexCount = 0;
};
