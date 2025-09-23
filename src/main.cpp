#include <iostream>
#include "Core/MainApp.h"

int main(int argc, char* argv[])
{
    visualnnz::MainApp mainApp;

    // GetModuleHandle(NULL)은 현재 프로세스의 HINSTANCE를 반환
    if (!mainApp.Initialize(GetModuleHandle(NULL), SW_SHOW))
    {
        std::cout << "Application initialization failed!" << std::endl;
        return -1;
    }

    // 디버깅용 콘솔 출력 예시
    std::cout << "Application initialized successfully!" << std::endl;

    return mainApp.Run();
}
