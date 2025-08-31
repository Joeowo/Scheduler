@echo off
echo Building Scheduler...

REM Check if Visual Studio is installed
where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo Visual Studio C++ compiler not found. Please run from Developer Command Prompt.
    pause
    exit /b 1
)

REM Create build directory
if not exist "build" mkdir build
cd build

REM Compile the project
cl /EHsc /std:c++17 /Fe:Scheduler.exe ..
    src\main.cpp ..
    src\Task.cpp ..
    src\DataManager.cpp ..
    src\WindowManager.cpp ..
    /I ..\src ..
    /link user32.lib gdi32.lib comdlg32.lib comctl32.lib shell32.lib

if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build completed successfully!
echo You can now run Scheduler.exe
cd ..
pause