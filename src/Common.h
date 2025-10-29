// Common.h
#pragma once
#include <directxmath.h>
using namespace DirectX;

// 바운딩 박스 구조체
struct BoundingBox {
    XMFLOAT3 min;    // 최소 경계점
    XMFLOAT3 max;    // 최대 경계점
    XMFLOAT3 center; // 중심점
    float radius;    // 경계 구의 반지름
};
