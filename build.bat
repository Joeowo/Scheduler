a@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cl /EHsc /std:c++17 /DUNICODE /D_UNICODE /Fe:Scheduler.exe src\main.cpp src\Task.cpp src\DataManager.cpp src\WindowManager.cpp /I src /link user32.lib gdi32.lib comdlg32.lib comctl32.lib shell32.lib ole32.lib oleaut32.lib /SUBSYSTEM:WINDOWS
if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)
echo Build completed successfully!
Scheduler.exe