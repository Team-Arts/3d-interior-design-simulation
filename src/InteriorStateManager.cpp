#include "InteriorStateManager.h"
#include "tiny_gltf.h" // JSON을 위해 이미 포함된 라이브러리 사용
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

#define _CRT_SECURE_NO_WARNINGS

// tiny_gltf에서 제공하는 JSON 사용
using json = nlohmann::json;

InteriorStateManager::InteriorStateManager()
{
}

InteriorStateManager::~InteriorStateManager()
{
}

bool InteriorStateManager::SaveStateToFile(const std::string &filename,
                                           ModelManager *modelManager,
                                           std::shared_ptr<RoomModel> roomModel,
                                           Camera *camera,
                                           LightManager *lightManager,
                                           const std::string &description)
{
    if (!modelManager || !roomModel || !camera || !lightManager)
    {
        return false;
    }

    try
    {
        // 현재 상태 캡처
        auto state = CaptureCurrentState(modelManager, roomModel, camera, lightManager);
        state.description = description;
        state.createdAt = GetCurrentTimeString();

        // JSON으로 변환
        std::string jsonString = StateToJson(state);

        // 파일에 저장
        std::ofstream file(filename);
        if (!file.is_open())
        {
            return false;
        }

        file << jsonString;
        file.close();

        // 최근 파일 목록에 추가
        AddRecentFile(filename);

        return true;
    }
    catch (const std::exception &e)
    {
        OutputDebugStringA(("인테리어 상태 저장 실패: " + std::string(e.what()) + "\n").c_str());
        return false;
    }
}

