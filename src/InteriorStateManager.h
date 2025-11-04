#pragma once
#include "Camera.h"
#include "InteriorState.h"
#include "LightManager.h"
#include "ModelManager.h"
#include "RoomModel.h"
#include <memory>
#include <nlohmann/json.hpp>

class InteriorStateManager
{
public:
    InteriorStateManager();
    ~InteriorStateManager();

    // 현재 상태를 파일로 저장
    bool SaveStateToFile(const std::string &filename,
                         ModelManager *modelManager,
                         std::shared_ptr<RoomModel> roomModel,
                         Camera *camera,
                         LightManager *lightManager,
                         const std::string &description = "");

    // 파일에서 상태를 로드
    bool LoadStateFromFile(const std::string &filename,
                           ModelManager *modelManager,
                           std::shared_ptr<RoomModel> roomModel,
                           Camera *camera,
                           LightManager *lightManager,
                           ID3D11Device *device);

    // 현재 상태를 구조체로 변환
    InteriorDesign::InteriorState CaptureCurrentState(
        ModelManager *modelManager,
        std::shared_ptr<RoomModel> roomModel,
        Camera *camera,
        LightManager *lightManager);

    // 구조체에서 상태를 복원
    bool RestoreState(const InteriorDesign::InteriorState &state,
                      ModelManager *modelManager,
                      std::shared_ptr<RoomModel> roomModel,
                      Camera *camera,
                      LightManager *lightManager,
                      ID3D11Device *device);

    // 최근 저장된 파일 목록 관리
    void AddRecentFile(const std::string &filename);
    std::vector<std::string> GetRecentFiles() const;
    void ClearRecentFiles();

private:
    // JSON 변환 헬퍼 함수들
    std::string StateToJson(const InteriorDesign::InteriorState &state);
    InteriorDesign::InteriorState JsonToState(const std::string &json);

    // 개별 컴포넌트 변환 함수들
    nlohmann::json RoomInfoToJson(const InteriorDesign::SavedRoomInfo &room);
    InteriorDesign::SavedRoomInfo JsonToRoomInfo(const nlohmann::json &json);

    nlohmann::json ModelInfoToJson(const InteriorDesign::SavedModelInfo &model);
    InteriorDesign::SavedModelInfo JsonToModelInfo(const nlohmann::json &json);

    nlohmann::json CameraInfoToJson(const InteriorDesign::SavedCameraInfo &camera);
    InteriorDesign::SavedCameraInfo JsonToCameraInfo(const nlohmann::json &json);

    nlohmann::json LightInfoToJson(const InteriorDesign::SavedLightInfo &light);
    InteriorDesign::SavedLightInfo JsonToLightInfo(const nlohmann::json &json);

    // 헬퍼 함수들
    std::string GetCurrentTimeString();
    bool FileExists(const std::string &filename);

    std::vector<std::string> recentFiles;
    static const size_t MAX_RECENT_FILES = 10;
};