#include "WindowManager.h"
#include "Task.h"
#include <commctrl.h>
#include <shellapi.h>

// 字符串转换工具函数
std::wstring utf8ToWide(const std::string& utf8Str) {
    if (utf8Str.empty()) return L"";
    
    int wideLength = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, nullptr, 0);
    if (wideLength <= 0) return L"";
    
    std::wstring wideStr(wideLength, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wideStr[0], wideLength);
    
    // 移除空字符
    if (!wideStr.empty() && wideStr.back() == '\0') {
        wideStr.pop_back();
    }
    
    return wideStr;
}

WindowManager* WindowManager::instance = nullptr;

WindowManager::WindowManager() {
    instance = this;
    hFont = nullptr;
}

WindowManager::~WindowManager() {
    instance = nullptr;
    if (hFont) DeleteObject(hFont);
}

bool WindowManager::createWindow() {
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    
    // Register window class
    const wchar_t CLASS_NAME[] = L"SchedulerWindowClass";
    
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
    RegisterClass(&wc);
    
    // Create window
    hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        CLASS_NAME,
        L"Scheduler",
        WS_POPUP | WS_BORDER,
        GetSystemMetrics(SM_CXSCREEN) - WINDOW_WIDTH, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
        nullptr, nullptr, hInstance, nullptr
    );
    
    if (!hwnd) return false;
    
    // Make window transparent and initially not topmost
    SetLayeredWindowAttributes(hwnd, 0, 220, LWA_ALPHA);
    SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    
    // Initialize drag state
    isDragging = false;
    isPinned = false;
    
    // 优化后的代码
    hFont = CreateFont(
        18,                    // 增加字体高度到18像素
        0,                     // 字体宽度（0表示自动）
        0,                     // 文本角度
        0,                     // 基线角度
        FW_NORMAL,             // 字体粗细
        FALSE,                 // 斜体
        FALSE,                 // 下划线
        FALSE,                 // 删除线
        DEFAULT_CHARSET,       // 字符集
        OUT_TT_PRECIS,         // 使用TrueType字体输出精度（提高质量）
        CLIP_DEFAULT_PRECIS,   // 裁剪精度
        CLEARTYPE_QUALITY,     // 使用ClearType渲染（提高清晰度）
        DEFAULT_PITCH | FF_DONTCARE, // 字体间距和族
        L"Microsoft YaHei UI"  // 使用微软雅黑UI字体（中文显示更好）
    );

    // 更健壮的字体创建
    if (!hFont) {
        hFont = CreateFont(
            18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI"
        );
    }

    createControls();
    
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    
    return true;
}

