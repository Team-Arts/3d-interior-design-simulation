#pragma once

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <iostream>
#include <vector>
#include <windows.h>

// ���ø����̼��� ��� ���� ����� ����ϴ� �߻� ��� Ŭ����
namespace visualnnz
{
class AppBase
{
public:
    AppBase();
    virtual ~AppBase();

    // ���� ���� ����
    int Run();

    // Win32 ������ ����
    virtual bool Initialize(HINSTANCE hInstance, int nCmdShow);

    // Win32 ������ �ݱ�
    virtual void Shutdown();

    // �ڽ� Ŭ�������� �������� ���� ���� �Լ�
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;

    // �ٸ� ��⿡�� ������ �ڵ��� ����� �� �ֵ��� �ϴ� getter
    HWND GetWindowHandle() const;

protected:
    // Win32 �޽��� ó�� �Լ�
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    HWND m_mainWindow;
    HINSTANCE m_hInstance;

private:
    int m_screenWidth;
    int m_screenHeight;
    // int m_guiWidth;
};
} // namespace visualnnz
