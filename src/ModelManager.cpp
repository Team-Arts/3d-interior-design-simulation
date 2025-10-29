#include "ModelManager.h"
#include "imgui.h"
#include <Commdlg.h>
#include <shlobj.h>
#include <iostream>
#include <codecvt>
#include <chrono>
#include <algorithm>

ModelManager::ModelManager() : device(nullptr)
{
    // 카메라 초기화 - 초기 위치 설정  
    camera.SetPosition(0.0f, 0.0f, -20.0f);
    camera.SetRotation(0.0f, 0.0f, 0.0f);
    camera.SetProjection(XM_PIDIV4, 1280.0f / 800.0f, 0.1f, 1000.0f);

    // 조명 관리자는 디바이스가 있을 때 초기화됨
    lightManager = std::make_shared<LightManager>();
}

// 조명 관리자 초기화 함수 추가
void ModelManager::InitLightManager(ID3D11Device* device)
{
    if (lightManager && device) {
        lightManager->Initialize(device);
    }
}

ModelManager::~ModelManager()
{
    Release();
}

ModelType ModelManager::GetModelTypeFromExtension(const std::string& filePath)
{
    // 파일 경로에서 확장자 추출
    size_t lastDot = filePath.find_last_of(".");
    if (lastDot == std::string::npos) {
        return MODEL_OBJ; // 기본값으로 OBJ 사용
    }

    std::string extension = filePath.substr(lastDot + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    if (extension == "glb" || extension == "gltf") {
        return MODEL_GLB;
    }

    return MODEL_OBJ; // 기본값으로 OBJ 사용
}

void ModelManager::AddModel(const std::string& path, ID3D11Device* device)
{
    ModelType type = GetModelTypeFromExtension(path);

    if (type == MODEL_GLB) {
        AddGlbModel(path, device);
    }
    else {
        AddObjModel(path, device);
    }
}

void ModelManager::AddObjModel(const std::string& path, ID3D11Device* device)
{
    auto wrapper = std::make_shared<ObjModelWrapper>();
    if (wrapper->model->LoadObjModel(path, device)) {
        ModelInfo info;
        info.model = wrapper;
        info.type = MODEL_OBJ;
        info.name = wrapper->model->GetModelInfo().Name;
        info.path = path;

        models.push_back(info);
        selectedModelIndex = static_cast<int>(models.size()) - 1; // 새로 추가된 모델 선택

        // 첫 번째 재질을 선택
        const auto& materials = wrapper->model->GetMaterials();
        if (!materials.empty()) {
            selectedMaterialName = materials.begin()->first;
        }
    }
    else {
        OutputDebugStringA(("Failed to load OBJ model: " + path + "\n").c_str());
    }
}

void ModelManager::AddGlbModel(const std::string& path, ID3D11Device* device)
{
    auto wrapper = std::make_shared<GlbModelWrapper>();
    if (wrapper->model->LoadGlbModel(path, device)) {
        ModelInfo info;
        info.model = wrapper;
        info.type = MODEL_GLB;
        info.name = wrapper->model->GetModelInfo().Name;
        info.path = path;

        models.push_back(info);
        selectedModelIndex = static_cast<int>(models.size()) - 1; // 새로 추가된 모델 선택

        // 첫 번째 재질을 선택
        const auto& materials = wrapper->model->GetMaterials();
        if (!materials.empty()) {
            selectedMaterialName = materials.begin()->first;
        }
    }
    else {
        OutputDebugStringA(("Failed to load GLB model: " + path + "\n").c_str());
    }
}

void ModelManager::AddModelAsync(const std::string& path)
{
    ModelType type = GetModelTypeFromExtension(path);

    if (type == MODEL_GLB) {
        AddGlbModelAsync(path);
    }
    else {
        AddObjModelAsync(path);
    }
}

void ModelManager::AddObjModelAsync(const std::string& path)
{
    if (!device) {
        OutputDebugStringA("Device is null, cannot load model asynchronously\n");
        return;
    }

    auto progress = std::make_shared<LoadingProgress>();
    progress->filename = path.substr(path.find_last_of("/\\") + 1);
    progress->progress = 0.0f;
    progress->isComplete = false;
    progress->isError = false;

    {
        std::lock_guard<std::mutex> lock(progressMutex);
        loadingProgresses.push_back(progress);
    }

    LoadRequest request;
    request.filePath = path;
    request.type = MODEL_OBJ;
    request.progress = progress;

    auto objWrapper = std::make_shared<ObjModelWrapper>();
    request.model = objWrapper;

    // 스레드 풀에서 작업을 실행하고 future를 받아옴
    request.future = std::async(std::launch::async,
        &ModelManager::LoadObjModelThreadFunction,
        path, device, objWrapper->model, progress);

    {
        std::lock_guard<std::mutex> lock(loadRequestsMutex);
        loadRequests.push_back(std::move(request));
    }
}

void ModelManager::AddGlbModelAsync(const std::string& path)
{
    if (!device) {
        OutputDebugStringA("Device is null, cannot load model asynchronously\n");
        return;
    }

    auto progress = std::make_shared<LoadingProgress>();
    progress->filename = path.substr(path.find_last_of("/\\") + 1);
    progress->progress = 0.0f;
    progress->isComplete = false;
    progress->isError = false;

    {
        std::lock_guard<std::mutex> lock(progressMutex);
        loadingProgresses.push_back(progress);
    }

    LoadRequest request;
    request.filePath = path;
    request.type = MODEL_GLB;
    request.progress = progress;

    auto glbWrapper = std::make_shared<GlbModelWrapper>();
    request.model = glbWrapper;

    // 스레드 풀에서 작업을 실행하고 future를 받아옴
    request.future = std::async(std::launch::async,
        &ModelManager::LoadGlbModelThreadFunction,
        path, device, glbWrapper->model, progress);

    {
        std::lock_guard<std::mutex> lock(loadRequestsMutex);
        loadRequests.push_back(std::move(request));
    }
}

bool ModelManager::LoadObjModelThreadFunction(const std::string& path, ID3D11Device* device,
    std::shared_ptr<Model> model,
    std::shared_ptr<LoadingProgress> progress)
{
    try {
        // 실제 로딩 진행
        progress->progress = 0.1f; // 시작

        bool result = model->LoadObjModel(path, device);

        if (result) {
            progress->progress = 1.0f;
            progress->isComplete = true;

            // 시간 기록 (완료된 시간 표시용)
            progress->errorMessage = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());

            return true;
        }
        else {
            progress->isError = true;
            progress->errorMessage = "Failed to load OBJ model: " + path;
            return false;
        }
    }
    catch (const std::exception& e) {
        progress->isError = true;
        progress->errorMessage = "Exception: " + std::string(e.what());
        return false;
    }
    catch (...) {
        progress->isError = true;
        progress->errorMessage = "Unknown error occurred while loading model";
        return false;
    }
}

bool ModelManager::LoadGlbModelThreadFunction(const std::string& path, ID3D11Device* device,
    std::shared_ptr<GltfLoader> model,
    std::shared_ptr<LoadingProgress> progress)
{
    try {
        // 실제 로딩 진행
        progress->progress = 0.1f; // 시작

        bool result = model->LoadGlbModel(path, device);

        if (result) {
            progress->progress = 1.0f;
            progress->isComplete = true;

            // 시간 기록 (완료된 시간 표시용)
            progress->errorMessage = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());

            return true;
        }
        else {
            progress->isError = true;
            progress->errorMessage = "Failed to load GLB model: " + path;
            return false;
        }
    }
    catch (const std::exception& e) {
        progress->isError = true;
        progress->errorMessage = "Exception: " + std::string(e.what());
        return false;
    }
    catch (...) {
        progress->isError = true;
        progress->errorMessage = "Unknown error occurred while loading model";
        return false;
    }
}

