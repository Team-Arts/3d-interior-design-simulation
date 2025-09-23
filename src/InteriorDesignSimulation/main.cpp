#include <iostream>
#include "Core/MainApp.h"

int main(int argc, char* argv[])
{
    visualnnz::MainApp mainApp;

    // GetModuleHandle(NULL)�� ���� ���μ����� HINSTANCE�� ��ȯ
    if (!mainApp.Initialize(GetModuleHandle(NULL), SW_SHOW))
    {
        std::cout << "Application initialization failed!" << std::endl;
        return -1;
    }

    // ������ �ܼ� ��� ����
    std::cout << "Application initialized successfully!" << std::endl;

    return mainApp.Run();
}