bool InteriorStateManager::LoadStateFromFile(const std::string &filename,
                                             ModelManager *modelManager,
                                             std::shared_ptr<RoomModel> roomModel,
                                             Camera *camera,
                                             LightManager *lightManager,
                                             ID3D11Device *device)
{
    if (!modelManager || !roomModel || !camera || !lightManager || !device)
    {
        return false;
    }

    try
    {
        // 파일 존재 확인
        if (!FileExists(filename))
        {
            return false;
        }

        // 파일 읽기
        std::ifstream file(filename);
        if (!file.is_open())
        {
            return false;
        }

        std::string jsonString((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
        file.close();

        // JSON에서 상태 복원
        auto state = JsonToState(jsonString);

        // 상태 적용
        bool success = RestoreState(state, modelManager, roomModel, camera, lightManager, device);

        if (success)
        {
            // 최근 파일 목록에 추가
            AddRecentFile(filename);
        }

        return success;
    }
    catch (const std::exception &e)
    {
        OutputDebugStringA(("인테리어 상태 로드 실패: " + std::string(e.what()) + "\n").c_str());
        return false;
    }
}

InteriorDesign::InteriorState InteriorStateManager::CaptureCurrentState(
    ModelManager *modelManager,
    std::shared_ptr<RoomModel> roomModel,
    Camera *camera,
    LightManager *lightManager)
{

    InteriorDesign::InteriorState state;

    // 방 정보 캡처
    state.room.width = roomModel->GetRoomWidth();
    state.room.height = roomModel->GetRoomHeight();
    state.room.depth = roomModel->GetRoomDepth();
    state.room.floorColor = roomModel->GetFloorColor();
    state.room.ceilingColor = roomModel->GetCeilingColor();
    state.room.wallColor = roomModel->GetWallColor();
    state.room.windowColor = roomModel->GetWindowColor();
    state.room.hasWindow = roomModel->GetHasWindow();

    // 카메라 정보 캡처
    state.camera.position = camera->GetPosition();
    state.camera.rotation = camera->GetRotation();
    state.camera.fieldOfView = camera->GetFieldOfView();
    state.camera.nearPlane = camera->GetNearPlane();
    state.camera.farPlane = camera->GetFarPlane();
    state.camera.mode = static_cast<int>(camera->GetCameraMode());

    // 모델 정보 캡처 (ModelManager에 접근자 메서드가 필요함)
    for (size_t i = 0; i < modelManager->GetModels().size(); ++i)
    {
        auto model = modelManager->GetModels()[i];
        InteriorDesign::SavedModelInfo modelInfo;
        modelInfo.name = model.name;
        modelInfo.filePath = model.path;
        modelInfo.type = (model.type == ModelType::MODEL_GLB) ? "GLB" : "OBJ";
        modelInfo.position = model.model->GetPosition();
        modelInfo.rotation = model.model->GetRotation();
        modelInfo.scale = model.model->GetScale();
        modelInfo.visible = model.model->IsVisible();
        state.models.push_back(modelInfo);
    }
    
    // 조명 정보 캡처 (LightManager에 접근자 메서드가 필요함)
    // 이 부분도 LightManager에 GetLights() 같은 메서드가 필요합니다.
    state.lights.clear();

    return state;
}

bool InteriorStateManager::RestoreState(const InteriorDesign::InteriorState &state,
                                        ModelManager *modelManager,
                                        std::shared_ptr<RoomModel> roomModel,
                                        Camera *camera,
                                        LightManager *lightManager,
                                        ID3D11Device *device)
{
    try
    {
        // 방 상태 복원
        roomModel->SetRoomWidth(state.room.width);
        roomModel->SetRoomHeight(state.room.height);
        roomModel->SetRoomDepth(state.room.depth);
        roomModel->SetFloorColor(state.room.floorColor);
        roomModel->SetCeilingColor(state.room.ceilingColor);
        roomModel->SetWallColor(state.room.wallColor);
        roomModel->SetWindowColor(state.room.windowColor);
        roomModel->SetHasWindow(state.room.hasWindow);

        // 카메라 상태 복원
        camera->SetPosition(state.camera.position.x, state.camera.position.y, state.camera.position.z);
        camera->SetRotation(state.camera.rotation.x, state.camera.rotation.y, state.camera.rotation.z);
        camera->SetFieldOfView(state.camera.fieldOfView);
        camera->SetNearPlane(state.camera.nearPlane);
        camera->SetFarPlane(state.camera.farPlane);
        camera->SetCameraMode(static_cast<CameraMode>(state.camera.mode));

        // 기존 모델들 제거
        modelManager->ClearModels();

        // 모델들 복원
        // for (const auto &modelInfo : state.models)
        for (size_t i = 0; i < state.models.size(); ++i)
        {
            // 모델 로드 및 위치/회전/스케일 설정
            // 이 부분은 ModelManager의 구현에 따라 달라집니다
            if (state.models[i].type == "OBJ")
            {
                modelManager->AddObjModel(state.models[i].filePath, device);
            }
            else if (state.models[i].type == "GLB")
            {
                modelManager->AddGlbModel(state.models[i].filePath, device);
            }

            // 로드된 모델의 속성 설정하여 인테리어 상태 복원
            // 이 부분도 ModelManager의 API에 따라 구현 필요
            modelManager->SetModels(static_cast<int>(i),
                                    state.models[i].position,
                                    state.models[i].rotation,
                                    state.models[i].scale,
                                    state.models[i].visible);
        }

        // 조명들 복원 (LightManager에 ClearLights(), AddLight() 등의 메서드가 필요함)
        for (const auto &lightInfo : state.lights)
        {
            // 조명 추가 및 설정
        }

        return true;
    }
    catch (const std::exception &e)
    {
        OutputDebugStringA(("상태 복원 실패: " + std::string(e.what()) + "\n").c_str());
        return false;
    }
}

std::string InteriorStateManager::StateToJson(const InteriorDesign::InteriorState &state)
{
    json j;

    j["version"] = state.version;
    j["createdAt"] = state.createdAt;
    j["description"] = state.description;

    // 방 정보
    j["room"] = RoomInfoToJson(state.room);

    // 카메라 정보
    j["camera"] = CameraInfoToJson(state.camera);

    // 모델들
    j["models"] = json::array();
    for (const auto &model : state.models)
    {
        j["models"].push_back(ModelInfoToJson(model));
    }

    // 조명들
    j["lights"] = json::array();
    for (const auto &light : state.lights)
    {
        j["lights"].push_back(LightInfoToJson(light));
    }

    return j.dump(4); // 4칸 들여쓰기로 예쁘게 포맷
}

InteriorDesign::InteriorState InteriorStateManager::JsonToState(const std::string &jsonString)
{
    json j = json::parse(jsonString);

    InteriorDesign::InteriorState state;

    state.version = j.value("version", "1.0");
    state.createdAt = j.value("createdAt", "");
    state.description = j.value("description", "");

    // 방 정보
    if (j.contains("room"))
    {
        state.room = JsonToRoomInfo(j["room"]);
    }

    // 카메라 정보
    if (j.contains("camera"))
    {
        state.camera = JsonToCameraInfo(j["camera"]);
    }

    // 모델들
    if (j.contains("models") && j["models"].is_array())
    {
        for (const auto &modelJson : j["models"])
        {
            state.models.push_back(JsonToModelInfo(modelJson));
        }
    }

    // 조명들
    if (j.contains("lights") && j["lights"].is_array())
    {
        for (const auto &lightJson : j["lights"])
        {
            state.lights.push_back(JsonToLightInfo(lightJson));
        }
    }

    return state;
}

json InteriorStateManager::RoomInfoToJson(const InteriorDesign::SavedRoomInfo &room)
{
    json j;
    j["width"] = room.width;
    j["height"] = room.height;
    j["depth"] = room.depth;
    j["floorColor"] = {room.floorColor.x, room.floorColor.y, room.floorColor.z, room.floorColor.w};
    j["ceilingColor"] = {room.ceilingColor.x, room.ceilingColor.y, room.ceilingColor.z, room.ceilingColor.w};
    j["wallColor"] = {room.wallColor.x, room.wallColor.y, room.wallColor.z, room.wallColor.w};
    j["windowColor"] = {room.windowColor.x, room.windowColor.y, room.windowColor.z, room.windowColor.w};
    j["hasWindow"] = room.hasWindow;
    return j;
}

InteriorDesign::SavedRoomInfo InteriorStateManager::JsonToRoomInfo(const json &j)
{
    InteriorDesign::SavedRoomInfo room;
    room.width = j.value("width", 20.0f);
    room.height = j.value("height", 10.0f);
    room.depth = j.value("depth", 20.0f);

    if (j.contains("floorColor") && j["floorColor"].is_array() && j["floorColor"].size() >= 4)
    {
        room.floorColor = {j["floorColor"][0], j["floorColor"][1], j["floorColor"][2], j["floorColor"][3]};
    }
    if (j.contains("ceilingColor") && j["ceilingColor"].is_array() && j["ceilingColor"].size() >= 4)
    {
        room.ceilingColor = {j["ceilingColor"][0], j["ceilingColor"][1], j["ceilingColor"][2], j["ceilingColor"][3]};
    }
    if (j.contains("wallColor") && j["wallColor"].is_array() && j["wallColor"].size() >= 4)
    {
        room.wallColor = {j["wallColor"][0], j["wallColor"][1], j["wallColor"][2], j["wallColor"][3]};
    }
    if (j.contains("windowColor") && j["windowColor"].is_array() && j["windowColor"].size() >= 4)
    {
        room.windowColor = {j["windowColor"][0], j["windowColor"][1], j["windowColor"][2], j["windowColor"][3]};
    }

    room.hasWindow = j.value("hasWindow", false);
    return room;
}

json InteriorStateManager::ModelInfoToJson(const InteriorDesign::SavedModelInfo &model)
{
    json j;
    j["name"] = model.name;
    j["filePath"] = model.filePath;
    j["type"] = model.type;
    j["position"] = {model.position.x, model.position.y, model.position.z};
    j["rotation"] = {model.rotation.x, model.rotation.y, model.rotation.z};
    j["scale"] = {model.scale.x, model.scale.y, model.scale.z};
    j["visible"] = model.visible;
    return j;
}

InteriorDesign::SavedModelInfo InteriorStateManager::JsonToModelInfo(const json &j)
{
    InteriorDesign::SavedModelInfo model;
    model.name = j.value("name", "");
    model.filePath = j.value("filePath", "");
    model.type = j.value("type", "OBJ");

    if (j.contains("position") && j["position"].is_array() && j["position"].size() >= 3)
    {
        model.position = {j["position"][0], j["position"][1], j["position"][2]};
    }
    if (j.contains("rotation") && j["rotation"].is_array() && j["rotation"].size() >= 3)
    {
        model.rotation = {j["rotation"][0], j["rotation"][1], j["rotation"][2]};
    }
    if (j.contains("scale") && j["scale"].is_array() && j["scale"].size() >= 3)
    {
        model.scale = {j["scale"][0], j["scale"][1], j["scale"][2]};
    }

    model.visible = j.value("visible", true);
    return model;
}

json InteriorStateManager::CameraInfoToJson(const InteriorDesign::SavedCameraInfo &camera)
{
    json j;
    j["position"] = {camera.position.x, camera.position.y, camera.position.z};
    j["rotation"] = {camera.rotation.x, camera.rotation.y, camera.rotation.z};
    j["fieldOfView"] = camera.fieldOfView;
    j["nearPlane"] = camera.nearPlane;
    j["farPlane"] = camera.farPlane;
    j["mode"] = camera.mode;
    return j;
}

InteriorDesign::SavedCameraInfo InteriorStateManager::JsonToCameraInfo(const json &j)
{
    InteriorDesign::SavedCameraInfo camera;

    if (j.contains("position") && j["position"].is_array() && j["position"].size() >= 3)
    {
        camera.position = {j["position"][0], j["position"][1], j["position"][2]};
    }
    if (j.contains("rotation") && j["rotation"].is_array() && j["rotation"].size() >= 3)
    {
        camera.rotation = {j["rotation"][0], j["rotation"][1], j["rotation"][2]};
    }

    camera.fieldOfView = j.value("fieldOfView", XM_PIDIV4);
    camera.nearPlane = j.value("nearPlane", 0.1f);
    camera.farPlane = j.value("farPlane", 1000.0f);
    camera.mode = j.value("mode", 0);
    return camera;
}

json InteriorStateManager::LightInfoToJson(const InteriorDesign::SavedLightInfo &light)
{
    json j;
    j["type"] = light.type;
    j["position"] = {light.position.x, light.position.y, light.position.z};
    j["direction"] = {light.direction.x, light.direction.y, light.direction.z};
    j["color"] = {light.color.x, light.color.y, light.color.z};
    j["intensity"] = light.intensity;
    j["range"] = light.range;
    j["attenuation"] = light.attenuation;
    j["innerCone"] = light.innerCone;
    j["outerCone"] = light.outerCone;
    return j;
}

InteriorDesign::SavedLightInfo InteriorStateManager::JsonToLightInfo(const json &j)
{
    InteriorDesign::SavedLightInfo light;
    light.type = j.value("type", 1); // 기본값: Point Light

    if (j.contains("position") && j["position"].is_array() && j["position"].size() >= 3)
    {
        light.position = {j["position"][0], j["position"][1], j["position"][2]};
    }
    if (j.contains("direction") && j["direction"].is_array() && j["direction"].size() >= 3)
    {
        light.direction = {j["direction"][0], j["direction"][1], j["direction"][2]};
    }
    if (j.contains("color") && j["color"].is_array() && j["color"].size() >= 3)
    {
        light.color = {j["color"][0], j["color"][1], j["color"][2]};
    }

    light.intensity = j.value("intensity", 1.0f);
    light.range = j.value("range", 50.0f);
    light.attenuation = j.value("attenuation", 0.1f);
    light.innerCone = j.value("innerCone", XM_PIDIV4);
    light.outerCone = j.value("outerCone", XM_PIDIV2);
    return light;
}

std::string InteriorStateManager::GetCurrentTimeString()
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
#ifdef _WIN32
    struct tm timeinfo;
    if (localtime_s(&timeinfo, &time_t) == 0) {
        ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
    } else {
        ss << "Unknown time";
    }
#else
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
#endif
    return ss.str();
}

bool InteriorStateManager::FileExists(const std::string &filename)
{
    std::ifstream file(filename);
    return file.good();
}

void InteriorStateManager::AddRecentFile(const std::string &filename)
{
    // 이미 존재하는 파일이면 제거
    auto it = std::find(recentFiles.begin(), recentFiles.end(), filename);
    if (it != recentFiles.end())
    {
        recentFiles.erase(it);
    }

    // 맨 앞에 추가
    recentFiles.insert(recentFiles.begin(), filename);

    // 최대 개수 제한
    if (recentFiles.size() > MAX_RECENT_FILES)
    {
        recentFiles.resize(MAX_RECENT_FILES);
    }
}

std::vector<std::string> InteriorStateManager::GetRecentFiles() const
{
    return recentFiles;
}

void InteriorStateManager::ClearRecentFiles()
{
    recentFiles.clear();
}