void ModelManager::UpdateLoadingStatus()
{
    std::lock_guard<std::mutex> lock(loadRequestsMutex);

    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // 완료된 로딩 요청 처리
    for (auto it = loadRequests.begin(); it != loadRequests.end(); ) {
        auto& request = *it;

        // future가 ready 상태인지 검사 (차단 없이)
        if (request.future.valid() &&
            request.future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {

            bool result = false;
            try {
                result = request.future.get(); // 결과 가져오기
            }
            catch (...) {
                // 예외 처리
                request.progress->isError = true;
                request.progress->errorMessage = "Exception during model loading";
            }

            if (result) {
                // 성공적으로 로드된 모델 추가
                ModelInfo info;
                info.model = request.model;
                info.type = request.type;

                if (request.type == MODEL_OBJ) {
                    auto objWrapper = std::static_pointer_cast<ObjModelWrapper>(request.model);
                    info.name = objWrapper->model->GetModelInfo().Name;
                    info.path = request.filePath;
                }
                else {
                    auto glbWrapper = std::static_pointer_cast<GlbModelWrapper>(request.model);
                    info.name = glbWrapper->model->GetModelInfo().Name;
                    info.path = request.filePath;
                }

                models.push_back(info);
                selectedModelIndex = static_cast<int>(models.size()) - 1;

                // 첫 번째 재질 선택
                if (request.type == MODEL_OBJ) {
                    auto objWrapper = std::static_pointer_cast<ObjModelWrapper>(request.model);
                    const auto& materials = objWrapper->model->GetMaterials();
                    if (!materials.empty()) {
                        selectedMaterialName = materials.begin()->first;
                    }
                }
                else {
                    auto glbWrapper = std::static_pointer_cast<GlbModelWrapper>(request.model);
                    const auto& materials = glbWrapper->model->GetMaterials();
                    if (!materials.empty()) {
                        selectedMaterialName = materials.begin()->first;
                    }
                }
            }

            // 처리 완료된 요청 제거
            it = loadRequests.erase(it);
        }
        else {
            ++it;
        }
    }

    // 오래된 완료 진행 상태 제거
    {
        std::lock_guard<std::mutex> progressLock(progressMutex);

        for (auto it = loadingProgresses.begin(); it != loadingProgresses.end(); ) {
            auto& progress = *it;

            if (progress->isComplete || progress->isError) {
                // 완료 시간이 저장된 경우 (밀리초 타임스탬프)
                if (!progress->errorMessage.empty() && progress->isComplete) {
                    try {
                        long long completeTime = std::stoll(progress->errorMessage);
                        if (now - completeTime > loadingCompleteDisplayTime) {
                            it = loadingProgresses.erase(it);
                            continue;
                        }
                    }
                    catch (...) {
                        // 시간 파싱 실패하면 그냥 유지
                    }
                }
            }
            ++it;
        }
    }
}

void ModelManager::RemoveModel(int index)
{
    if (index >= 0 && index < models.size()) {
        models.erase(models.begin() + index);

        // 선택된 모델 인덱스 업데이트
        if (selectedModelIndex == index) {
            selectedModelIndex = models.empty() ? -1 : 0;
            selectedMaterialName = "";
        }
        else if (selectedModelIndex > index) {
            selectedModelIndex--;
        }
    }
}

void ModelManager::RenderModels(ID3D11DeviceContext* deviceContext)
{
    // 먼저 방 렌더링
    if (roomModel) {
        // 방 모델에도 조명 관리자 전달
        roomModel->Render(deviceContext, camera, lightManager.get());
    }

    // (1인칭 모드에서는 자신을 보지 못함)
    if (dummyCharacter && !isFirstPersonMode) {
        dummyCharacter->Render(deviceContext, camera);
    }

    // 그 다음 모델 렌더링
    for (const auto& modelInfo : models) {
        // 모델 타입에 따라 조명 관리자 전달
        if (modelInfo.type == MODEL_OBJ) {
            auto objWrapper = std::static_pointer_cast<ObjModelWrapper>(modelInfo.model);
            objWrapper->model->Render(deviceContext, camera, lightManager.get());
        }
        else if (modelInfo.type == MODEL_GLB) {
            // GLB 모델은 일반 렌더링 (향후 조명 지원 확장 가능)
            modelInfo.model->Render(deviceContext, camera, lightManager.get());
        }
    }
}

// 프레임 처리 함수
void ModelManager::ProcessFrame(HWND hwnd, float deltaTime)
{
    // 현재 카메라 모드에 따라 다른 처리
    if (isFirstPersonMode) {
        // 1인칭 모드: 캐릭터 입력 처리
        ProcessCharacterInput(hwnd, deltaTime);
        // 카메라 위치/회전 업데이트
        camera.UpdateFirstPersonView();
    }
    else {
        // 자유 시점 모드: 기존 카메라 입력 처리
        camera.ProcessInput(hwnd, deltaTime);
    }

    //// 카메라 입력 처리
    //camera.ProcessInput(hwnd, deltaTime);

    // GLB 모델의 애니메이션 업데이트
    for (auto& modelInfo : models) {
        if (modelInfo.type == MODEL_GLB) {
            auto glbWrapper = std::static_pointer_cast<GlbModelWrapper>(modelInfo.model);
            glbWrapper->model->UpdateAnimation(deltaTime);
        }
    }
}

void ModelManager::ProcessCharacterInput(HWND hwnd, float deltaTime) {
    // 키보드 입력 처리
    bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    float moveSpeed = shiftPressed ? 5.0f : 3.0f; // 이동 속도 (m/s)
    moveSpeed *= deltaTime; // 시간 보정

    // 충돌 처리를 위한 방 크기 정보
    float roomWidth = 20.0f;
    float roomHeight = 10.0f;
    float roomDepth = 20.0f;

    if (roomModel) {
        roomWidth = roomModel->GetRoomWidth();
        roomHeight = roomModel->GetRoomHeight();
        roomDepth = roomModel->GetRoomDepth();
    }

    // 캐릭터 이동
    XMFLOAT3 newPosition = dummyCharacter->GetPosition();

    if (GetAsyncKeyState('W') & 0x8000) {
        // 시험해보고 문제 없으면 이동
        XMFLOAT3 testPos = newPosition;
        testPos.x += moveSpeed * sinf(XMConvertToRadians(dummyCharacter->GetRotation()));
        testPos.z += moveSpeed * cosf(XMConvertToRadians(dummyCharacter->GetRotation()));

        if (!dummyCharacter->CheckCollision(testPos, roomWidth, roomHeight, roomDepth)) {
            dummyCharacter->MoveForward(moveSpeed);
            newPosition = dummyCharacter->GetPosition();
        }
    }

    if (GetAsyncKeyState('S') & 0x8000) {
        // 뒤로 이동
        XMFLOAT3 testPos = newPosition;
        testPos.x -= moveSpeed * sinf(XMConvertToRadians(dummyCharacter->GetRotation()));
        testPos.z -= moveSpeed * cosf(XMConvertToRadians(dummyCharacter->GetRotation()));

        if (!dummyCharacter->CheckCollision(testPos, roomWidth, roomHeight, roomDepth)) {
            dummyCharacter->MoveForward(-moveSpeed);
            newPosition = dummyCharacter->GetPosition();
        }
    }

    if (GetAsyncKeyState('A') & 0x8000) {
        // 왼쪽으로 이동
        XMFLOAT3 testPos = newPosition;
        testPos.x -= moveSpeed * cosf(XMConvertToRadians(dummyCharacter->GetRotation()));
        testPos.z += moveSpeed * sinf(XMConvertToRadians(dummyCharacter->GetRotation()));

        if (!dummyCharacter->CheckCollision(testPos, roomWidth, roomHeight, roomDepth)) {
            dummyCharacter->MoveRight(-moveSpeed);
            newPosition = dummyCharacter->GetPosition();
        }
    }

    if (GetAsyncKeyState('D') & 0x8000) {
        // 오른쪽으로 이동
        XMFLOAT3 testPos = newPosition;
        testPos.x += moveSpeed * cosf(XMConvertToRadians(dummyCharacter->GetRotation()));
        testPos.z -= moveSpeed * sinf(XMConvertToRadians(dummyCharacter->GetRotation()));

        if (!dummyCharacter->CheckCollision(testPos, roomWidth, roomHeight, roomDepth)) {
            dummyCharacter->MoveRight(moveSpeed);
            newPosition = dummyCharacter->GetPosition();
        }
    }

    // 마우스로 시선 방향 회전 (수정)
    static bool mouseTracking = false;
    static POINT lastMousePos = { 0, 0 };

    // 현재 마우스 우클릭 상태 확인
    bool rightButtonDown = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

    if (rightButtonDown) {
        POINT currentMousePos;
        GetCursorPos(&currentMousePos);
        ScreenToClient(hwnd, &currentMousePos);

        if (!mouseTracking) {
            // 우클릭을 처음 눌렀을 때, 단순히 마우스 위치만 기록하고 회전은 하지 않음
            lastMousePos = currentMousePos;
            mouseTracking = true;
        }
        else {
            // 우클릭을 누른 상태에서 마우스 이동 시에만 회전 처리
            float yawDelta = (currentMousePos.x - lastMousePos.x) * mouseSensitivity;
            float pitchDelta = (lastMousePos.y - currentMousePos.y) * mouseSensitivity;

            // 현재 회전 값 가져오기
            float newRotation = dummyCharacter->GetRotation() + yawDelta;
            float newPitch = dummyCharacter->GetPitch() + pitchDelta;

            // 회전 값 설정
            dummyCharacter->SetRotation(newRotation);
            dummyCharacter->SetPitch(newPitch);

            // 마우스 위치 업데이트
            lastMousePos = currentMousePos;
        }
    }
    else {
        // 마우스 버튼을 떼면 추적 중단
        mouseTracking = false;
    }
}


bool ModelManager::OpenTextureFileDialog(HWND hwnd, std::string& filePath)
{
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "텍스처 파일\0*.jpg;*.png;*.bmp;*.dds\0모든 파일\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn) == TRUE)
    {
        filePath = ofn.lpstrFile;
        return true;
    }
    return false;
}

//void ModelManager::RenderLoadingProgress()
//{
//    std::lock_guard<std::mutex> lock(progressMutex);
//
//    if (loadingProgresses.empty()) {
//        return;
//    }
//
//    ImGui::SetNextWindowSize(ImVec2(400, 150), ImGuiCond_FirstUseEver);
//    if (ImGui::Begin("모델 로딩 상태", nullptr)) {
//        for (const auto& progress : loadingProgresses) {
//            std::string label = progress->filename;
//
//            if (progress->isComplete) {
//                ImGui::TextColored(ImVec4(0, 1, 0, 1), "✓ %s - 로딩 완료", label.c_str());
//                continue;
//            }
//
//            if (progress->isError) {
//                ImGui::TextColored(ImVec4(1, 0, 0, 1), "✗ %s - 오류: %s",
//                    label.c_str(), progress->errorMessage.c_str());
//                continue;
//            }
//
//            ImGui::Text("%s", label.c_str());
//            ImGui::ProgressBar(progress->progress, ImVec2(-1, 0));
//        }
//    }
//    ImGui::End();
//}

void ModelManager::RenderObjMaterialProperties(std::shared_ptr<Model> model)
{
    if (!model) return;

    const auto& materials = model->GetMaterials();
    if (materials.empty()) return;

    ImGui::Text("재질 선택");
    if (ImGui::BeginCombo("##MaterialCombo", selectedMaterialName.c_str()))
    {
        for (const auto& material : materials)
        {
            bool isSelected = (selectedMaterialName == material.first);
            if (ImGui::Selectable(material.first.c_str(), isSelected))
            {
                selectedMaterialName = material.first;
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Separator();

    // 선택된 재질 정보 표시 및 편집
    auto it = materials.find(selectedMaterialName);
    if (it != materials.end())
    {
        // 재질을 직접 수정하기 위해 const를 제거 (주의: 원본 맵을 수정)
        auto& material = const_cast<Model::Material&>(it->second);

        ImGui::Text("재질 정보: %s", material.Name.c_str());

        // 색상 편집 위젯 추가
        ImGui::Text("색상 속성 (RGB 0.0 ~ 1.0)");

        // 앰비언트 색상 편집
        if (ImGui::ColorEdit3("주변광 색상", (float*)&material.Ambient)) {
            // 색상이 변경됨
        }

        // 디퓨즈 색상 편집
        if (ImGui::ColorEdit3("확산광 색상", (float*)&material.Diffuse)) {
            // 색상이 변경됨
        }

        // 스페큘러 색상 편집
        if (ImGui::ColorEdit3("반사광 색상", (float*)&material.Specular)) {
            // 색상이 변경됨
        }

        // 광택도 편집
        if (ImGui::SliderFloat("광택도", &material.Shininess, 1.0f, 128.0f)) {
            // 광택도가 변경됨
        }

        // 텍스처 정보
        if (!material.DiffuseMapPath.empty())
        {
            ImGui::Text("디퓨즈 텍스처: %s", material.DiffuseMapPath.c_str());
            ImGui::Text("텍스처 로드 상태: %s", material.DiffuseMap ? "성공" : "실패");

            // 텍스처 변경 버튼
            if (ImGui::Button("텍스처 변경")) {
                std::string texturePath;
                if (OpenTextureFileDialog(GetActiveWindow(), texturePath)) {
                    // 선택된 텍스처 로드
                    model->LoadTexture(texturePath, device, &material.DiffuseMap);
                    material.DiffuseMapPath = texturePath;
                }
            }
        }
        else
        {
            ImGui::Text("디퓨즈 텍스처: 없음");

            // 텍스처 추가 버튼
            if (ImGui::Button("텍스처 추가")) {
                std::string texturePath;
                if (OpenTextureFileDialog(GetActiveWindow(), texturePath)) {
                    // 선택된 텍스처 로드
                    model->LoadTexture(texturePath, device, &material.DiffuseMap);
                    material.DiffuseMapPath = texturePath;
                }
            }
        }
    }
}

void ModelManager::RenderGlbMaterialProperties(std::shared_ptr<GltfLoader> model)
{
    if (!model) return;

    const auto& materials = model->GetMaterials();
    if (materials.empty()) return;

    ImGui::Text("PBR 재질 선택");
    if (ImGui::BeginCombo("##MaterialCombo", selectedMaterialName.c_str()))
    {
        for (const auto& material : materials)
        {
            bool isSelected = (selectedMaterialName == material.first);
            if (ImGui::Selectable(material.first.c_str(), isSelected))
            {
                selectedMaterialName = material.first;
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Separator();

    // 선택된 재질 정보 표시 및 편집
    auto it = materials.find(selectedMaterialName);
    if (it != materials.end())
    {
        // 재질을 직접 수정하기 위해 const를 제거 (주의: 원본 맵을 수정)
        auto& material = const_cast<GltfLoader::PbrMaterial&>(it->second);

        ImGui::Text("재질 정보: %s", material.Name.c_str());

        // 색상 편집 위젯 추가
        ImGui::Text("PBR 재질 속성");

        // 베이스 컬러 편집
        if (ImGui::ColorEdit4("베이스 컬러", (float*)&material.BaseColorFactor)) {
            // 색상이 변경됨
        }

        // 이미시브 편집
        if (ImGui::ColorEdit3("이미시브 컬러", (float*)&material.EmissiveFactor)) {
            // 색상이 변경됨
        }

        // 메탈릭 인자 편집
        if (ImGui::SliderFloat("메탈릭 인자", &material.MetallicFactor, 0.0f, 1.0f)) {
            // 메탈릭 인자가 변경됨
        }

        // 러프니스 인자 편집
        if (ImGui::SliderFloat("러프니스 인자", &material.RoughnessFactor, 0.0f, 1.0f)) {
            // 러프니스 인자가 변경됨
        }

        // 텍스처 정보 - 베이스 컬러
        ImGui::Separator();
        ImGui::Text("텍스처 맵");

        if (!material.BaseColorTexturePath.empty())
        {
            ImGui::Text("베이스 컬러 텍스처: %s", material.BaseColorTexturePath.c_str());
            ImGui::Text("텍스처 로드 상태: %s", material.BaseColorTexture ? "성공" : "실패");
        }
        else
        {
            ImGui::Text("베이스 컬러 텍스처: 없음");
        }

        // 메탈릭 러프니스 텍스처
        if (!material.MetallicRoughnessTexturePath.empty())
        {
            ImGui::Text("메탈릭-러프니스 텍스처: %s", material.MetallicRoughnessTexturePath.c_str());
            ImGui::Text("텍스처 로드 상태: %s", material.MetallicRoughnessTexture ? "성공" : "실패");
        }
        else
        {
            ImGui::Text("메탈릭-러프니스 텍스처: 없음");
        }

        // 노멀 텍스처
        if (!material.NormalTexturePath.empty())
        {
            ImGui::Text("노멀 텍스처: %s", material.NormalTexturePath.c_str());
            ImGui::Text("텍스처 로드 상태: %s", material.NormalTexture ? "성공" : "실패");
        }
        else
        {
            ImGui::Text("노멀 텍스처: 없음");
        }

        // 이미시브 텍스처
        if (!material.EmissiveTexturePath.empty())
        {
            ImGui::Text("이미시브 텍스처: %s", material.EmissiveTexturePath.c_str());
            ImGui::Text("텍스처 로드 상태: %s", material.EmissiveTexture ? "성공" : "실패");
        }
        else
        {
            ImGui::Text("이미시브 텍스처: 없음");
        }

        // 오클루전 텍스처
        if (!material.OcclusionTexturePath.empty())
        {
            ImGui::Text("오클루전 텍스처: %s", material.OcclusionTexturePath.c_str());
            ImGui::Text("텍스처 로드 상태: %s", material.OcclusionTexture ? "성공" : "실패");
        }
        else
        {
            ImGui::Text("오클루전 텍스처: 없음");
        }

        // 참고: GLB 모델에서 텍스처 변경은 더 복잡하므로 여기서는 구현하지 않음
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "참고: GLB 모델의 텍스처는 변경할 수 없습니다.");
    }
}

//void ModelManager::RenderModelProperties(int modelIndex)
//{
//    if (modelIndex < 0 || modelIndex >= models.size())
//        return;
//
//    const auto& modelInfo = models[modelIndex];
//
//    ImGui::Text("모델 속성");
//    ImGui::Separator();
//
//    // 모델 타입에 따라 적절한 속성 편집 UI 표시
//    if (modelInfo.type == MODEL_OBJ) {
//        auto objWrapper = std::static_pointer_cast<ObjModelWrapper>(modelInfo.model);
//        auto& objModelInfo = objWrapper->model->GetModelInfo();
//
//        // 가시성
//        ImGui::Checkbox("표시", &objModelInfo.Visible);
//
//        // 위치
//        ImGui::Text("위치");
//        ImGui::SliderFloat("X##Pos", &objModelInfo.Position.x, -10.0f, 10.0f);
//        ImGui::SliderFloat("Y##Pos", &objModelInfo.Position.y, -10.0f, 10.0f);
//        ImGui::SliderFloat("Z##Pos", &objModelInfo.Position.z, -10.0f, 10.0f);
//
//        // 회전
//        ImGui::Text("회전");
//        ImGui::SliderFloat("X##Rot", &objModelInfo.Rotation.x, -180.0f, 180.0f);
//        ImGui::SliderFloat("Y##Rot", &objModelInfo.Rotation.y, -180.0f, 180.0f);
//        ImGui::SliderFloat("Z##Rot", &objModelInfo.Rotation.z, -180.0f, 180.0f);
//
//        // 크기
//        ImGui::Text("크기");
//        ImGui::SliderFloat("X##Scale", &objModelInfo.Scale.x, 0.1f, 5.0f);
//        ImGui::SliderFloat("Y##Scale", &objModelInfo.Scale.y, 0.1f, 5.0f);
//        ImGui::SliderFloat("Z##Scale", &objModelInfo.Scale.z, 0.1f, 5.0f);
//
//        // 파일 경로 표시
//        ImGui::Separator();
//        ImGui::Text("OBJ 파일 경로: %s", objModelInfo.FilePath.c_str());
//
//        if (!objModelInfo.MtlFilePath.empty()) {
//            ImGui::Text("MTL 파일 경로: %s", objModelInfo.MtlFilePath.c_str());
//        }
//    }
//    else if (modelInfo.type == MODEL_GLB) {
//        auto glbWrapper = std::static_pointer_cast<GlbModelWrapper>(modelInfo.model);
//        auto& glbModelInfo = glbWrapper->model->GetModelInfo();
//
//        // 가시성
//        ImGui::Checkbox("표시", &glbModelInfo.Visible);
//
//        // 위치
//        ImGui::Text("위치");
//        ImGui::SliderFloat("X##Pos", &glbModelInfo.Position.x, -10.0f, 10.0f);
//        ImGui::SliderFloat("Y##Pos", &glbModelInfo.Position.y, -10.0f, 10.0f);
//        ImGui::SliderFloat("Z##Pos", &glbModelInfo.Position.z, -10.0f, 10.0f);
//
//        // 회전
//        ImGui::Text("회전");
//        ImGui::SliderFloat("X##Rot", &glbModelInfo.Rotation.x, -180.0f, 180.0f);
//        ImGui::SliderFloat("Y##Rot", &glbModelInfo.Rotation.y, -180.0f, 180.0f);
//        ImGui::SliderFloat("Z##Rot", &glbModelInfo.Rotation.z, -180.0f, 180.0f);
//
//        // 크기
//        ImGui::Text("크기");
//        ImGui::SliderFloat("X##Scale", &glbModelInfo.Scale.x, 0.1f, 5.0f);
//        ImGui::SliderFloat("Y##Scale", &glbModelInfo.Scale.y, 0.1f, 5.0f);
//        ImGui::SliderFloat("Z##Scale", &glbModelInfo.Scale.z, 0.1f, 5.0f);
//
//        // 파일 경로 표시
//        ImGui::Separator();
//        ImGui::Text("GLB 파일 경로: %s", glbModelInfo.FilePath.c_str());
//    }
//}

void ModelManager::RenderImGuiWindow(HWND hwnd, ID3D11Device* device, float deltaTime)
{
    this->device = device;

    // 프레임 처리
    ProcessFrame(hwnd, deltaTime);

    // 로딩 상태 업데이트
    UpdateLoadingStatus();

    // 로딩 진행 상태 창 렌더링
    RenderLoadingProgress();

    // 방 속성 창 렌더링
    if (roomModel) {
        RenderRoomProperties();
    }

    // 카메라 설정 창 렌더링
    camera.RenderUI();

    ImGui::Begin("모델 관리자", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    // 모델 추가 버튼 - OBJ
    if (ImGui::Button("OBJ 모델 추가 (동기)"))
    {
        std::string filePath;
        if (OpenObjFileDialog(hwnd, filePath))
        {
            AddObjModel(filePath, device);
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("OBJ 모델 추가 (비동기)"))
    {
        std::string filePath;
        if (OpenObjFileDialog(hwnd, filePath))
        {
            AddObjModelAsync(filePath);
        }
    }

    // 모델 추가 버튼 - GLB
    if (ImGui::Button("GLB 모델 추가 (동기)"))
    {
        std::string filePath;
        if (OpenGlbFileDialog(hwnd, filePath))
        {
            AddGlbModel(filePath, device);
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("GLB 모델 추가 (비동기)"))
    {
        std::string filePath;
        if (OpenGlbFileDialog(hwnd, filePath))
        {
            AddGlbModelAsync(filePath);
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("선택한 모델 제거") && selectedModelIndex >= 0)
    {
        RemoveModel(selectedModelIndex);
    }

    ImGui::Separator();

    // 모델 목록
    ImGui::Text("모델 목록:");
    ImGui::BeginChild("ModelList", ImVec2(150, 200), true);
    for (int i = 0; i < models.size(); i++)
    {
        const auto& modelInfo = models[i];
        std::string displayName = modelInfo.name;

        // 모델 타입에 따라 아이콘 추가
        if (modelInfo.type == MODEL_GLB) {
            displayName = "[GLB] " + displayName;
        }
        else {
            displayName = "[OBJ] " + displayName;
        }

        if (ImGui::Selectable(displayName.c_str(), selectedModelIndex == i))
        {
            selectedModelIndex = i;

            // 첫 번째 재질 선택
            if (modelInfo.type == MODEL_OBJ) {
                auto objWrapper = std::static_pointer_cast<ObjModelWrapper>(modelInfo.model);
                const auto& materials = objWrapper->model->GetMaterials();
                if (!materials.empty()) {
                    selectedMaterialName = materials.begin()->first;
                }
                else {
                    selectedMaterialName = "";
                }
            }
            else {
                auto glbWrapper = std::static_pointer_cast<GlbModelWrapper>(modelInfo.model);
                const auto& materials = glbWrapper->model->GetMaterials();
                if (!materials.empty()) {
                    selectedMaterialName = materials.begin()->first;
                }
                else {
                    selectedMaterialName = "";
                }
            }
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // 선택된 모델 속성 편집
    ImGui::BeginGroup();

    // 모델 속성 및 재질 속성을 탭으로 구분
    ImGui::BeginChild("ModelProperties", ImVec2(400, 400), true);
    if (selectedModelIndex >= 0 && selectedModelIndex < models.size())
    {
        if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("모델"))
            {
                RenderModelProperties(selectedModelIndex);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("재질"))
            {
                if (models[selectedModelIndex].type == MODEL_OBJ) {
                    auto objWrapper = std::static_pointer_cast<ObjModelWrapper>(models[selectedModelIndex].model);
                    RenderObjMaterialProperties(objWrapper->model);
                }
                else {
                    auto glbWrapper = std::static_pointer_cast<GlbModelWrapper>(models[selectedModelIndex].model);
                    RenderGlbMaterialProperties(glbWrapper->model);
                }
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    else
    {
        ImGui::Text("모델을 선택하세요");
    }
    ImGui::EndChild();
    ImGui::EndGroup();

    ImGui::Separator();

    // 모델 정보 요약
    if (models.size() > 0)
    {
        ImGui::Text("로드된 모델: %d", models.size());
        for (int i = 0; i < models.size(); i++)
        {
            std::string typeStr = (models[i].type == MODEL_GLB) ? "[GLB]" : "[OBJ]";
            ImGui::Text("%d: %s %s", i, typeStr.c_str(), models[i].name.c_str());
        }
    }
    else
    {
        ImGui::Text("로드된 모델이 없습니다. '모델 추가' 버튼을 클릭하세요.");
    }

    ImGui::End();
}

//void ModelManager::RenderRoomProperties()
//{
//    if (!roomModel) return;
//
//    ImGui::Begin("방 속성", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
//
//    // 초기 위치 설정 (첫 실행 시)
//    static bool firstTime = true;
//    if (firstTime) {
//        ImGui::SetWindowPos(ImVec2(50, 500)); // 창 위치 조정
//        firstTime = false;
//    }
//    // 방 크기 조정
//    float width = roomModel->GetRoomWidth();
//    float height = roomModel->GetRoomHeight();
//    float depth = roomModel->GetRoomDepth();
//
//    ImGui::Text("방 크기");
//    if (ImGui::SliderFloat("너비", &width, 5.0f, 50.0f)) {
//        roomModel->SetRoomWidth(width);
//    }
//    if (ImGui::SliderFloat("높이", &height, 5.0f, 30.0f)) {
//        roomModel->SetRoomHeight(height);
//    }
//    if (ImGui::SliderFloat("깊이", &depth, 5.0f, 50.0f)) {
//        roomModel->SetRoomDepth(depth);
//    }
//
//    ImGui::Separator();
//
//    // 방 색상 조정
//    XMFLOAT4 floorColor = roomModel->GetFloorColor();
//    XMFLOAT4 ceilingColor = roomModel->GetCeilingColor();
//    XMFLOAT4 wallColor = roomModel->GetWallColor();
//    XMFLOAT4 windowColor = roomModel->GetWindowColor();
//
//    ImGui::Text("방 색상");
//    if (ImGui::ColorEdit3("바닥 색상", (float*)&floorColor)) {
//        roomModel->SetFloorColor(floorColor);
//    }
//    if (ImGui::ColorEdit3("천장 색상", (float*)&ceilingColor)) {
//        roomModel->SetCeilingColor(ceilingColor);
//    }
//    if (ImGui::ColorEdit3("벽 색상", (float*)&wallColor)) {
//        roomModel->SetWallColor(wallColor);
//    }
//    if (ImGui::ColorEdit4("창문 색상", (float*)&windowColor)) {
//        roomModel->SetWindowColor(windowColor);
//    }
//
//    // 창문 유무 토글
//    bool hasWindow = roomModel->GetHasWindow();
//    if (ImGui::Checkbox("창문 표시", &hasWindow)) {
//        roomModel->SetHasWindow(hasWindow);
//    }
//
//    ImGui::End();
//}

bool ModelManager::OpenObjFileDialog(HWND hwnd, std::string& filePath)
{
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "OBJ 파일\0*.obj\0모든 파일\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn) == TRUE)
    {
        filePath = ofn.lpstrFile;
        return true;
    }
    return false;
}

bool ModelManager::OpenGlbFileDialog(HWND hwnd, std::string& filePath)
{
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "GLB/GLTF 파일\0*.glb;*.gltf\0모든 파일\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn) == TRUE)
    {
        filePath = ofn.lpstrFile;
        return true;
    }
    return false;
}

void ModelManager::Release()
{
    // 로딩 중인 모든 요청을 기다림
    {
        std::lock_guard<std::mutex> lock(loadRequestsMutex);
        for (auto& request : loadRequests) {
            if (request.future.valid()) {
                try {
                    request.future.wait(); // 스레드 종료 대기
                }
                catch (...) {
                    // 예외 무시
                }
            }
        }
        loadRequests.clear();
    }

    {
        std::lock_guard<std::mutex> lock(progressMutex);
        loadingProgresses.clear();
    }

    // 모든 모델 해제
    for (auto& modelInfo : models) {
        modelInfo.model->Release();
    }
    models.clear();
}
// 향상된 UI 렌더링 함수
void ModelManager::RenderEnhancedUI(HWND hwnd, ID3D11Device* device, float deltaTime)
{
    this->device = device;


    // 최초 호출 시 조명 관리자 초기화
    static bool firstTime = true;
    if (firstTime && device) {
        InitLightManager(device);
        firstTime = false;
    }

    // 프레임 처리
    ProcessFrame(hwnd, deltaTime);

    // 로딩 상태 업데이트
    UpdateLoadingStatus();

    // UI 테마 설정
    static bool uiInitialized = false;
    if (!uiInitialized) {
        EnhancedUI::SetupTheme();
        uiInitialized = true;
    }

    // 로딩 진행 상태 창 렌더링 (간결하게 유지)
    RenderLoadingProgress();

    // 메인 UI 레이아웃 
    RenderMainInterface(hwnd);

    //// 조명 설정 UI 렌더링
    //if (lightManager) {
    //    lightManager->RenderUI();
    //}
}

// 메인 인터페이스 렌더링 (전체 UI 통합)
void ModelManager::RenderMainInterface(HWND hwnd)
{
    // 상단 툴바 
    RenderToolbar(hwnd);

    // 좌측 모델 목록 패널
    ImGui::SetNextWindowPos(ImVec2(10, 70), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(220, 500), ImGuiCond_FirstUseEver);
    ImGui::Begin("모델 브라우저", nullptr, ImGuiWindowFlags_NoCollapse);
    RenderModelBrowser();
    ImGui::End();

    // 우측 속성 패널
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 350, 70), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(340, 500), ImGuiCond_FirstUseEver);
    ImGui::Begin("속성", nullptr, ImGuiWindowFlags_NoCollapse);
    RenderPropertiesPanel();
    ImGui::End();

    // 하단 상태바 (상태 정보 표시)
    RenderStatusBar();

  


}

// 사용법 도움말 팝업 표시
void ModelManager::RenderDragHelpPopup()
{
    if (ImGui::BeginPopup("DragHelpPopup")) {
        ImGui::Text("모델 드래그 사용법");
        ImGui::Separator();

        ImGui::BulletText("좌클릭 + 드래그: 모든 방향으로 자유롭게 이동");
        ImGui::BulletText("Shift + 좌클릭 + 드래그: Y축(상하)으로만 이동");
        ImGui::BulletText("Ctrl + 좌클릭 + 드래그: X축(좌우)으로만 이동");
        ImGui::BulletText("Alt + 좌클릭 + 드래그: Z축(앞뒤)으로만 이동");

        ImGui::EndPopup();
    }
}


// 툴바 렌더링 (상단 버튼 모음)
void ModelManager::RenderToolbar(HWND hwnd)
{
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 60));
    ImGui::Begin("##toolbar", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse);

    // 버튼 크기 설정
    ImVec2 buttonSize(100, 40);

    // OBJ 모델 추가 버튼
    if (EnhancedUI::IconButton("OBJ 추가", EnhancedUI::ICON_OBJ, buttonSize)) {
        std::string filePath;
        if (OpenObjFileDialog(hwnd, filePath)) {
            AddObjModelAsync(filePath);
        }
    }
    ImGui::SameLine();

    // GLB 모델 추가 버튼
    if (EnhancedUI::IconButton("GLB 추가", EnhancedUI::ICON_GLB, buttonSize)) {
        std::string filePath;
        if (OpenGlbFileDialog(hwnd, filePath)) {
            AddGlbModelAsync(filePath);
        }
    }
    ImGui::SameLine();

    // 모델 제거 버튼
    if (EnhancedUI::IconButton("모델 제거", EnhancedUI::ICON_REMOVE, buttonSize)) {
        if (selectedModelIndex >= 0) {
            RemoveModel(selectedModelIndex);
        }
    }
    ImGui::SameLine();

    ImGui::SameLine(ImGui::GetWindowWidth() - 420); // 위치 조정

    // 조명 설정 버튼 추가
    static bool showLightSettings = false;
    if (EnhancedUI::IconButton("조명 설정", "L", ImVec2(120, 40))) { // "L"를 아이콘으로 사용
        showLightSettings = !showLightSettings;
    }

    ImGui::SameLine();


    ImGui::SameLine(ImGui::GetWindowWidth() - 280);

    
    // 방 설정 버튼
    static bool showRoomSettings = false;
    if (EnhancedUI::IconButton("방 설정", EnhancedUI::ICON_ROOM, ImVec2(120, 40))) {
        showRoomSettings = !showRoomSettings;
    }

    ImGui::SameLine();
  

    // 카메라 설정 버튼 
    static bool showCameraSettings = false;


    if (EnhancedUI::IconButton("카메라", EnhancedUI::ICON_CAMERA, ImVec2(120, 40))) {
        showCameraSettings = !showCameraSettings;
    }

    // 도움말 버튼 추가
    ImGui::SameLine(ImGui::GetWindowWidth() - 640);

    if (EnhancedUI::IconButton("드래그 도움말", "?", ImVec2(160, 40))) {
        ImGui::OpenPopup("DragHelpPopup");
    }

    // 카메라 모드 전환 버튼 추가
    ImGui::SameLine(ImGui::GetWindowWidth() - 460);
    std::string cameraButtonLabel = isFirstPersonMode ? "자유 시점" : "1인칭 시점";
    if (EnhancedUI::IconButton(cameraButtonLabel.c_str(), EnhancedUI::ICON_CAMERA)) {
        ToggleCameraMode();
    }


    // 도움말 팝업 렌더링
    RenderDragHelpPopup();

    ImGui::End();
  
    // 조명 설정 창 표시 (LightManager의 RenderUI 함수 활용)
    if (showLightSettings && lightManager){ 
            lightManager->RenderUI(); // 기존 LightManager의 UI 렌더링 활용
    }

    if (showCameraSettings) {
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x / 2 - 150, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
        ImGui::Begin("카메라 설정", &showCameraSettings);
        camera.RenderEnhancedUI(); // 이 부분이 문제일 수 있음
        ImGui::End();
    }
    
    // 방 설정 창 (팝업 대신 일반 창으로)
    if (showRoomSettings && roomModel) {
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x / 2 - 150, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("방 설정", &showRoomSettings)) {
            RenderRoomProperties();
            ImGui::End();
        }
    }

    

}

// 모델 브라우저 렌더링 (좌측 패널)
void ModelManager::RenderModelBrowser()
{
    // 상단 검색창
    static char searchBuffer[128] = "";
    ImGui::PushItemWidth(-1);
    if (ImGui::InputTextWithHint("##search", "모델 검색...", searchBuffer, IM_ARRAYSIZE(searchBuffer)))
    {
        // 검색어로 필터링 기능 (나중에 구현)
    }
    ImGui::PopItemWidth();

    ImGui::Spacing();
    EnhancedUI::RenderHeader("모델 목록");

    // 모델 리스트
    if (ImGui::BeginChild("ModelList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true)) {
        if (models.empty()) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "모델이 없습니다.\n상단 버튼을 클릭하여\n모델을 추가하세요.");
        }
        else {
            for (int i = 0; i < models.size(); i++)
            {
                const auto& modelInfo = models[i];
                std::string displayName = modelInfo.name;

                // 모델 리스트 아이템 (커스텀 스타일 적용)
                if (EnhancedUI::ModelListItem(displayName, selectedModelIndex == i, modelInfo.type))
                {
                    selectedModelIndex = i;

                    // 첫 번째 재질 선택
                    if (modelInfo.type == MODEL_OBJ) {
                        auto objWrapper = std::static_pointer_cast<ObjModelWrapper>(modelInfo.model);
                        const auto& materials = objWrapper->model->GetMaterials();
                        selectedMaterialName = materials.empty() ? "" : materials.begin()->first;
                    }
                    else {
                        auto glbWrapper = std::static_pointer_cast<GlbModelWrapper>(modelInfo.model);
                        const auto& materials = glbWrapper->model->GetMaterials();
                        selectedMaterialName = materials.empty() ? "" : materials.begin()->first;
                    }
                }
            }
        }
        ImGui::EndChild();
    }

    // 모델 통계 정보
    ImGui::Text("총 모델 수: %d", (int)models.size());
}

// 속성 패널 렌더링 (우측 패널)
void ModelManager::RenderPropertiesPanel()
{
    if (selectedModelIndex < 0 || selectedModelIndex >= models.size()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "왼쪽 목록에서 모델을 선택하세요");
        return;
    }

    // 탭바 시작
    if (EnhancedUI::BeginTabBar("PropertiesTabs")) {
        // 모델 탭
        if (ImGui::BeginTabItem("모델 속성")) {
            ImGui::BeginChild("ModelPropertiesChild", ImVec2(0, 0), false);
            RenderModelProperties(selectedModelIndex);
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        // 재질 탭
        if (ImGui::BeginTabItem("재질 속성")) {
            ImGui::BeginChild("MaterialPropertiesChild", ImVec2(0, 0), false);

            if (models[selectedModelIndex].type == MODEL_OBJ) {
                auto objWrapper = std::static_pointer_cast<ObjModelWrapper>(models[selectedModelIndex].model);
                RenderObjMaterialPropertiesEnhanced(objWrapper->model);
            }
            else {
                auto glbWrapper = std::static_pointer_cast<GlbModelWrapper>(models[selectedModelIndex].model);
                RenderGlbMaterialPropertiesEnhanced(glbWrapper->model);
            }

            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        EnhancedUI::EndTabBar();
    }
}

// 향상된 OBJ 재질 속성 패널
void ModelManager::RenderObjMaterialPropertiesEnhanced(std::shared_ptr<Model> model)
{
    if (!model) return;

    const auto& materials = model->GetMaterials();
    if (materials.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "이 모델에는 재질이 없습니다");
        return;
    }

    // 재질 선택 콤보박스
    EnhancedUI::RenderHeader("재질 선택");

    ImGui::PushItemWidth(-1);
    if (ImGui::BeginCombo("##MaterialCombo", selectedMaterialName.c_str()))
    {
        for (const auto& material : materials)
        {
            bool isSelected = (selectedMaterialName == material.first);
            if (ImGui::Selectable(material.first.c_str(), isSelected))
            {
                selectedMaterialName = material.first;
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    // 선택된 재질 속성 편집
    auto it = materials.find(selectedMaterialName);
    if (it != materials.end())
    {
        // 재질을 직접 수정하기 위해 const를 제거
        auto& material = const_cast<Model::Material&>(it->second);

        EnhancedUI::RenderHeader("색상 속성");

        // 앰비언트 색상 편집
        EnhancedUI::ColorEdit("주변광 색상", (float*)&material.Ambient, "물체에 적용되는 기본 빛의 색상");

        // 디퓨즈 색상 편집
        EnhancedUI::ColorEdit("확산광 색상", (float*)&material.Diffuse, "직접적인 빛에 반응하는 물체의 기본 색상");

        // 스페큘러 색상 편집
        EnhancedUI::ColorEdit("반사광 색상", (float*)&material.Specular, "반짝이는 하이라이트의 색상");

        // 광택도 편집
        EnhancedUI::SliderFloat("광택도", &material.Shininess, 1.0f, 128.0f, "하이라이트의 집중도. 높을수록 더 날카롭게 반짝입니다");

        EnhancedUI::RenderHeader("텍스처");

        // 텍스처 정보
        if (!material.DiffuseMapPath.empty())
        {
            ImGui::Text("현재 텍스처:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "%s",
                material.DiffuseMapPath.substr(material.DiffuseMapPath.find_last_of("/\\") + 1).c_str());

            ImGui::Text("로드 상태: %s", material.DiffuseMap ? "성공" : "실패");

            // 텍스처 미리보기 (실제로는 텍스처 ID를 제공해야 함)
            // 이 부분은 실제 텍스처 미리보기를 위해 추가 구현이 필요함
            ImGui::Spacing();

            // 텍스처 변경 버튼
            if (EnhancedUI::IconButton("텍스처 변경", EnhancedUI::ICON_IMPORT)) {
                std::string texturePath;
                if (OpenTextureFileDialog(GetActiveWindow(), texturePath)) {
                    // 선택된 텍스처 로드
                    model->LoadTexture(texturePath, device, &material.DiffuseMap);
                    material.DiffuseMapPath = texturePath;
                }
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "텍스처가 없습니다");
            ImGui::Spacing();

            // 텍스처 추가 버튼
            if (EnhancedUI::IconButton("텍스처 추가", EnhancedUI::ICON_IMPORT)) {
                std::string texturePath;
                if (OpenTextureFileDialog(GetActiveWindow(), texturePath)) {
                    // 선택된 텍스처 로드
                    model->LoadTexture(texturePath, device, &material.DiffuseMap);
                    material.DiffuseMapPath = texturePath;
                }
            }
        }
    }
}

// 향상된 GLB 재질 속성 패널
void ModelManager::RenderGlbMaterialPropertiesEnhanced(std::shared_ptr<GltfLoader> model)
{
    if (!model) return;

    const auto& materials = model->GetMaterials();
    if (materials.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "이 모델에는 재질이 없습니다");
        return;
    }

    // 재질 선택 콤보박스
    EnhancedUI::RenderHeader("PBR 재질 선택");

    ImGui::PushItemWidth(-1);
    if (ImGui::BeginCombo("##MaterialCombo", selectedMaterialName.c_str()))
    {
        for (const auto& material : materials)
        {
            bool isSelected = (selectedMaterialName == material.first);
            if (ImGui::Selectable(material.first.c_str(), isSelected))
            {
                selectedMaterialName = material.first;
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    // 선택된 재질 정보 표시 및 편집
    auto it = materials.find(selectedMaterialName);
    if (it != materials.end())
    {
        // 재질을 직접 수정하기 위해 const 제거
        auto& material = const_cast<GltfLoader::PbrMaterial&>(it->second);

        EnhancedUI::RenderHeader("PBR 재질 속성");

        // 베이스 컬러 편집
        EnhancedUI::ColorEdit("베이스 컬러", (float*)&material.BaseColorFactor, "물체의 기본 색상");

        // 이미시브 편집
        EnhancedUI::ColorEdit("자체발광 컬러", (float*)&material.EmissiveFactor, "물체가 자체적으로 발광하는 색상");

        // 메탈릭 인자 편집
        EnhancedUI::SliderFloat("메탈릭 인자", &material.MetallicFactor, 0.0f, 1.0f,
            "0: 비금속(플라스틱 등), 1: 금속. 반사 특성을 결정합니다");

        // 러프니스 인자 편집
        EnhancedUI::SliderFloat("러프니스 인자", &material.RoughnessFactor, 0.0f, 1.0f,
            "0: 매끄러움(거울), 1: 거침(분산된 반사). 표면의 미세한 거칠기를 결정합니다");

        // 텍스처 정보 표시
        EnhancedUI::RenderHeader("텍스처 맵");

        ImGui::BeginChild("TextureInfo", ImVec2(0, 150), true);

        // 베이스 컬러 텍스처
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "베이스 컬러 텍스처:");
        if (!material.BaseColorTexturePath.empty()) {
            ImGui::SameLine();
            ImGui::Text("%s", material.BaseColorTexturePath.substr(
                material.BaseColorTexturePath.find_last_of("/\\") + 1).c_str());
        }
        else {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "없음");
        }

        // 메탈릭 러프니스 텍스처
        ImGui::TextColored(ImVec4(0.2f, 0.7f, 0.9f, 1.0f), "메탈릭-러프니스 텍스처:");
        if (!material.MetallicRoughnessTexturePath.empty()) {
            ImGui::SameLine();
            ImGui::Text("%s", material.MetallicRoughnessTexturePath.substr(
                material.MetallicRoughnessTexturePath.find_last_of("/\\") + 1).c_str());
        }
        else {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "없음");
        }

        // 노멀 텍스처
        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "노멀 텍스처:");
        if (!material.NormalTexturePath.empty()) {
            ImGui::SameLine();
            ImGui::Text("%s", material.NormalTexturePath.substr(
                material.NormalTexturePath.find_last_of("/\\") + 1).c_str());
        }
        else {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "없음");
        }

        // 이미시브 텍스처
        ImGui::TextColored(ImVec4(0.9f, 0.5f, 0.2f, 1.0f), "이미시브 텍스처:");
        if (!material.EmissiveTexturePath.empty()) {
            ImGui::SameLine();
            ImGui::Text("%s", material.EmissiveTexturePath.substr(
                material.EmissiveTexturePath.find_last_of("/\\") + 1).c_str());
        }
        else {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "없음");
        }

        // 오클루전 텍스처
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "오클루전 텍스처:");
        if (!material.OcclusionTexturePath.empty()) {
            ImGui::SameLine();
            ImGui::Text("%s", material.OcclusionTexturePath.substr(
                material.OcclusionTexturePath.find_last_of("/\\") + 1).c_str());
        }
        else {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "없음");
        }

        ImGui::EndChild();

        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f),
            "참고: GLB 모델의 텍스처는 변경할 수 없습니다.");
    }
}

// 기존 모델 속성 편집 함수 대체
void ModelManager::RenderModelProperties(int modelIndex)
{
    if (modelIndex < 0 || modelIndex >= models.size())
        return;

    const auto& modelInfo = models[modelIndex];

    EnhancedUI::RenderHeader("모델 기본 정보");

    // 모델 타입 표시
    ImGui::Text("모델 타입: ");
    ImGui::SameLine();
    if (modelInfo.type == MODEL_OBJ) {
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "OBJ");
    }
    else {
        ImGui::TextColored(ImVec4(0.2f, 0.6f, 0.9f, 1.0f), "GLB");
    }

    // 파일 경로 표시
    std::string filename = modelInfo.path.substr(modelInfo.path.find_last_of("/\\") + 1);
    ImGui::Text("파일 이름: %s", filename.c_str());

    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("%s", modelInfo.path.c_str());
        ImGui::EndTooltip();
    }

    ImGui::Spacing();
    EnhancedUI::RenderHeader("변형");

    if (modelInfo.type == MODEL_OBJ) {
        auto objWrapper = std::static_pointer_cast<ObjModelWrapper>(modelInfo.model);
        auto& objModelInfo = objWrapper->model->GetModelInfo();

        // 가시성
        ImGui::Checkbox("표시", &objModelInfo.Visible);
        ImGui::Spacing();

        // 위치 조절
        if (ImGui::TreeNodeEx("위치", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushItemWidth(-1);
            EnhancedUI::SliderFloat("X##Pos", &objModelInfo.Position.x, -10.0f, 10.0f);
            EnhancedUI::SliderFloat("Y##Pos", &objModelInfo.Position.y, -10.0f, 10.0f);
            EnhancedUI::SliderFloat("Z##Pos", &objModelInfo.Position.z, -10.0f, 10.0f);
            ImGui::PopItemWidth();
            ImGui::TreePop();
        }

        // 회전 조절
        if (ImGui::TreeNodeEx("회전", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushItemWidth(-1);
            EnhancedUI::SliderFloat("X (도)##Rot", &objModelInfo.Rotation.x, -180.0f, 180.0f);
            EnhancedUI::SliderFloat("Y (도)##Rot", &objModelInfo.Rotation.y, -180.0f, 180.0f);
            EnhancedUI::SliderFloat("Z (도)##Rot", &objModelInfo.Rotation.z, -180.0f, 180.0f);
            ImGui::PopItemWidth();
            ImGui::TreePop();
        }

        // 크기 조절
        if (ImGui::TreeNodeEx("크기", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushItemWidth(-1);
            EnhancedUI::SliderFloat("X##Scale", &objModelInfo.Scale.x, 0.1f, 5.0f);
            EnhancedUI::SliderFloat("Y##Scale", &objModelInfo.Scale.y, 0.1f, 5.0f);
            EnhancedUI::SliderFloat("Z##Scale", &objModelInfo.Scale.z, 0.1f, 5.0f);
            ImGui::PopItemWidth();

            // 균일 크기 조절 버튼
            if (ImGui::Button("균일 크기 적용")) {
                float avg = (objModelInfo.Scale.x + objModelInfo.Scale.y + objModelInfo.Scale.z) / 3.0f;
                objModelInfo.Scale = XMFLOAT3(avg, avg, avg);
            }

            ImGui::TreePop();
        }
    }
    else if (modelInfo.type == MODEL_GLB) {
        auto glbWrapper = std::static_pointer_cast<GlbModelWrapper>(modelInfo.model);
        auto& glbModelInfo = glbWrapper->model->GetModelInfo();

        // 가시성
        ImGui::Checkbox("표시", &glbModelInfo.Visible);
        ImGui::Spacing();

        // 위치 조절
        if (ImGui::TreeNodeEx("위치", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushItemWidth(-1);
            EnhancedUI::SliderFloat("X##Pos", &glbModelInfo.Position.x, -10.0f, 10.0f);
            EnhancedUI::SliderFloat("Y##Pos", &glbModelInfo.Position.y, -10.0f, 10.0f);
            EnhancedUI::SliderFloat("Z##Pos", &glbModelInfo.Position.z, -10.0f, 10.0f);
            ImGui::PopItemWidth();
            ImGui::TreePop();
        }

        // 회전 조절
        if (ImGui::TreeNodeEx("회전", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushItemWidth(-1);
            EnhancedUI::SliderFloat("X (도)##Rot", &glbModelInfo.Rotation.x, -180.0f, 180.0f);
            EnhancedUI::SliderFloat("Y (도)##Rot", &glbModelInfo.Rotation.y, -180.0f, 180.0f);
            EnhancedUI::SliderFloat("Z (도)##Rot", &glbModelInfo.Rotation.z, -180.0f, 180.0f);
            ImGui::PopItemWidth();
            ImGui::TreePop();
        }

        // 크기 조절
        if (ImGui::TreeNodeEx("크기", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushItemWidth(-1);
            EnhancedUI::SliderFloat("X##Scale", &glbModelInfo.Scale.x, 0.1f, 5.0f);
            EnhancedUI::SliderFloat("Y##Scale", &glbModelInfo.Scale.y, 0.1f, 5.0f);
            EnhancedUI::SliderFloat("Z##Scale", &glbModelInfo.Scale.z, 0.1f, 5.0f);
            ImGui::PopItemWidth();

            // 균일 크기 조절 버튼
            if (ImGui::Button("균일 크기 적용")) {
                float avg = (glbModelInfo.Scale.x + glbModelInfo.Scale.y + glbModelInfo.Scale.z) / 3.0f;
                glbModelInfo.Scale = XMFLOAT3(avg, avg, avg);
            }

            ImGui::TreePop();
        }

        // GLB 애니메이션 컨트롤 (있는 경우)
        const auto& animations = glbWrapper->model->GetAnimations();
        if (!animations.empty()) {
            EnhancedUI::RenderHeader("애니메이션");

            // 현재 애니메이션 인덱스와 시간 가져오기
            int currentAnim = glbWrapper->model->GetCurrentAnimationIndex();
            float currentTime = glbWrapper->model->GetCurrentAnimationTime();

            // 애니메이션 선택 콤보박스
            std::vector<std::string> animNames;
            for (const auto& anim : animations) {
                animNames.push_back(anim.Name);
            }

            ImGui::PushItemWidth(-1);
            if (ImGui::BeginCombo("애니메이션",
                (currentAnim >= 0 && currentAnim < animNames.size()) ?
                animNames[currentAnim].c_str() : "없음"))
            {
                for (int i = 0; i < animNames.size(); i++) {
                    bool isSelected = (currentAnim == i);
                    if (ImGui::Selectable(animNames[i].c_str(), isSelected)) {
                        glbWrapper->model->SetCurrentAnimation(i);
                    }

                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();

            // 애니메이션 재생/일시정지 버튼
            bool isPlaying = glbWrapper->model->IsAnimationPlaying();
            if (isPlaying) {
                if (ImGui::Button("일시정지", ImVec2(100, 0))) {
                    glbWrapper->model->PauseAnimation();
                }
            }
            else {
                if (ImGui::Button("재생", ImVec2(100, 0))) {
                    glbWrapper->model->PlayAnimation();
                }
            }

            // 애니메이션 속도 조절
            float speed = glbWrapper->model->GetAnimationSpeed();
            if (EnhancedUI::SliderFloat("속도", &speed, 0.1f, 2.0f, "애니메이션 재생 속도")) {
                glbWrapper->model->SetAnimationSpeed(speed);
            }

            // 현재 시간 표시
            if (currentAnim >= 0 && currentAnim < animations.size()) {
                const auto& anim = animations[currentAnim];
                ImGui::Text("시간: %.2f / %.2f", currentTime, anim.EndTime);
            }
        }
    }
}

// 로딩 진행 상태 창 렌더링 (개선된 버전)
void ModelManager::RenderLoadingProgress()
{
    std::lock_guard<std::mutex> lock(progressMutex);

    if (loadingProgresses.empty()) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Once);
    if (ImGui::Begin("모델 로딩 상태", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
        for (const auto& progress : loadingProgresses) {
            std::string label = progress->filename;

            if (progress->isComplete) {
                ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "✓ %s", label.c_str());
                ImGui::ProgressBar(1.0f, ImVec2(-1, 8), "");
                continue;
            }

            if (progress->isError) {
                ImGui::TextColored(ImVec4(0.8f, 0.0f, 0.0f, 1.0f), "✗ %s", label.c_str());
                ImGui::ProgressBar(0.0f, ImVec2(-1, 8), "");
                ImGui::TextColored(ImVec4(0.8f, 0.0f, 0.0f, 1.0f), "오류: %s", progress->errorMessage.c_str());
                continue;
            }

            ImGui::Text("%s", label.c_str());
            ImGui::ProgressBar(progress->progress, ImVec2(-1, 8), "");
        }
    }
    ImGui::End();
}

// 방 속성 렌더링 (개선된 버전)
void ModelManager::RenderRoomProperties()
{
    if (!roomModel) return;

    // 방 크기 조정
    float width = roomModel->GetRoomWidth();
    float height = roomModel->GetRoomHeight();
    float depth = roomModel->GetRoomDepth();

    EnhancedUI::RenderHeader("방 크기");

    bool sizeChanged = false;

    ImGui::PushItemWidth(-1);
    sizeChanged |= EnhancedUI::SliderFloat("너비", &width, 5.0f, 50.0f, "방의 X축 길이");
    sizeChanged |= EnhancedUI::SliderFloat("높이", &height, 5.0f, 30.0f, "방의 Y축 높이");
    sizeChanged |= EnhancedUI::SliderFloat("깊이", &depth, 5.0f, 50.0f, "방의 Z축 길이");
    ImGui::PopItemWidth();

    if (sizeChanged) {
        roomModel->SetRoomWidth(width);
        roomModel->SetRoomHeight(height);
        roomModel->SetRoomDepth(depth);
    }

    EnhancedUI::RenderHeader("방 색상");

    // 방 색상 조정
    XMFLOAT4 floorColor = roomModel->GetFloorColor();
    XMFLOAT4 ceilingColor = roomModel->GetCeilingColor();
    XMFLOAT4 wallColor = roomModel->GetWallColor();
    XMFLOAT4 windowColor = roomModel->GetWindowColor();

    bool colorChanged = false;

    colorChanged |= EnhancedUI::ColorEdit("바닥 색상", (float*)&floorColor, "바닥의 색상");
    colorChanged |= EnhancedUI::ColorEdit("천장 색상", (float*)&ceilingColor, "천장의 색상");
    colorChanged |= EnhancedUI::ColorEdit("벽 색상", (float*)&wallColor, "벽의 색상");
    colorChanged |= EnhancedUI::ColorEdit("창문 색상", (float*)&windowColor, "창문의 색상 및 투명도");

    if (colorChanged) {
        roomModel->SetFloorColor(floorColor);
        roomModel->SetCeilingColor(ceilingColor);
        roomModel->SetWallColor(wallColor);
        roomModel->SetWindowColor(windowColor);
    }

    // 창문 유무 토글
    bool hasWindow = roomModel->GetHasWindow();
    if (ImGui::Checkbox("창문 표시", &hasWindow)) {
        roomModel->SetHasWindow(hasWindow);
    }

    // 프리셋 버튼 (추가 기능)
    ImGui::Spacing();
    EnhancedUI::RenderHeader("색상 프리셋");

    if (ImGui::Button("밝은 방", ImVec2(95, 0))) {
        roomModel->SetFloorColor(XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f));
        roomModel->SetCeilingColor(XMFLOAT4(0.95f, 0.95f, 0.95f, 1.0f));
        roomModel->SetWallColor(XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f));
    }
    ImGui::SameLine();

    if (ImGui::Button("어두운 방", ImVec2(95, 0))) {
        roomModel->SetFloorColor(XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f));
        roomModel->SetCeilingColor(XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f));
        roomModel->SetWallColor(XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f));
    }
    ImGui::SameLine();

    if (ImGui::Button("따뜻한 방", ImVec2(95, 0))) {
        roomModel->SetFloorColor(XMFLOAT4(0.8f, 0.6f, 0.4f, 1.0f));
        roomModel->SetCeilingColor(XMFLOAT4(0.9f, 0.85f, 0.7f, 1.0f));
        roomModel->SetWallColor(XMFLOAT4(0.9f, 0.8f, 0.6f, 1.0f));
    }
}

// 기존 RenderStatusBar 함수 수정
void ModelManager::RenderStatusBar()
{
    ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetIO().DisplaySize.y - 25));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 25));
    ImGui::Begin("##statusbar", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse);

    // 좌측에 모델 수 표시
    ImGui::Text("모델: %d개", (int)models.size());

    // 드래그 상태 정보 표시
    RenderDragStatusInfo();

    ImGui::SameLine(ImGui::GetWindowWidth() - 280);

    // 현재 선택된 모델 표시
    if (selectedModelIndex >= 0 && selectedModelIndex < models.size()) {
        const auto& modelInfo = models[selectedModelIndex];
        std::string modelType = (modelInfo.type == MODEL_GLB) ? "GLB" : "OBJ";
        ImGui::Text("선택: %s (%s)", modelInfo.name.c_str(), modelType.c_str());
    }
    else {
        ImGui::Text("선택된 모델 없음");
    }

    ImGui::End();
}


