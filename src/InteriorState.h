#pragma once
#include <directxmath.h>
#include <string>
#include <vector>

using namespace DirectX;

// 인테리어 상태 저장을 위한 구조체들
namespace InteriorDesign
{

// 모델 정보 구조체
struct SavedModelInfo
{
    std::string name;
    std::string filePath;
    std::string type; // "OBJ" or "GLB"
    XMFLOAT3 position;
    XMFLOAT3 rotation;
    XMFLOAT3 scale;
    bool visible;
};

// 방 정보 구조체
struct SavedRoomInfo
{
    float width;
    float height;
    float depth;
    XMFLOAT4 floorColor;
    XMFLOAT4 ceilingColor;
    XMFLOAT4 wallColor;
    XMFLOAT4 windowColor;
    bool hasWindow;
};

// 조명 정보 구조체
struct SavedLightInfo
{
    int type; // 0: Directional, 1: Point, 2: Spot
    XMFLOAT3 position;
    XMFLOAT3 direction;
    XMFLOAT3 color;
    float intensity;
    float range;
    float attenuation;
    float innerCone;
    float outerCone;
};

// 카메라 정보 구조체
struct SavedCameraInfo
{
    XMFLOAT3 position;
    XMFLOAT3 rotation;
    float fieldOfView;
    float nearPlane;
    float farPlane;
    int mode; // 0: Free, 1: FirstPerson
};

// 전체 인테리어 상태
struct InteriorState
{
    std::string version = "1.0";
    SavedRoomInfo room;
    SavedCameraInfo camera;
    std::vector<SavedModelInfo> models;
    std::vector<SavedLightInfo> lights;
    std::string createdAt;
    std::string description;
};

} // namespace InteriorDesign