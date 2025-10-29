#pragma once
#include "Model.h"
#include "LightManager.h"
#include "GltfLoader.h"  // GLB 로더 헤더 포함
#include "DummyCharacter.h"  // 추가
#include "Common.h"
#include "EnhancedUI.h"
#include "RoomModel.h"
#include "Camera.h"
#include <vector>
#include <memory>
#include <string>
#include <d3d11.h>
#include <windows.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <future>

// 모델 타입 enum 
enum ModelType {
    MODEL_OBJ,
    MODEL_GLB
};

// 레이 정의 구조체 (레이캐스팅용)
struct Ray {
    XMFLOAT3 origin;
    XMFLOAT3 direction;
};


// 로딩 진행 상태를 표시하기 위한 구조체
struct LoadingProgress {
    std::string filename;
    float progress;
    bool isComplete;
    bool isError;
    std::string errorMessage;
};

// 베이스 모델 인터페이스
class BaseModel {
public:
    virtual ~BaseModel() = default;
    virtual void Render(ID3D11DeviceContext* deviceContext, const Camera& camera) = 0;
    virtual void Release() = 0;
    virtual XMFLOAT3 GetPosition() const = 0;
    virtual void SetPosition(const XMFLOAT3& position) = 0;
    virtual BoundingBox GetBoundingBox() const = 0;
    // 추가 렌더링 함수 - 조명 관리자 지원
    virtual void Render(ID3D11DeviceContext* deviceContext, const Camera& camera, LightManager* lightManager) {
        // 기본 구현은 일반 렌더링 호출
        Render(deviceContext, camera);
    }
};

// OBJ 모델 래퍼 클래스
class ObjModelWrapper : public BaseModel {
public:
    ObjModelWrapper() : model(std::make_shared<Model>()) {}
    ~ObjModelWrapper() override { model->Release(); }

    void Render(ID3D11DeviceContext* deviceContext, const Camera& camera) override {
        //조명없는 기본호출
        model->Render(deviceContext, camera, nullptr);
    }

    // 조명 관리자를 지원하는 확장 렌더링 함수
    void Render(ID3D11DeviceContext* deviceContext, const Camera& camera, LightManager* lightManager) {
        model->Render(deviceContext, camera, lightManager);
    }
    void Release() override { model->Release(); }

    XMFLOAT3 GetPosition() const override {
        return model->GetModelInfo().Position;
    }

    void SetPosition(const XMFLOAT3& position) override {
        model->GetModelInfo().Position = position;
    }

    BoundingBox GetBoundingBox() const override {
        // 모델의 바운딩 박스 계산 (실제 구현에서는 모델 데이터 기반으로 계산)
        BoundingBox box;
        XMFLOAT3 pos = model->GetModelInfo().Position;
        XMFLOAT3 scale = model->GetModelInfo().Scale;
        float size = 1.0f; // 기본 크기 (실제 모델에 맞게 조정 필요)

        box.min = XMFLOAT3(
            pos.x - size * scale.x,
            pos.y - size * scale.y,
            pos.z - size * scale.z
        );

        box.max = XMFLOAT3(
            pos.x + size * scale.x,
            pos.y + size * scale.y,
            pos.z + size * scale.z
        );

        box.center = pos;
        box.radius = size * max(max(scale.x, scale.y), scale.z);

        return box;
    }

    std::shared_ptr<Model> model;
};

// GLB 모델 래퍼 클래스
class GlbModelWrapper : public BaseModel {
public:
    GlbModelWrapper() : model(std::make_shared<GltfLoader>()) {}
    ~GlbModelWrapper() override { model->Release(); }

    void Render(ID3D11DeviceContext* deviceContext, const Camera& camera) override {
        model->Render(deviceContext, camera);
    }

    // 추가: 조명 지원 렌더링 함수 오버라이드
    void Render(ID3D11DeviceContext* deviceContext, const Camera& camera, LightManager* lightManager) override {
        if (model && lightManager) {
            // 조명 정보를 GLB 모델 렌더링에 전달
            model->Render(deviceContext, camera, lightManager);
        }
        else {
            // 조명 관리자가 없으면 기본 렌더링 사용
            model->Render(deviceContext, camera);
        }
    }
    void Release() override { model->Release(); }