// ModelManager.cpp 파일에 다음 함수들을 추가합니다.

// 스크린 좌표로부터 3D 레이 생성
Ray ModelManager::CreateRayFromScreenPoint(int screenX, int screenY, int screenWidth, int screenHeight)
{
    Ray ray;

    // 카메라 위치를 레이 원점으로 사용
    ray.origin = camera.GetPosition();

    // 스크린 좌표를 정규화된 장치 좌표(NDC)로 변환 (-1에서 1 사이)
    float ndcX = (2.0f * screenX / screenWidth) - 1.0f;
    float ndcY = 1.0f - (2.0f * screenY / screenHeight);

    // 정규화된 장치 좌표로 레이 방향 계산
    XMFLOAT4 rayStartNDC = XMFLOAT4(ndcX, ndcY, 0.0f, 1.0f);
    XMFLOAT4 rayEndNDC = XMFLOAT4(ndcX, ndcY, 1.0f, 1.0f);

    // NDC 좌표를 월드 좌표로 변환하기 위한 역행렬 계산
    XMMATRIX viewMatrix = camera.GetViewMatrix();
    XMMATRIX projMatrix = camera.GetProjectionMatrix();
    XMMATRIX viewProjMatrix = XMMatrixMultiply(viewMatrix, projMatrix);
    XMMATRIX invViewProj = XMMatrixInverse(nullptr, viewProjMatrix);

    // NDC 좌표를 월드 좌표로 변환
    XMVECTOR rayStartWorld = XMVector4Transform(XMLoadFloat4(&rayStartNDC), invViewProj);
    XMVECTOR rayEndWorld = XMVector4Transform(XMLoadFloat4(&rayEndNDC), invViewProj);

    // w로 나누어 투영 변환
    rayStartWorld = XMVectorDivide(rayStartWorld, XMVectorSplatW(rayStartWorld));
    rayEndWorld = XMVectorDivide(rayEndWorld, XMVectorSplatW(rayEndWorld));

    // 카메라 위치를 시작점으로, 방향을 계산
    XMVECTOR rayOrigin = XMLoadFloat3(&ray.origin);
    XMVECTOR rayDir = XMVector3Normalize(XMVectorSubtract(rayEndWorld, rayStartWorld));

    // 결과 저장
    XMStoreFloat3(&ray.direction, rayDir);

    return ray;
}

