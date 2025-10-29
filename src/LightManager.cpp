#include "LightManager.h"
#include "imgui.h"
#include "EnhancedUI.h"

LightManager::LightManager() : lightConstantBuffer(nullptr) {
}

LightManager::~LightManager() {
    Release();
}

bool LightManager::Initialize(ID3D11Device* device) {
    // 기본 조명 추가 
    AddLight(LIGHT_DIRECTIONAL); // 기본 방향성 조명 

    // 상수 버퍼 생성
    return CreateLightBuffer(device);
}

void LightManager::Release() {
    if (lightConstantBuffer) {
        lightConstantBuffer->Release();
        lightConstantBuffer = nullptr;
    }

    lights.clear();
}

int LightManager::AddLight(LightType type) {
    // 최대 8개의 조명만 지원
    if (lights.size() >= 8) {
        return -1;
    }

    auto light = std::make_shared<Light>(type);
    lights.push_back(light);

    return static_cast<int>(lights.size() - 1);
}

void LightManager::RemoveLight(int index) {
    if (index >= 0 && index < lights.size()) {
        lights.erase(lights.begin() + index);
    }
}

Light* LightManager::GetLight(int index) {
    if (index >= 0 && index < lights.size()) {
        return lights[index].get();
    }
    return nullptr;
}

int LightManager::GetLightCount() const {
    return static_cast<int>(lights.size());
}

bool LightManager::CreateLightBuffer(ID3D11Device* device) {
    // 상수 버퍼 설명 설정
    D3D11_BUFFER_DESC bufferDesc;
    ZeroMemory(&bufferDesc, sizeof(bufferDesc));
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = sizeof(LightBufferType);
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    // 상수 버퍼 생성
    HRESULT result = device->CreateBuffer(&bufferDesc, nullptr, &lightConstantBuffer);
    if (FAILED(result)) {
        return false;
    }

    return true;
}

void LightManager::UpdateLightBuffer(ID3D11DeviceContext* deviceContext) {
    // 조명 데이터 준비
    LightBufferType lightBuffer;
    ZeroMemory(&lightBuffer, sizeof(LightBufferType));

    // 조명 데이터 설정
    lightBuffer.LightCount = static_cast<int>(lights.size());
    for (int i = 0; i < lights.size() && i < 8; i++) {
        lightBuffer.Lights[i] = lights[i]->GetLightData();
    }

    // 상수 버퍼 매핑
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT result = deviceContext->Map(lightConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(result)) {
        // 데이터 복사
        memcpy(mappedResource.pData, &lightBuffer, sizeof(LightBufferType));
        deviceContext->Unmap(lightConstantBuffer, 0);
    }
}

void LightManager::SetLightBuffer(ID3D11DeviceContext* deviceContext) {
    // 픽셀 셰이더에 조명 상수 버퍼 설정 (레지스터 b1 사용)
    deviceContext->PSSetConstantBuffers(1, 1, &lightConstantBuffer);
}

void LightManager::RenderUI() {
    if (ImGui::Begin("조명 설정", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        // 조명 추가 버튼
        if (ImGui::Button("조명 추가")) {
            if (lights.size() < 8) {
                ImGui::OpenPopup("조명 타입 선택");
            }
        }

        // 조명 타입 선택 팝업
        if (ImGui::BeginPopup("조명 타입 선택")) {
            if (ImGui::MenuItem("방향성 조명")) {
                AddLight(LIGHT_DIRECTIONAL);
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("점 조명")) {
                AddLight(LIGHT_POINT);
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::MenuItem("스포트라이트")) {
                AddLight(LIGHT_SPOT);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();
        ImGui::Text("(최대 8개)");

        ImGui::Separator();

        // 조명 목록 및 속성
        for (int i = 0; i < lights.size(); i++) {
            EnhancedUI::RenderHeader(("조명 #" + std::to_string(i + 1)).c_str());

            // 조명 속성 UI 렌더링
            lights[i]->RenderUI(i);

            // 제거 버튼
            if (lights.size() > 1) { // 최소 하나의 조명은 유지
                if (ImGui::Button(("제거##" + std::to_string(i)).c_str())) {
                    RemoveLight(i);
                    break; // UI 요소가 변경되었으므로 루프 중단
                }
            }

            ImGui::Separator();
        }
    }
    ImGui::End();
}
