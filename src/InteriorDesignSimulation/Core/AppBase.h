#pragma once

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <iostream>
#include <vector>
#include <windows.h>

// 애플리케이션의 모든 공통 기능을 담당하는 추상 기반 클래스
namespace visualnnz
{
class AppBase
{
public:
    AppBase();
    virtual ~AppBase();

    // 메인 루프 실행
    int Run();

    // Win32 윈도우 생성
    virtual bool Initialize(HINSTANCE hInstance, int nCmdShow);

    // Win32 윈도우 닫기
    virtual void Shutdown();

    // 자식 클래스에서 재정의할 순수 가상 함수
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;

    // 다른 모듈에서 윈도우 핸들을 사용할 수 있도록 하는 getter
    HWND GetWindowHandle() const;

protected:
    // Win32 메시지 처리 함수
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    HWND m_mainWindow;
    HINSTANCE m_hInstance;

private:
    int m_screenWidth;
    int m_screenHeight;
    // int m_guiWidth;
};
} // namespace visualnnz