// 레이와 바운딩 박스 교차 검사
bool ModelManager::RayIntersectsBoundingBox(const Ray& ray, const BoundingBox& box)
{
    // 간단한 구 충돌 테스트 (빠른 판단을 위해)
    XMVECTOR rayOrigin = XMLoadFloat3(&ray.origin);
    XMVECTOR rayDir = XMLoadFloat3(&ray.direction);
    XMVECTOR sphereCenter = XMLoadFloat3(&box.center);

    XMVECTOR originToCenter = XMVectorSubtract(sphereCenter, rayOrigin);
    float tca = XMVectorGetX(XMVector3Dot(originToCenter, rayDir));

    // 레이가 구에서 멀리 떨어져 있는 경우
    if (tca < 0.0f)
        return false;

    float d2 = XMVectorGetX(XMVector3Dot(originToCenter, originToCenter)) - tca * tca;
    float radius2 = box.radius * box.radius;

    // 레이가 구를 지나가지 않는 경우
    if (d2 > radius2)
        return false;

    // 보다 정확한 바운딩 박스 충돌 검사
    // AABB와 레이 충돌 계산 (슬랩 메서드)
    XMFLOAT3 invDir = XMFLOAT3(
        1.0f / ray.direction.x,
        1.0f / ray.direction.y,
        1.0f / ray.direction.z
    );

    XMFLOAT3 t1 = XMFLOAT3(
        (box.min.x - ray.origin.x) * invDir.x,
        (box.min.y - ray.origin.y) * invDir.y,
        (box.min.z - ray.origin.z) * invDir.z
    );

    XMFLOAT3 t2 = XMFLOAT3(
        (box.max.x - ray.origin.x) * invDir.x,
        (box.max.y - ray.origin.y) * invDir.y,
        (box.max.z - ray.origin.z) * invDir.z
    );

    // min/max 교체 (invDir이 음수인 경우)
    float tmin = max(
        min(t1.x, t2.x),
        max(min(t1.y, t2.y), min(t1.z, t2.z))
    );

    float tmax = min(
        max(t1.x, t2.x),
        min(max(t1.y, t2.y), max(t1.z, t2.z))
    );

    // 박스 뒤에 있거나 교차하지 않는 경우
    if (tmax < 0.0f || tmin > tmax)
        return false;

    // 교차 발생
    return true;
}

