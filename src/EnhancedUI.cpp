#include "EnhancedUI.h"


const char* EnhancedUI::ICON_ADD = "+";
const char* EnhancedUI::ICON_REMOVE = "-";
const char* EnhancedUI::ICON_OBJ = "O";
const char* EnhancedUI::ICON_GLB = "G";
const char* EnhancedUI::ICON_SETTINGS = "*";
const char* EnhancedUI::ICON_CAMERA = "C";
const char* EnhancedUI::ICON_ROOM = "R";
const char* EnhancedUI::ICON_MATERIAL = "M";
const char* EnhancedUI::ICON_MODEL = "3D";
const char* EnhancedUI::ICON_IMPORT = "I";
const char* EnhancedUI::ICON_EXPORT = "E";

// UI 색상 테마 설정
void EnhancedUI::SetupTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // 어두운 테마 기반의 현대적 스타일
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.32f, 0.68f, 0.96f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.32f, 0.68f, 0.96f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.46f, 0.76f, 0.98f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.32f, 0.68f, 0.96f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.46f, 0.76f, 0.98f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.32f, 0.68f, 0.96f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.46f, 0.76f, 0.98f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.32f, 0.68f, 0.96f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.28f, 0.56f, 0.90f, 1.00f);

    // 스타일 조정
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.WindowRounding = 6.0f;
    style.ChildRounding = 6.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.TabRounding = 4.0f;
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.ItemSpacing = ImVec2(8.0f, 4.0f);
}

// 사용자 정의 UI 헤더 (섹션 제목) 렌더링
void EnhancedUI::RenderHeader(const char* title) {
    ImGui::Spacing();
    ImGui::SeparatorText(title);
    ImGui::Spacing();
}

// 향상된 슬라이더 (라벨, 값, 최소값, 최대값, 힌트 제공)
bool EnhancedUI::SliderFloat(const char* label, float* value, float min, float max, const char* hint) {
    // 슬라이더 너비 조정
    float width = ImGui::GetContentRegionAvail().x;

    ImGui::PushItemWidth(width * 0.7f);
    bool changed = ImGui::SliderFloat(label, value, min, max, "%.2f");
    ImGui::PopItemWidth();

    // 힌트 표시 (있는 경우)
    if (hint && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", hint);
    }

    return changed;
}

// 색상 선택 위젯 (향상된 UI로)
bool EnhancedUI::ColorEdit(const char* label, float* color, const char* hint) {
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
    bool changed = ImGui::ColorEdit3(label, color, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB);
    ImGui::PopItemWidth();

    if (hint && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", hint);
    }

    return changed;
}

// 모델 리스트 아이템 (아이콘 포함)
bool EnhancedUI::ModelListItem(const std::string& name, bool selected, int type) {
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.22f, 0.22f, 0.22f, 0.55f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.26f, 0.59f, 0.98f, 0.80f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.26f, 0.59f, 0.98f, 1.00f));

    // 모델 타입에 따라 아이콘 텍스트 선택
    const char* icon = (type == 1) ? ICON_GLB : ICON_OBJ;  // MODEL_GLB = 1, MODEL_OBJ = 0
    std::string displayName = std::string(icon) + " " + name;

    bool isSelected = ImGui::Selectable(displayName.c_str(), selected);

    ImGui::PopStyleColor(3);
    return isSelected;
}

// 커스텀 버튼 (아이콘과 텍스트 포함)
bool EnhancedUI::IconButton(const char* label, const char* icon, const ImVec2& size) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.22f, 0.22f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.32f, 0.68f, 0.96f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.46f, 0.76f, 0.98f, 1.00f));

    std::string buttonLabel = std::string(icon) + " " + label;
    bool result = ImGui::Button(buttonLabel.c_str(), size);

    ImGui::PopStyleColor(3);
    return result;
}

// 커스텀 탭 렌더링
bool EnhancedUI::BeginTabBar(const char* str_id, ImGuiTabBarFlags flags) {
    ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.17f, 0.17f, 0.17f, 0.86f));
    ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.26f, 0.59f, 0.98f, 0.80f));
    ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.26f, 0.59f, 0.98f, 1.00f));
    bool result = ImGui::BeginTabBar(str_id, flags);
    return result;
}

void EnhancedUI::EndTabBar() {
    ImGui::EndTabBar();
    ImGui::PopStyleColor(3);
}
