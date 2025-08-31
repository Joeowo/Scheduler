#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <chrono>

enum class TaskStatus {
    ACTIVE = 1,
    COMPLETED = 2,
    TERMINATED = 3
};

struct Task {
    int id;
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
    std::wstring name;
    TaskStatus status;

    Task(const std::wstring& taskName, 
         const std::chrono::system_clock::time_point& start,
         const std::chrono::system_clock::time_point& end,
         int taskId = -1)
        : id(taskId), startTime(start), endTime(end), 
          name(taskName), status(TaskStatus::ACTIVE) {}

    static std::wstring formatDateTime(const std::chrono::system_clock::time_point& time);
    static std::chrono::system_clock::time_point parseDateTime(const std::wstring& dateStr);
};

struct UserSettings {
    std::chrono::system_clock::time_point lastStartTime;
    std::chrono::system_clock::time_point lastEndTime;
    int mainMode = 0; // 0 = active tasks, 1 = history
    int activeTaskMode = 0; // 0 = EARLY, 1 = DDL
    int historyMode = 0; // 0 = completed, 1 = terminated
    COLORREF windowColor = RGB(255, 255, 255);
    COLORREF textColor = RGB(0, 0, 0);
    std::wstring fontName = L"Segoe UI";
    int fontSize = 12;

    UserSettings() {
        auto now = std::chrono::system_clock::now();
        lastStartTime = now;
        lastEndTime = now + std::chrono::hours(24);
    }
};

struct LogEntry {
    int taskId;
    TaskStatus status;
    std::chrono::system_clock::time_point changeTime;

    LogEntry(int id, TaskStatus s, std::chrono::system_clock::time_point t)
        : taskId(id), status(s), changeTime(t) {}
};