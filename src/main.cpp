#include "WindowManager.h"
#include <Windows.h>
#include <commctrl.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // Initialize common controls
    INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX) };
    icex.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);
    
    // Create and run the application
    WindowManager* windowManager = new WindowManager();
    if (!windowManager->createWindow()) {
        MessageBox(nullptr, L"Failed to create window", L"Error", MB_OK);
        delete windowManager;
        return 1;
    }
    
    windowManager->runMessageLoop();
    
    // 确保数据保存
    delete windowManager;
    
    return 0;
}