void WindowManager::createControls() {
    // Top navigation buttons
    btnActiveTasks = CreateWindow(L"BUTTON", L"活跃任务", 
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        MARGIN, MARGIN, 80, 25, hwnd, (HMENU)1, GetModuleHandle(nullptr), nullptr);
    
    btnHistory = CreateWindow(L"BUTTON", L"历史记录",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        95, MARGIN, 80, 25, hwnd, (HMENU)2, GetModuleHandle(nullptr), nullptr);
    
    // Mode selection buttons
    btnEarly = CreateWindow(L"BUTTON", L"EARLY",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        MARGIN, 35, 60, 25, hwnd, (HMENU)3, GetModuleHandle(nullptr), nullptr);
    
    btnDDL = CreateWindow(L"BUTTON", L"DDL",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        75, 35, 60, 25, hwnd, (HMENU)4, GetModuleHandle(nullptr), nullptr);
    
    btnCompleted = CreateWindow(L"BUTTON", L"完成",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        MARGIN, 35, 60, 25, hwnd, (HMENU)5, GetModuleHandle(nullptr), nullptr);
    
    btnTerminated = CreateWindow(L"BUTTON", L"终止",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        75, 35, 60, 25, hwnd, (HMENU)6, GetModuleHandle(nullptr), nullptr);
    
    // Create task button
    btnCreateTask = CreateWindow(L"BUTTON", L"创建任务",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        MARGIN, 65, 80, 25, hwnd, (HMENU)7, GetModuleHandle(nullptr), nullptr);
    
    // Close button
    btnClose = CreateWindow(L"BUTTON", L"✕",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        WINDOW_WIDTH - 35, 5, 25, 25, hwnd, (HMENU)16, GetModuleHandle(nullptr), nullptr);
    
    // Pin button (moved left to avoid overlap)
    btnPin = CreateWindow(L"BUTTON", L"📌",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        WINDOW_WIDTH - 70, 5, 25, 25, hwnd, (HMENU)14, GetModuleHandle(nullptr), nullptr);
    
    // Pagination buttons
    btnPrevPage = CreateWindow(L"BUTTON", L"上一页",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        MARGIN, WINDOW_HEIGHT - 40, 60, 25, hwnd, (HMENU)8, GetModuleHandle(nullptr), nullptr);
    
    btnNextPage = CreateWindow(L"BUTTON", L"下一页",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        75, WINDOW_HEIGHT - 40, 60, 25, hwnd, (HMENU)9, GetModuleHandle(nullptr), nullptr);
    
    btnPageNumbers = CreateWindow(L"BUTTON", L"1",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        145, WINDOW_HEIGHT - 40, 40, 25, hwnd, (HMENU)10, GetModuleHandle(nullptr), nullptr);
    
    // Settings button
    btnSettings = CreateWindow(L"BUTTON", L"设置",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        WINDOW_WIDTH - 60, WINDOW_HEIGHT - 40, 50, 25, hwnd, (HMENU)11, GetModuleHandle(nullptr), nullptr);
    
    // Edit controls (initially hidden)
    hwndTaskName = CreateWindow(L"EDIT", L"",
        WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        MARGIN, 100, WINDOW_WIDTH - 2*MARGIN, 25, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
    
    hwndStartTime = CreateWindow(L"EDIT", L"",
        WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        MARGIN, 130, (WINDOW_WIDTH - 3*MARGIN)/2, 25, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
    
    hwndEndTime = CreateWindow(L"EDIT", L"",
        WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        MARGIN + (WINDOW_WIDTH - 3*MARGIN)/2 + MARGIN, 130, (WINDOW_WIDTH - 3*MARGIN)/2, 25, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
    
    hwndConfirm = CreateWindow(L"BUTTON", L"确认",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        MARGIN, 165, 60, 25, hwnd, (HMENU)12, GetModuleHandle(nullptr), nullptr);
    
    hwndCancel = CreateWindow(L"BUTTON", L"取消",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        75, 165, 60, 25, hwnd, (HMENU)13, GetModuleHandle(nullptr), nullptr);
    
    // Set font for all controls
    SendMessage(btnActiveTasks, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(btnHistory, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(btnEarly, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(btnDDL, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(btnCompleted, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(btnTerminated, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(btnCreateTask, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(btnPin, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(btnClose, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(btnPrevPage, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(btnNextPage, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(btnPageNumbers, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(btnSettings, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hwndTaskName, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hwndStartTime, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hwndEndTime, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hwndConfirm, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hwndCancel, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    updateLayout();
}

void WindowManager::updateLayout() {
    auto& settings = dataManager.getUserSettings();
    bool isActiveMode = settings.mainMode == 0;
    
    // Show/hide mode buttons based on current mode
    ShowWindow(btnEarly, isActiveMode ? SW_SHOW : SW_HIDE);
    ShowWindow(btnDDL, isActiveMode ? SW_SHOW : SW_HIDE);
    ShowWindow(btnCompleted, !isActiveMode ? SW_SHOW : SW_HIDE);
    ShowWindow(btnTerminated, !isActiveMode ? SW_SHOW : SW_HIDE);
    
    // Hide create task button in history mode
    ShowWindow(btnCreateTask, isActiveMode ? SW_SHOW : SW_HIDE);
    
    // Update pin button text
    SetWindowText(btnPin, isPinned ? L"📍" : L"📌");
    
    // Update page buttons
    updatePageButtons();
    
    // Show/hide edit controls
    ShowWindow(hwndTaskName, isCreatingTask || isEditingTask ? SW_SHOW : SW_HIDE);
    ShowWindow(hwndStartTime, isCreatingTask || isEditingTask ? SW_SHOW : SW_HIDE);
    ShowWindow(hwndEndTime, isCreatingTask || isEditingTask ? SW_SHOW : SW_HIDE);
    ShowWindow(hwndConfirm, isCreatingTask || isEditingTask ? SW_SHOW : SW_HIDE);
    ShowWindow(hwndCancel, isCreatingTask || isEditingTask ? SW_SHOW : SW_HIDE);
}

LRESULT CALLBACK WindowManager::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (!instance) return DefWindowProc(hwnd, uMsg, wParam, lParam);
    
    switch (uMsg) {
        case WM_COMMAND:
            instance->handleCommand(wParam, lParam);
            break;
        case WM_PAINT:
            instance->handlePaint();
            break;
        case WM_MOUSEMOVE:
            instance->handleMouseMove(wParam, lParam);
            break;
        case WM_LBUTTONDOWN:
            instance->handleLButtonDown(wParam, lParam);
            break;
        case WM_LBUTTONUP:
            instance->handleLButtonUp(wParam, lParam);
            break;
        case WM_RBUTTONDOWN:
            instance->handleRButtonDown(wParam, lParam);
            break;
        case WM_DESTROY:
            // 在窗口销毁前保存数据
            instance->dataManager.saveUserSettings();
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void WindowManager::handleCommand(WPARAM wParam, LPARAM lParam) {
    int id = LOWORD(wParam);
    
    switch (id) {
        case 1: // Active Tasks
            dataManager.getUserSettings().mainMode = 0;
            currentPage = 0;
            updateLayout();
            refreshDisplay();
            break;
        case 2: // History
            dataManager.getUserSettings().mainMode = 1;
            currentPage = 0;
            updateLayout();
            refreshDisplay();
            break;
        case 3: // EARLY
            dataManager.getUserSettings().activeTaskMode = 0;
            dataManager.saveUserSettings();
            refreshDisplay();
            break;
        case 4: // DDL
            dataManager.getUserSettings().activeTaskMode = 1;
            dataManager.saveUserSettings();
            refreshDisplay();
            break;
        case 5: // Completed
            dataManager.getUserSettings().historyMode = 0;
            dataManager.saveUserSettings();
            refreshDisplay();
            break;
        case 6: // Terminated
            dataManager.getUserSettings().historyMode = 1;
            dataManager.saveUserSettings();
            refreshDisplay();
            break;
        case 7: // Create Task
            {
                isCreatingTask = true;
                isEditingTask = false;
                
                // Set default values from user settings
                auto& settings = dataManager.getUserSettings();
                SetWindowText(hwndTaskName, L"");
                SetWindowText(hwndStartTime, Task::formatDateTime(settings.lastStartTime).c_str());
                SetWindowText(hwndEndTime, Task::formatDateTime(settings.lastEndTime).c_str());
                
                updateLayout();
            }
            break;
        case 8: // Previous Page
            if (currentPage > 0) {
                currentPage--;
                refreshDisplay();
            }
            break;
        case 9: // Next Page
            if (currentPage < getTotalPages() - 1) {
                currentPage++;
                refreshDisplay();
            }
            break;
        case 10: // Page Numbers
            // TODO: Show page selection dialog
            break;
        case 11: // Settings
            showSettingsDialog();
            break;
        case 12: // Confirm
            if (isCreatingTask || isEditingTask) {
                wchar_t nameWide[256], startTime[64], endTime[64];
                GetWindowText(hwndTaskName, nameWide, 256);
                GetWindowText(hwndStartTime, startTime, 64);
                GetWindowText(hwndEndTime, endTime, 64);
                
                if (wcslen(nameWide) > 0) {
                    try {
                        // 将宽字符串转换为UTF-8
                        int nameLen = WideCharToMultiByte(CP_UTF8, 0, nameWide, -1, nullptr, 0, nullptr, nullptr);
                        std::string nameStr(nameLen - 1, 0);
                        WideCharToMultiByte(CP_UTF8, 0, nameWide, -1, &nameStr[0], nameLen, nullptr, nullptr);
                        
                        // 将宽字符串时间转换为UTF-8
                        int startLen = WideCharToMultiByte(CP_UTF8, 0, startTime, -1, nullptr, 0, nullptr, nullptr);
                        std::string startTimeStr(startLen - 1, 0);
                        WideCharToMultiByte(CP_UTF8, 0, startTime, -1, &startTimeStr[0], startLen, nullptr, nullptr);
                        
                        int endLen = WideCharToMultiByte(CP_UTF8, 0, endTime, -1, nullptr, 0, nullptr, nullptr);
                        std::string endTimeStr(endLen - 1, 0);
                        WideCharToMultiByte(CP_UTF8, 0, endTime, -1, &endTimeStr[0], endLen, nullptr, nullptr);
                        
                        auto start = Task::parseDateTime(startTime);
                        auto end = Task::parseDateTime(endTime);
                        
                        if (isCreatingTask) {
                            dataManager.createTask(nameStr, start, end);
                        } else {
                            dataManager.updateTask(editingTaskId, nameStr, start, end);
                        }
                        
                        // Update user settings
                        auto& settings = dataManager.getUserSettings();
                        settings.lastStartTime = start;
                        settings.lastEndTime = end;
                        dataManager.saveUserSettings();
                        
                        isCreatingTask = false;
                        isEditingTask = false;
                        editingTaskId = -1;
                        refreshDisplay();
                    } catch (...) {
                        MessageBox(hwnd, L"Invalid date/time format", L"Error", MB_OK);
                    }
                }
            }
            break;
        case 13: // Cancel
            isCreatingTask = false;
            isEditingTask = false;
            editingTaskId = -1;
            updateLayout();
            refreshDisplay();
            break;
        case 14: // Pin/Unpin
            handlePinToggle();
            break;
        case 16: // Close button
            PostQuitMessage(0);
            break;
    }
}

void WindowManager::handlePaint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    
    // Fill background
    RECT rect;
    GetClientRect(hwnd, &rect);
    HBRUSH hBrush = CreateSolidBrush(dataManager.getUserSettings().windowColor);
    FillRect(hdc, &rect, hBrush);
    DeleteObject(hBrush);
    
    // Draw tasks
    drawTasks(hdc);
    
    // Draw edit controls if visible
    if (isCreatingTask || isEditingTask) {
        drawEditControls(hdc);
    }
    
    EndPaint(hwnd, &ps);
}

void WindowManager::drawTasks(HDC hdc) {
    auto& settings = dataManager.getUserSettings();
    bool isActiveMode = settings.mainMode == 0;
    
    std::vector<Task> tasks = dataManager.getTasksByPage(
        currentPage, 10, isActiveMode, 
        isActiveMode ? (settings.activeTaskMode == 0) : true
    );
    
    int yStart = 100;
    if (isCreatingTask || isEditingTask) {
        yStart = 200;
    }
    
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, dataManager.getUserSettings().textColor);
    
    for (size_t i = 0; i < tasks.size(); ++i) {
        const Task& task = tasks[i];
        int yPos = yStart + i * TASK_HEIGHT;
        
        drawTaskRow(hdc, task, yPos);
    }
}

void WindowManager::drawTaskRow(HDC hdc, const Task& task, int yPos) {
    auto& settings = dataManager.getUserSettings();
    bool isActiveMode = settings.mainMode == 0;
    
    // Task name
    RECT nameRect = {MARGIN, yPos, WINDOW_WIDTH - MARGIN, yPos + 20};
    std::wstring nameWide = utf8ToWide(task.name);
    DrawText(hdc, nameWide.c_str(), -1, &nameRect, DT_LEFT | DT_VCENTER);
    
    // Start/end time
    std::wstring timeText = Task::formatDateTime(task.startTime) + L" - " + Task::formatDateTime(task.endTime);
    RECT timeRect = {MARGIN, yPos + 20, WINDOW_WIDTH - MARGIN, yPos + 40};
    DrawText(hdc, timeText.c_str(), -1, &timeRect, DT_LEFT | DT_VCENTER);
    
    if (isActiveMode) {
        // Complete and Terminate buttons
        RECT completeRect = {WINDOW_WIDTH - 120, yPos, WINDOW_WIDTH - 70, yPos + 20};
        RECT terminateRect = {WINDOW_WIDTH - 60, yPos, WINDOW_WIDTH - 10, yPos + 20};
        
        DrawText(hdc, L"完成", -1, &completeRect, DT_CENTER | DT_VCENTER);
        DrawText(hdc, L"终止", -1, &terminateRect, DT_CENTER | DT_VCENTER);
    }
}

void WindowManager::drawEditControls(HDC hdc) {
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, dataManager.getUserSettings().textColor);
    
    RECT labelRect = {MARGIN, 100, 100, 120};
    DrawText(hdc, L"任务名称:", -1, &labelRect, DT_LEFT | DT_VCENTER);
    
    labelRect = {MARGIN, 130, 100, 150};
    DrawText(hdc, L"开始时间:", -1, &labelRect, DT_LEFT | DT_VCENTER);
    
    labelRect = {MARGIN + (WINDOW_WIDTH - 3*MARGIN)/2 + MARGIN, 130, 
                 MARGIN + (WINDOW_WIDTH - 3*MARGIN)/2 + MARGIN + 100, 150};
    DrawText(hdc, L"结束时间:", -1, &labelRect, DT_LEFT | DT_VCENTER);
}

void WindowManager::handleLButtonDown(WPARAM wParam, LPARAM lParam) {
    int x = LOWORD(lParam);
    int y = HIWORD(lParam);
    
    // Check if click is in the top title bar area (for dragging)
    if (y <= 35 && x < WINDOW_WIDTH - 35) {  // Top 35 pixels, excluding close button area
        handleMouseDown(wParam, lParam);
        return;
    }
    
    // Handle button clicks
    auto& settings = dataManager.getUserSettings();
    bool isActiveMode = settings.mainMode == 0;
    
    if (isActiveMode) {
        std::vector<Task> tasks = dataManager.getTasksByPage(
            currentPage, 10, true, settings.activeTaskMode == 0
        );
        
        int yStart = 100;
        if (isCreatingTask || isEditingTask) {
            yStart = 200;
        }
        
        for (size_t i = 0; i < tasks.size(); ++i) {
            int yPos = yStart + i * TASK_HEIGHT;
            
            // Check Complete button
            RECT completeRect = {WINDOW_WIDTH - 120, yPos, WINDOW_WIDTH - 70, yPos + 20};
            if (x >= completeRect.left && x <= completeRect.right &&
                y >= completeRect.top && y <= completeRect.bottom) {
                dataManager.changeTaskStatus(tasks[i].id, TaskStatus::COMPLETED);
                refreshDisplay();
                return;
            }
            
            // Check Terminate button
            RECT terminateRect = {WINDOW_WIDTH - 60, yPos, WINDOW_WIDTH - 10, yPos + 20};
            if (x >= terminateRect.left && x <= terminateRect.right &&
                y >= terminateRect.top && y <= terminateRect.bottom) {
                dataManager.changeTaskStatus(tasks[i].id, TaskStatus::TERMINATED);
                refreshDisplay();
                return;
            }
        }
    }
}

void WindowManager::handleRButtonDown(WPARAM wParam, LPARAM lParam) {
    int x = LOWORD(lParam);
    int y = HIWORD(lParam);
    
    auto& settings = dataManager.getUserSettings();
    bool isActiveMode = settings.mainMode == 0;
    
    std::vector<Task> tasks = dataManager.getTasksByPage(
        currentPage, 10, isActiveMode, isActiveMode ? (settings.activeTaskMode == 0) : true
    );
    
    int yStart = 100;
    if (isCreatingTask || isEditingTask) {
        yStart = 200;
    }
    
    for (size_t i = 0; i < tasks.size(); ++i) {
        int yPos = yStart + i * TASK_HEIGHT;
        RECT taskRect = {MARGIN, yPos, WINDOW_WIDTH - MARGIN, yPos + TASK_HEIGHT};
        
        if (x >= taskRect.left && x <= taskRect.right &&
            y >= taskRect.top && y <= taskRect.bottom) {
            
            if (isActiveMode) {
                showTaskContextMenu(tasks[i].id, x, y);
            } else {
                showHistoryContextMenu(tasks[i].id, x, y);
            }
            break;
        }
    }
}

void WindowManager::showTaskContextMenu(int taskId, int x, int y) {
    HMENU hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, 1, L"编辑");
    
    POINT pt = {x, y};
    ClientToScreen(hwnd, &pt);
    
    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, nullptr);
    
    if (cmd == 1) {
        // Edit task
        auto tasks = dataManager.getActiveTasks();
        for (const auto& task : tasks) {
            if (task.id == taskId) {
                isEditingTask = true;
                isCreatingTask = false;
                editingTaskId = taskId;
                
                SetWindowText(hwndTaskName, utf8ToWide(task.name).c_str());
                SetWindowText(hwndStartTime, Task::formatDateTime(task.startTime).c_str());
                SetWindowText(hwndEndTime, Task::formatDateTime(task.endTime).c_str());
                
                updateLayout();
                break;
            }
        }
    }
    
    DestroyMenu(hMenu);
}

void WindowManager::showHistoryContextMenu(int taskId, int x, int y) {
    HMENU hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, 1, L"恢复");
    AppendMenu(hMenu, MF_STRING, 2, L"完成");
    AppendMenu(hMenu, MF_STRING, 3, L"终止");
    AppendMenu(hMenu, MF_STRING, 4, L"删除");
    
    POINT pt = {x, y};
    ClientToScreen(hwnd, &pt);
    
    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, nullptr);
    
    switch (cmd) {
        case 1: // Restore
            dataManager.changeTaskStatus(taskId, TaskStatus::ACTIVE);
            break;
        case 2: // Complete
            dataManager.changeTaskStatus(taskId, TaskStatus::COMPLETED);
            break;
        case 3: // Terminate
            dataManager.changeTaskStatus(taskId, TaskStatus::TERMINATED);
            break;
        case 4: // Delete
            if (MessageBox(hwnd, L"确认删除此任务？", L"确认删除", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                dataManager.deleteTask(taskId);
            }
            break;
    }
    
    refreshDisplay();
    DestroyMenu(hMenu);
}

void WindowManager::updatePageButtons() {
    int totalPages = getTotalPages();
    
    EnableWindow(btnPrevPage, currentPage > 0);
    EnableWindow(btnNextPage, currentPage < totalPages - 1);
    
    std::wstring pageText = std::to_wstring(currentPage + 1) + L" / " + std::to_wstring(totalPages);
    SetWindowText(btnPageNumbers, pageText.c_str());
}

int WindowManager::getTotalPages() {
    auto& settings = dataManager.getUserSettings();
    bool isActiveMode = settings.mainMode == 0;
    
    std::vector<Task> tasks;
    if (isActiveMode) {
        tasks = dataManager.getActiveTasks();
    } else {
        if (settings.historyMode == 0) {
            tasks = dataManager.getCompletedTasks();
        } else {
            tasks = dataManager.getTerminatedTasks();
        }
    }
    
    return (tasks.size() + 9) / 10; // Ceiling division
}

void WindowManager::refreshDisplay() {
    updateLayout();
    InvalidateRect(hwnd, nullptr, TRUE);
}

void WindowManager::showSettingsDialog() {
    // Simple color picker dialog
    CHOOSECOLOR cc = {sizeof(CHOOSECOLOR)};
    static COLORREF crCust[16];
    
    cc.hwndOwner = hwnd;
    cc.lpCustColors = crCust;
    cc.rgbResult = dataManager.getUserSettings().windowColor;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
    
    if (ChooseColor(&cc)) {
        dataManager.getUserSettings().windowColor = cc.rgbResult;
        dataManager.saveUserSettings();
        refreshDisplay();
    }
}

void WindowManager::runMessageLoop() {
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void WindowManager::handlePinToggle() {
    isPinned = !isPinned;
    
    if (isPinned) {
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    } else {
        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    
    updateLayout();
}

void WindowManager::handleLButtonUp(WPARAM wParam, LPARAM lParam) {
    if (isDragging) {
        isDragging = false;
        ReleaseCapture();
    }
}

void WindowManager::handleMouseDown(WPARAM wParam, LPARAM lParam) {
    // This is called for drag handle
    isDragging = true;
    SetCapture(hwnd);
    
    POINT pt = {LOWORD(lParam), HIWORD(lParam)};
    ClientToScreen(hwnd, &pt);
    dragStart = pt;
    
    // Get current window position
    RECT windowRect;
    GetWindowRect(hwnd, &windowRect);
    dragStart.x -= windowRect.left;
    dragStart.y -= windowRect.top;
}

void WindowManager::handleMouseMove(WPARAM wParam, LPARAM lParam) {
    if (isDragging) {
        POINT pt = {LOWORD(lParam), HIWORD(lParam)};
        ClientToScreen(hwnd, &pt);
        
        // Move window
        SetWindowPos(hwnd, nullptr, 
                    pt.x - dragStart.x, pt.y - dragStart.y, 
                    0, 0, SWP_NOSIZE | SWP_NOZORDER);
    } else {
        // Handle hover effects
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        
        // Check if mouse is in top title bar area
        bool isInTopArea = (y <= 35 && x < WINDOW_WIDTH - 35);  // Top 35 pixels, excluding close button area
        
        if (isInTopArea) {
            // 鼠标在顶部区域，显示移动光标
            SetCursor(LoadCursor(nullptr, IDC_SIZEALL));
        } else {
            // 鼠标离开顶部区域，恢复默认光标
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
        }
    }
}