// 모델 선택 (레이캐스팅 사용)
int ModelManager::PickModel(const Ray& ray)
{
    // 테스트 모드: GLB 모델 우선 선택
    bool testMode = true; // 테스트 모드 활성화
    if (testMode)
    {
        // 가장 가까운 GLB 모델 찾기
        float closestDist = FLT_MAX;
        int closestGlbIndex = -1;

        for (int i = 0; i < models.size(); i++)
        {
            if (models[i].type == MODEL_GLB)
            {
                XMFLOAT3 modelPos = models[i].model->GetPosition();
                XMVECTOR modelPosVec = XMLoadFloat3(&modelPos);
                XMVECTOR cameraPosVec = XMLoadFloat3(&camera.GetPosition());
                XMVECTOR distVec = XMVectorSubtract(modelPosVec, cameraPosVec);
                float dist = XMVectorGetX(XMVector3Length(distVec));

                if (dist < closestDist)
                {
                    closestDist = dist;
                    closestGlbIndex = i;
                }
            }
        }

        // GLB 모델이 있으면 해당 모델 선택, 없으면 기존 로직 실행
        if (closestGlbIndex >= 0)
        {
            OutputDebugStringA(("테스트 모드: GLB 모델 " + std::to_string(closestGlbIndex) + " 선택됨\n").c_str());
            return closestGlbIndex;
        }
    }
    int pickedModelIndex = -1;
    float closestDistance = FLT_MAX;

    for (int i = 0; i < models.size(); i++)
    {
        BoundingBox box = models[i].model->GetBoundingBox();

        // 각 모델의 바운딩 박스 정보 로그
        std::string modelType = (models[i].type == MODEL_GLB) ? "GLB" : "OBJ";
        OutputDebugStringA(("모델 [" + std::to_string(i) + "] " + modelType +
            " - 중심: " + std::to_string(box.center.x) + "," +
            std::to_string(box.center.y) + "," + std::to_string(box.center.z) +
            " - 반지름: " + std::to_string(box.radius) + "\n").c_str());

        if (RayIntersectsBoundingBox(ray, box))
        {

            OutputDebugStringA(("모델 [" + std::to_string(i) + "] 와 충돌 감지\n").c_str());

            // 충돌 거리 계산 (카메라에서 모델 중심까지의 거리)
            XMVECTOR rayOrigin = XMLoadFloat3(&ray.origin);
            XMVECTOR center = XMLoadFloat3(&box.center);
            XMVECTOR diff = XMVectorSubtract(center, rayOrigin);
            float distance = XMVectorGetX(XMVector3Length(diff));

            if (distance < closestDistance)
            {
                closestDistance = distance;
                pickedModelIndex = i;
            }
        }
    }

    return pickedModelIndex;
}