    XMFLOAT3 GetPosition() const override {
        return model->GetModelInfo().Position;
    }

    void SetPosition(const XMFLOAT3& position) override {
        model->GetModelInfo().Position = position;

        OutputDebugStringA(("GLB 모델 위치 설정: " + std::to_string(position.x) + "," +
            std::to_string(position.y) + "," + std::to_string(position.z) + "\n").c_str());
    }

    BoundingBox GetBoundingBox() const override {
        // 모델의 바운딩 박스 계산 (실제 구현에서는 모델 데이터 기반으로 계산)
        BoundingBox box;
        XMFLOAT3 pos = model->GetModelInfo().Position;
        XMFLOAT3 scale = model->GetModelInfo().Scale;
        float size = 5.0f; // 기본 크기 (실제 모델에 맞게 조정 필요)

        box.min = XMFLOAT3(
            pos.x - size * scale.x,
            pos.y - size * scale.y,
            pos.z - size * scale.z
        );

        box.max = XMFLOAT3(
            pos.x + size * scale.x,
            pos.y + size * scale.y,
            pos.z + size * scale.z
        );

        box.center = pos;
        box.radius = size * max(max(scale.x, scale.y), scale.z);

        return box;
    }

    std::shared_ptr<GltfLoader> model;
};

class ModelManager
{
public:
    // 생성자 및 소멸자
    ModelManager();
    ~ModelManager();
    void ModelManager::DragSelectedModel(int x, int y, HWND hwnd);
    // 모델 추가 함수들 - OBJ 및 GLB 지원
    void AddModel(const std::string& path, ID3D11Device* device);
    void AddModelAsync(const std::string& path);

    // 특정 타입의 모델 추가 (OBJ/GLB)
    void AddObjModel(const std::string& path, ID3D11Device* device);
    void AddGlbModel(const std::string& path, ID3D11Device* device);
    void AddObjModelAsync(const std::string& path);
    void AddGlbModelAsync(const std::string& path);

    //조명 관리자 관련 함수
     void InitLightManager(ID3D11Device* device);
    LightManager* GetLightManager() { return lightManager.get(); }


    // 모델 제거 함수
    void RemoveModel(int index);

    // 렌더링 함수
    void RenderModels(ID3D11DeviceContext* deviceContext);

    // ImGui 렌더링 함수
    void RenderImGuiWindow(HWND hwnd, ID3D11Device* device, float deltaTime);

    // 프레임 처리 함수
    void ProcessFrame(HWND hwnd, float deltaTime);

    // 리소스 해제
    void Release();

    // 로딩 상태 업데이트
    void UpdateLoadingStatus();

    // RoomModel 관련 함수
    void SetRoomModel(std::shared_ptr<RoomModel> room) { roomModel = room; }
    std::shared_ptr<RoomModel> GetRoomModel() const { return roomModel; }

    // 방 속성 렌더링
    void RenderRoomProperties();

    // 향상된 UI 렌더링 함수들
    void RenderEnhancedUI(HWND hwnd, ID3D11Device* device, float deltaTime);
    void RenderMainInterface(HWND hwnd);
    void RenderToolbar(HWND hwnd);
    void RenderModelBrowser();
    void RenderPropertiesPanel();
    void RenderStatusBar();

    // 향상된 재질 속성 편집 함수들
    void RenderObjMaterialPropertiesEnhanced(std::shared_ptr<Model> model);
    void RenderGlbMaterialPropertiesEnhanced(std::shared_ptr<GltfLoader> model);

    // 카메라 가져오기
    Camera& GetCamera() { return camera; }

    // *** 새로 추가된 마우스 상호작용 관련 함수들 ***
    // 마우스 이벤트 처리
    void OnMouseDown(int x, int y, HWND hwnd);
    void OnMouseMove(int x, int y, HWND hwnd);
    void OnMouseUp(int x, int y);

    // 레이캐스팅 관련 함수들
    Ray CreateRayFromScreenPoint(int screenX, int screenY, int screenWidth, int screenHeight);
    int PickModel(const Ray& ray);
    bool RayIntersectsBoundingBox(const Ray& ray, const BoundingBox& box);
    XMFLOAT3 GetPlaneIntersectionPoint(const Ray& ray, const XMFLOAT3& planeNormal, float planeD);
    // 드래그 축 제한 enum

