#pragma once

#include "DataManager.h"
#include <Windows.h>
#include <vector>
#include <functional>

class WindowManager {
private:
    HWND hwnd;
    DataManager dataManager;
    HFONT hFont;
    int currentPage = 0;
    bool isCreatingTask = false;
    bool isEditingTask = false;
    int editingTaskId = -1;

    // UI elements
    HWND btnActiveTasks;
    HWND btnHistory;
    HWND btnEarly;
    HWND btnDDL;
    HWND btnCompleted;
    HWND btnTerminated;
    HWND btnCreateTask;
    HWND btnPrevPage;
    HWND btnNextPage;
    HWND btnPageNumbers;
    HWND btnSettings;
    HWND btnPin;
    HWND btnClose;
    
    // Drag state
    bool isDragging;
    POINT dragStart;
    bool isPinned;

    // Edit controls for task creation/editing
    HWND hwndTaskName;
    HWND hwndStartTime;
    HWND hwndEndTime;
    HWND hwndConfirm;
    HWND hwndCancel;

    // Constants
    static constexpr int WINDOW_WIDTH = 300;
    static constexpr int WINDOW_HEIGHT = 450;
    static constexpr int TASK_HEIGHT = 60;
    static constexpr int MARGIN = 5;

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static WindowManager* instance;

    void createControls();
    void drawTasks(HDC hdc);
    void drawTaskRow(HDC hdc, const Task& task, int yPos);
    void drawEditControls(HDC hdc);
    void updateLayout();
    void handleCommand(WPARAM wParam, LPARAM lParam);
    void handlePaint();
    void handleMouseMove(WPARAM wParam, LPARAM lParam);
    void handleLButtonDown(WPARAM wParam, LPARAM lParam);
    void handleRButtonDown(WPARAM wParam, LPARAM lParam);
    void handleLButtonUp(WPARAM wParam, LPARAM lParam);
    void handleMouseDown(WPARAM wParam, LPARAM lParam);
    void handlePinToggle();
    
    void showTaskContextMenu(int taskId, int x, int y);
    void showHistoryContextMenu(int taskId, int x, int y);
    void updatePageButtons();
    int getTotalPages();

public:
    WindowManager();
    ~WindowManager();

    bool createWindow();
    void runMessageLoop();
    void refreshDisplay();
    void showSettingsDialog();
};