// 레이와 평면의 교차점 계산
XMFLOAT3 ModelManager::GetPlaneIntersectionPoint(const Ray& ray, const XMFLOAT3& planeNormal, float planeD)
{
    // 레이: P = rayOrigin + t * rayDirection
    // 평면: dot(planeNormal, P) + planeD = 0
    // 교차: t = -(dot(planeNormal, rayOrigin) + planeD) / dot(planeNormal, rayDirection)

    XMVECTOR vRayOrigin = XMLoadFloat3(&ray.origin);
    XMVECTOR vRayDir = XMLoadFloat3(&ray.direction);
    XMVECTOR vPlaneNormal = XMLoadFloat3(&planeNormal);

    float denom = XMVectorGetX(XMVector3Dot(vPlaneNormal, vRayDir));

    // 평면과 레이가 평행한 경우
    if (abs(denom) < 0.0001f)
    {
        // 기본값 반환 (원래는 교차점 없음)
        return ray.origin;
    }

    float t = -(XMVectorGetX(XMVector3Dot(vPlaneNormal, vRayOrigin)) + planeD) / denom;

    // 교차점 계산
    XMVECTOR vIntersection = XMVectorAdd(vRayOrigin, XMVectorScale(vRayDir, t));

    XMFLOAT3 intersection;
    XMStoreFloat3(&intersection, vIntersection);

    return intersection;
}