    enum DragAxis { FREE, X_AXIS, Y_AXIS, Z_AXIS };

    // 드래그 축 제한 설정
    void SetDragAxis(DragAxis axis);

    // 드래그 평면 설정 함수
    void SetupDragPlane(const Ray& ray);

    // 드래그 도움말 및 상태 표시 함수
    void RenderDragHelpPopup();
    void RenderDragStatusInfo();


    //카메라 모드 관련 1인칭 및 자유시점
    void ToggleCameraMode();
    bool IsFirstPersonMode() const { return isFirstPersonMode; }
    void ProcessCharacterInput(HWND hwnd, float deltaTime);
    void InitializeDummyCharacter(ID3D11Device* device);


private:


    float mouseSensitivity = 0.1f;  // 마우스 회전 민감도 추가

    // 파일 다이얼로그를 이용한 파일 선택
    bool OpenObjFileDialog(HWND hwnd, std::string& filePath);
    bool OpenGlbFileDialog(HWND hwnd, std::string& filePath);
    bool OpenTextureFileDialog(HWND hwnd, std::string& filePath);

    // ImGui UI 렌더링 함수들
    void RenderModelProperties(int modelIndex);
    void RenderObjMaterialProperties(std::shared_ptr<Model> model);
    void RenderGlbMaterialProperties(std::shared_ptr<GltfLoader> model);
    void RenderLoadingProgress();

    // 모델 타입 결정 함수
    ModelType GetModelTypeFromExtension(const std::string& filePath);

    std::shared_ptr<RoomModel> roomModel = nullptr;
    // 카메라
    Camera camera;

    float lastFrameTime = 0.0f;

    std::shared_ptr<DummyCharacter> dummyCharacter;
    bool isFirstPersonMode;

    // 모델 정보 구조체
    struct ModelInfo {
        std::shared_ptr<BaseModel> model;
        ModelType type;
        std::string name;
        std::string path;
    };

    // 모델 로딩 요청 구조체
    struct LoadRequest {
        std::string filePath;
        ModelType type;
        std::shared_ptr<LoadingProgress> progress;
        std::shared_ptr<BaseModel> model;
        std::future<bool> future;
    };

    // 비동기 로딩 스레드 함수들
    static bool LoadObjModelThreadFunction(
        const std::string& path, ID3D11Device* device,
        std::shared_ptr<Model> model,
        std::shared_ptr<LoadingProgress> progress);

    static bool LoadGlbModelThreadFunction(
        const std::string& path, ID3D11Device* device,
        std::shared_ptr<GltfLoader> model,
        std::shared_ptr<LoadingProgress> progress);

    // 모델 컬렉션
    std::vector<ModelInfo> models;

    // 현재 선택된 모델 인덱스
    int selectedModelIndex = -1;

    // 현재 선택된 재질 이름
    std::string selectedMaterialName;

    // 디바이스 참조
    ID3D11Device* device = nullptr;

    // 로딩 요청 목록
    std::vector<LoadRequest> loadRequests;
    std::mutex loadRequestsMutex;

    // 로딩 진행 상태 목록
    std::vector<std::shared_ptr<LoadingProgress>> loadingProgresses;
    std::mutex progressMutex;

    // 로딩 완료 대기 시간 (밀리초)
    const int loadingCompleteDisplayTime = 3000;

    // *** 새로 추가된 변수들 ***
    // 모델 드래그 관련 변수
    bool isDragging = false;
    int draggedModelIndex = -1;
    XMFLOAT3 dragStartModelPos;
    XMFLOAT3 dragStartIntersectPos;
    XMFLOAT3 currentIntersectPos;
    XMFLOAT3 dragPlaneNormal; // 드래그 이동 평면의 법선
    float dragPlaneD; // 드래그 평면의 D 값 (평면 방정식: ax + by + cz + d = 0)
    DragAxis dragAxis = FREE;

    // 조명 관리자 추가
    std::shared_ptr<LightManager> lightManager;

};
