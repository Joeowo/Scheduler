#pragma once

#include "Task.h"
#include <vector>
#include <map>
#include <fstream>
#include <regex>

class DataManager {
private:
    static constexpr const char* DATA_FILE = "scheduler_data.json";
    
    std::vector<Task> tasks;
    std::vector<int> activeLog;
    std::vector<LogEntry> inactiveLog;
    UserSettings userSettings;
    int nextTaskId = 1;

    void saveToFile();
    void loadFromFile();

public:
    DataManager();
    ~DataManager();

    // Task management
    int createTask(const std::wstring& name, 
                   const std::chrono::system_clock::time_point& start,
                   const std::chrono::system_clock::time_point& end);
    bool updateTask(int taskId, const std::wstring& name,
                    const std::chrono::system_clock::time_point& start,
                    const std::chrono::system_clock::time_point& end);
    bool deleteTask(int taskId);
    bool changeTaskStatus(int taskId, TaskStatus newStatus);
    
    // Query methods
    std::vector<Task> getActiveTasks(bool sortByStartTime = true);
    std::vector<Task> getCompletedTasks();
    std::vector<Task> getTerminatedTasks();
    std::vector<Task> getTasksByPage(int page, int pageSize = 10, bool activeOnly = true, bool sortByStartTime = true);
    
    // Settings
    UserSettings& getUserSettings() { return userSettings; }
    void saveUserSettings();
    
    // Logs
    std::vector<LogEntry> getInactiveLog() const { return inactiveLog; }
};