// OnMouseDown 함수 수정
void ModelManager::OnMouseDown(int x, int y, HWND hwnd)
{
    // ImGui UI 위에서는 작동하지 않도록 함
    if (ImGui::GetIO().WantCaptureMouse)
        return;

    // 왼쪽 마우스 버튼이 눌렸는지 확인
    if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000))
        return;

    // GLB 모델 정보 출력
    OutputDebugStringA("== GLB 모델 정보 ==\n");
    for (int i = 0; i < models.size(); i++)
    {
        if (models[i].type == MODEL_GLB)
        {
            auto glbWrapper = std::static_pointer_cast<GlbModelWrapper>(models[i].model);
            XMFLOAT3 pos = glbWrapper->GetPosition();
            BoundingBox box = glbWrapper->GetBoundingBox();

            OutputDebugStringA(("GLB 모델 [" + std::to_string(i) + "] - " + models[i].name + "\n").c_str());
            OutputDebugStringA(("  위치: " + std::to_string(pos.x) + "," +
                std::to_string(pos.y) + "," + std::to_string(pos.z) + "\n").c_str());
            OutputDebugStringA(("  바운딩 박스 중심: " + std::to_string(box.center.x) + "," +
                std::to_string(box.center.y) + "," + std::to_string(box.center.z) + "\n").c_str());
            OutputDebugStringA(("  바운딩 박스 반지름: " + std::to_string(box.radius) + "\n").c_str());
        }
    }

    OutputDebugStringA(("총 모델 수: " + std::to_string(models.size()) + "\n").c_str());

    for (int i = 0; i < models.size(); i++) {
        std::string modelType = (models[i].type == MODEL_GLB) ? "GLB" : "OBJ";
        OutputDebugStringA(("모델 [" + std::to_string(i) + "] - 타입: " + modelType +
            " - 이름: " + models[i].name + "\n").c_str());

        // 위치 정보도 출력
        XMFLOAT3 pos = models[i].model->GetPosition();
        OutputDebugStringA(("  위치: " + std::to_string(pos.x) + "," +
            std::to_string(pos.y) + "," + std::to_string(pos.z) + "\n").c_str());
    }

    // 현재 윈도우 크기 가져오기
    RECT rect;
    GetClientRect(hwnd, &rect);
    int screenWidth = rect.right - rect.left;
    int screenHeight = rect.bottom - rect.top;

    // 클릭 위치에서 레이 생성
    Ray ray = CreateRayFromScreenPoint(x, y, screenWidth, screenHeight);

    // 특수 키 상태 확인 (드래그 축 설정)
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
        // Shift 키 누름 - Y축 이동
        dragAxis = Y_AXIS;
    }
    else if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
        // Ctrl 키 누름 - X축 이동
        dragAxis = X_AXIS;
    }
    else if (GetAsyncKeyState(VK_MENU) & 0x8000) {
        // Alt 키 누름 - Z축 이동
        dragAxis = Z_AXIS;
    }
    else {
        // 기본 - 자유 이동
        dragAxis = FREE;
    }
    // 클릭 위치 로그
    OutputDebugStringA(("마우스 클릭: X=" + std::to_string(x) + ", Y=" + std::to_string(y) + "\n").c_str());

    // 레이와 충돌하는 모델 찾기
    draggedModelIndex = PickModel(ray);

    // 모델 선택 결과 로그
    OutputDebugStringA(("선택된 모델 인덱스: " + std::to_string(draggedModelIndex) + "\n").c_str());

    // 모델이 선택된 경우 드래그 시작
    if (draggedModelIndex >= 0)
    {
        isDragging = true;
        selectedModelIndex = draggedModelIndex; // UI에서 선택된 모델 업데이트

        // 현재 선택된 모델의 첫 번째 재질 선택
        if (models[draggedModelIndex].type == MODEL_OBJ) {
            auto objWrapper = std::static_pointer_cast<ObjModelWrapper>(models[draggedModelIndex].model);
            const auto& materials = objWrapper->model->GetMaterials();
            if (!materials.empty()) {
                selectedMaterialName = materials.begin()->first;
            }
        }
        else {
            auto glbWrapper = std::static_pointer_cast<GlbModelWrapper>(models[draggedModelIndex].model);
            const auto& materials = glbWrapper->model->GetMaterials();
            if (!materials.empty()) {
                selectedMaterialName = materials.begin()->first;
            }
        }

        // 모델 위치 저장
        dragStartModelPos = models[draggedModelIndex].model->GetPosition();

        // 드래그 평면 설정
        SetupDragPlane(ray);

        // 초기 교차점 계산
        dragStartIntersectPos = GetPlaneIntersectionPoint(ray, dragPlaneNormal, dragPlaneD);
        currentIntersectPos = dragStartIntersectPos;
    }
}

