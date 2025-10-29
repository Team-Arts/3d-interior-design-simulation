#pragma once
#include "Light.h"
#include <vector>
#include <memory>
#include <d3d11.h>

// 상수 버퍼에 들어갈 조명 데이터 
struct LightBufferType {
    LightData Lights[8];    // 최대 8개 조명 지원
    int LightCount;         // 활성화된 조명 수
    XMFLOAT3 Padding;       // 16바이트 정렬을 위한 패딩
};

class LightManager {
public:
    LightManager();
    ~LightManager();

    // 초기화 및 해제
    bool Initialize(ID3D11Device* device);
    void Release();

    // 조명 추가/제거
    int AddLight(LightType type);
    void RemoveLight(int index);

    // 조명 접근
    Light* GetLight(int index);
    int GetLightCount() const;

    // 조명 상수 버퍼 업데이트 및 설정
    void UpdateLightBuffer(ID3D11DeviceContext* deviceContext);
    void SetLightBuffer(ID3D11DeviceContext* deviceContext);

    // UI 렌더링
    void RenderUI();

private:
    std::vector<std::shared_ptr<Light>> lights;
    ID3D11Buffer* lightConstantBuffer;

    // 상수 버퍼 생성
    bool CreateLightBuffer(ID3D11Device* device);
};
