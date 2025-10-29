#pragma once
#include "imgui.h"
#include "imgui_internal.h"
#include <string>

// 향상된 ImGui UI 스타일 및 레이아웃을 위한 헬퍼 클래스
class EnhancedUI {
public:
    // UI 색상 테마 설정
    static void SetupTheme();

    // 사용자 정의 UI 헤더 (섹션 제목) 렌더링
    static void RenderHeader(const char* title);

    // 향상된 슬라이더 (라벨, 값, 최소값, 최대값, 힌트 제공)
    static bool SliderFloat(const char* label, float* value, float min, float max, const char* hint = nullptr);

    // 색상 선택 위젯 (향상된 UI로)
    static bool ColorEdit(const char* label, float* color, const char* hint = nullptr);

    // 모델 리스트 아이템 (아이콘 포함)
    static bool ModelListItem(const std::string& name, bool selected, int type);

    // 커스텀 버튼 (아이콘과 텍스트 포함)
    static bool IconButton(const char* label, const char* icon, const ImVec2& size = ImVec2(0, 0));

    // 커스텀 탭 렌더링
    static bool BeginTabBar(const char* str_id, ImGuiTabBarFlags flags = 0);
    static void EndTabBar();

    // 아이콘 정의 (폰트 어썸 스타일)
    static const char* ICON_ADD;
    static const char* ICON_REMOVE;
    static const char* ICON_OBJ;
    static const char* ICON_GLB;
    static const char* ICON_SETTINGS;
    static const char* ICON_CAMERA;
    static const char* ICON_ROOM;
    static const char* ICON_MATERIAL;
    static const char* ICON_MODEL;
    static const char* ICON_IMPORT;
    static const char* ICON_EXPORT;
};