// OnMouseMove 함수 수정
void ModelManager::OnMouseMove(int x, int y, HWND hwnd)
{
    // 드래그 중이고 선택된 모델이 있는 경우
    if (isDragging && draggedModelIndex >= 0)
    {
        // 현재 윈도우 크기 가져오기
        RECT rect;
        GetClientRect(hwnd, &rect);
        int screenWidth = rect.right - rect.left;
        int screenHeight = rect.bottom - rect.top;

        // 현재 마우스 위치에서 레이 생성
        Ray ray = CreateRayFromScreenPoint(x, y, screenWidth, screenHeight);

        // 현재 교차점 계산
        currentIntersectPos = GetPlaneIntersectionPoint(ray, dragPlaneNormal, dragPlaneD);

        // 교차점 이동 벡터 계산
        XMFLOAT3 movement;
        movement.x = currentIntersectPos.x - dragStartIntersectPos.x;
        movement.y = currentIntersectPos.y - dragStartIntersectPos.y;
        movement.z = currentIntersectPos.z - dragStartIntersectPos.z;

        // 선택된 축에 따라 이동 제한
        switch (dragAxis)
        {
        case X_AXIS:
            movement.y = 0.0f;
            movement.z = 0.0f;
            break;

        case Y_AXIS:
            movement.x = 0.0f;
            movement.z = 0.0f;
            break;

        case Z_AXIS:
            movement.x = 0.0f;
            movement.y = 0.0f;
            break;

        case FREE:
        default:
            // 모든 축 자유 이동
            break;
        }

        // 새 모델 위치 계산
        XMFLOAT3 newPosition;
        newPosition.x = dragStartModelPos.x + movement.x;
        newPosition.y = dragStartModelPos.y + movement.y;
        newPosition.z = dragStartModelPos.z + movement.z;

        // 모델 위치 업데이트
        models[draggedModelIndex].model->SetPosition(newPosition);

        // UI 속성 패널도 업데이트 (모델 위치 변경이 UI에 반영되도록)
        if (models[draggedModelIndex].type == MODEL_OBJ) {
            auto objWrapper = std::static_pointer_cast<ObjModelWrapper>(models[draggedModelIndex].model);
            objWrapper->model->GetModelInfo().Position = newPosition;
        }
        else {
            auto glbWrapper = std::static_pointer_cast<GlbModelWrapper>(models[draggedModelIndex].model);
            glbWrapper->model->GetModelInfo().Position = newPosition;
        }
    }
}
// 마우스 버튼 놓기 처리
void ModelManager::OnMouseUp(int x, int y)
{
    // 드래그 끝
    isDragging = false;
    draggedModelIndex = -1;
}
// 드래그 축 제한 설정
void ModelManager::SetDragAxis(DragAxis axis)
{
    dragAxis = axis;
}
// 드래그할 평면 설정 함수 수정
void ModelManager::SetupDragPlane(const Ray& ray)
{
    // 모델의 현재 위치 가져오기
    XMFLOAT3 modelPos = models[draggedModelIndex].model->GetPosition();

    // 카메라 방향 벡터
    XMMATRIX viewMatrix = camera.GetViewMatrix();
    XMVECTOR cameraDir = XMVectorSet(
        viewMatrix.r[2].m128_f32[0],
        viewMatrix.r[2].m128_f32[1],
        viewMatrix.r[2].m128_f32[2],
        0.0f
    );

    // 선택한 축에 따라 다른 평면 법선 벡터 설정
    switch (dragAxis)
    {
    case X_AXIS:
        // YZ 평면 (X축을 따라 이동)
        XMStoreFloat3(&dragPlaneNormal, XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f));
        dragPlaneD = -modelPos.x;
        break;

    case Y_AXIS:
        // XZ 평면 (Y축을 따라 이동)
        XMStoreFloat3(&dragPlaneNormal, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
        dragPlaneD = -modelPos.y;
        break;

    case Z_AXIS:
        // XY 평면 (Z축을 따라 이동)
        XMStoreFloat3(&dragPlaneNormal, XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f));
        dragPlaneD = -modelPos.z;
        break;

    case FREE:
    default:
        // 카메라의 뷰 방향과 수직인 평면
        XMVECTOR normalizedCameraDir = XMVector3Normalize(cameraDir);
        XMStoreFloat3(&dragPlaneNormal, normalizedCameraDir);

        // 평면의 d 값: -dot(normal, point)
        XMVECTOR planeNormal = XMLoadFloat3(&dragPlaneNormal);
        XMVECTOR modelPosVector = XMLoadFloat3(&modelPos);
        dragPlaneD = -XMVectorGetX(XMVector3Dot(planeNormal, modelPosVector));
        break;
    }
}
// 상태바에 드래그 정보 표시 함수
void ModelManager::RenderDragStatusInfo()
{
    // 드래그 중인 경우 상태 정보 표시
    if (isDragging && draggedModelIndex >= 0)
    {
        ImGui::SameLine(ImGui::GetWindowWidth() - 500);

        // 선택된 모델 이름
        std::string modelName = models[draggedModelIndex].name;
        std::string axisName;

        // 현재 드래그 축 표시
        switch (dragAxis)
        {
        case X_AXIS:
            axisName = "X축";
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "이동 중: %s (X축 제한)", modelName.c_str());
            break;

        case Y_AXIS:
            axisName = "Y축";
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "이동 중: %s (Y축 제한)", modelName.c_str());
            break;

        case Z_AXIS:
            axisName = "Z축";
            ImGui::TextColored(ImVec4(0.3f, 0.3f, 1.0f, 1.0f), "이동 중: %s (Z축 제한)", modelName.c_str());
            break;

        case FREE:
        default:
            axisName = "자유";
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "이동 중: %s (자유 이동)", modelName.c_str());
            break;
        }
    }
}

void ModelManager::InitializeDummyCharacter(ID3D11Device* device) {
    if (!device) {
        OutputDebugStringA("InitializeDummyCharacter: device가 NULL입니다.\n");
        return;
    }

    try {
        dummyCharacter = std::make_shared<DummyCharacter>();
        if (!dummyCharacter) {
            OutputDebugStringA("DummyCharacter 생성 실패\n");
            return;
        }

        if (!dummyCharacter->Initialize(device)) {
            OutputDebugStringA("DummyCharacter 초기화 실패\n");
            return;
        }

        // 방 중앙에 위치
        dummyCharacter->SetPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));
        dummyCharacter->SetRotation(0.0f);

        // 카메라에 캐릭터 참조 전달
        camera.SetCharacter(dummyCharacter.get());

        // 기본 모드는 자유 시점
        isFirstPersonMode = false;
        camera.SetCameraMode(FREE_CAMERA);

        OutputDebugStringA("DummyCharacter 초기화 성공\n");
    }
    catch (std::exception& e) {
        OutputDebugStringA(("DummyCharacter 초기화 예외: " + std::string(e.what()) + "\n").c_str());
    }
    catch (...) {
        OutputDebugStringA("DummyCharacter 초기화 중 알 수 없는 예외 발생\n");
    }
}


void ModelManager::ToggleCameraMode() {
    // dummyCharacter 유효성 검사
    if (!dummyCharacter) {
        OutputDebugStringA("ToggleCameraMode: dummyCharacter가 초기화되지 않았습니다.\n");
        return;
    }

    isFirstPersonMode = !isFirstPersonMode;

    if (isFirstPersonMode) {
        // 1인칭 모드로 전환
        camera.SetCameraMode(FIRST_PERSON);

        // 안전 확인
        camera.SetCharacter(dummyCharacter.get());

        // 디버그 메시지
        OutputDebugStringA("1인칭 시점으로 전환합니다.\n");
        OutputDebugStringA(("캐릭터 위치: " +
            std::to_string(dummyCharacter->GetPosition().x) + ", " +
            std::to_string(dummyCharacter->GetPosition().y) + ", " +
            std::to_string(dummyCharacter->GetPosition().z) + "\n").c_str());

        try {
            camera.UpdateFirstPersonView();
        }
        catch (...) {
            OutputDebugStringA("카메라 업데이트 중 오류 발생\n");
        }
    }
    else {
        // 자유 시점 모드로 전환
        camera.SetCameraMode(FREE_CAMERA);
        // 자유 시점에서의 카메라 초기 위치/회전 설정
        camera.SetPosition(0.0f, 2.0f, -5.0f);

        OutputDebugStringA("자유 시점으로 전환합니다.\n");
    }
}
