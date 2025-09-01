#include "DataManager.h"
#include <algorithm>
#include <sstream>
#include <windows.h>

DataManager::DataManager() {
    loadFromFile();
}

DataManager::~DataManager() {
    saveToFile();
}

void DataManager::saveToFile() {
    std::ofstream file(DATA_FILE);
    if (!file.is_open()) return;
    
    file << "{\n";
    
    // Save tasks
    file << "  \"tasks\": [\n";
    for (size_t i = 0; i < tasks.size(); ++i) {
        const auto& task = tasks[i];
        file << "    {\n";
        file << "      \"id\": " << task.id << ",\n";
        // 直接使用UTF-8字符串，无需转换
        file << "      \"name\": \"" << task.name << "\",\n";
        
        auto startTime = std::chrono::system_clock::to_time_t(task.startTime);
        file << "      \"startTime\": " << startTime << ",\n";
        
        auto endTime = std::chrono::system_clock::to_time_t(task.endTime);
        file << "      \"endTime\": " << endTime << ",\n";
        
        file << "      \"status\": " << static_cast<int>(task.status) << "\n";
        file << "    " << (i == tasks.size() - 1 ? "}" : "},") << "\n";
    }
    file << "  ],\n";
    
    // Save active log
    file << "  \"activeLog\": [";
    for (size_t i = 0; i < activeLog.size(); ++i) {
        file << activeLog[i];
        if (i < activeLog.size() - 1) file << ",";
    }
    file << "],\n";
    
    // Save inactive log
    file << "  \"inactiveLog\": [\n";
    for (size_t i = 0; i < inactiveLog.size(); ++i) {
        const auto& entry = inactiveLog[i];
        file << "    {\n";
        file << "      \"taskId\": " << entry.taskId << ",\n";
        file << "      \"status\": " << static_cast<int>(entry.status) << ",\n";
        file << "      \"changeTime\": " << std::chrono::system_clock::to_time_t(entry.changeTime) << "\n";
        file << "    " << (i == inactiveLog.size() - 1 ? "}" : "},") << "\n";
    }
    file << "  ],\n";
    
    // Save user settings
    file << "  \"userSettings\": {\n";
    file << "    \"nextTaskId\": " << nextTaskId << ",\n";
    file << "    \"mainMode\": " << userSettings.mainMode << ",\n";
    file << "    \"activeTaskMode\": " << userSettings.activeTaskMode << ",\n";
    file << "    \"historyMode\": " << userSettings.historyMode << ",\n";
    file << "    \"windowColor\": " << userSettings.windowColor << ",\n";
    file << "    \"textColor\": " << userSettings.textColor << "\n";
    file << "  }\n";
    
    file << "}\n";
}

std::wstring parseStringValue(const std::wstring& line, const std::wstring& key) {
    std::wstring search = key + L"\"";
    size_t start = line.find(search);
    if (start == std::wstring::npos) return L"";
    
    start += search.length();
    size_t end = line.find(L"\"", start);
    if (end == std::wstring::npos) return L"";
    
    return line.substr(start, end - start);
}

int parseIntValue(const std::wstring& line, const std::wstring& key) {
    std::wstring search = key;
    size_t start = line.find(search);
    if (start == std::wstring::npos) return 0;
    
    start += search.length();
    size_t end = line.find_first_of(L",", start);
    if (end == std::wstring::npos) end = line.length();
    
    std::wstring value = line.substr(start, end - start);
    value.erase(0, value.find_first_not_of(L" \t"));
    
    try {
        return std::stoi(value);
    } catch (...) {
        return 0;
    }
}

std::string parseStringValue(const std::string& line, const std::string& key) {
    std::string search = key + "\"";
    size_t start = line.find(search);
    if (start == std::string::npos) return "";
    
    start += search.length();
    
    // 处理转义字符
    std::string result;
    bool escaped = false;
    for (size_t i = start; i < line.length(); ++i) {
        char c = line[i];
        if (!escaped && c == '\\') {
            escaped = true;
        } else if (!escaped && c == '"') {
            // 结束引号
            break;
        } else if (escaped) {
            switch (c) {
                case '\\': result += '\\'; break;
                case '"': result += '"'; break;
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case 'r': result += '\r'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                default: result += c; break;
            }
            escaped = false;
        } else {
            result += c;
        }
    }
    
    return result;
}

int parseIntValue(const std::string& line, const std::string& key) {
    std::string search = key;
    size_t start = line.find(search);
    if (start == std::string::npos) return 0;
    
    start += search.length();
    size_t end = line.find_first_of(",", start);
    if (end == std::string::npos) end = line.length();
    
    std::string value = line.substr(start, end - start);
    value.erase(0, value.find_first_not_of(" \t"));
    
    try {
        return std::stoi(value);
    } catch (...) {
        return 0;
    }
}

void DataManager::loadFromFile() {
    std::ifstream file(DATA_FILE);
    if (!file.is_open()) return;
    
    std::string line;
    bool inTasks = false;
    bool inTask = false;
    bool inActiveLog = false;
    bool inInactiveLog = false;
    bool inSettings = false;
    
    Task currentTask("", std::chrono::system_clock::now(), std::chrono::system_clock::now());
    LogEntry currentEntry(0, TaskStatus::ACTIVE, std::chrono::system_clock::now());
    
    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t"));
        
        if (line.find("\"tasks\":") != std::string::npos) {
            inTasks = true;
            inTask = false;
            inActiveLog = false;
            inInactiveLog = false;
            inSettings = false;
        } else if (line.find("\"activeLog\":") != std::string::npos) {
            inTasks = false;
            inActiveLog = true;
            inInactiveLog = false;
            inSettings = false;
        } else if (line.find("\"inactiveLog\":") != std::string::npos) {
            inTasks = false;
            inActiveLog = false;
            inInactiveLog = true;
            inSettings = false;
        } else if (line.find("\"userSettings\":") != std::string::npos) {
            inTasks = false;
            inActiveLog = false;
            inInactiveLog = false;
            inSettings = true;
        }
        
        if (inTasks) {
            if (line.find("{") != std::string::npos && line.find("\"id\":") != std::string::npos) {
                inTask = true;
                currentTask = Task("", std::chrono::system_clock::now(), std::chrono::system_clock::now());
            } else if (inTask) {
                if (line.find("\"id\":") != std::string::npos) {
                    currentTask.id = parseIntValue(line, "\"id\": ");
                } else if (line.find("\"name\":") != std::string::npos) {
                    std::string nameStr = parseStringValue(line, "\"name\": \"");
                    currentTask.name = nameStr;
                } else if (line.find("\"startTime\":") != std::string::npos) {
                    auto time = parseIntValue(line, "\"startTime\": ");
                    currentTask.startTime = std::chrono::system_clock::from_time_t(time);
                } else if (line.find("\"endTime\":") != std::string::npos) {
                    auto time = parseIntValue(line, "\"endTime\": ");
                    currentTask.endTime = std::chrono::system_clock::from_time_t(time);
                } else if (line.find("\"status\":") != std::string::npos) {
                    currentTask.status = static_cast<TaskStatus>(parseIntValue(line, "\"status\": "));
                } else if (line.find("}") != std::string::npos) {
                    tasks.push_back(currentTask);
                    nextTaskId = (std::max)(nextTaskId, currentTask.id + 1);
                    inTask = false;
                }
            }
        } else if (inActiveLog) {
            std::regex number_regex(R"(\d+)");
            std::smatch match;
            if (std::regex_search(line, match, number_regex)) {
                try {
                    int id = std::stoi(match.str());
                    activeLog.push_back(id);
                } catch (...) {}
            }
        } else if (inInactiveLog) {
            if (line.find("{") != std::string::npos && line.find("\"taskId\":") != std::string::npos) {
                inTask = true;
                currentEntry = LogEntry(0, TaskStatus::ACTIVE, std::chrono::system_clock::now());
            } else if (inTask) {
                if (line.find("\"taskId\":") != std::string::npos) {
                    currentEntry.taskId = parseIntValue(line, "\"taskId\": ");
                } else if (line.find("\"status\":") != std::string::npos) {
                    currentEntry.status = static_cast<TaskStatus>(parseIntValue(line, "\"status\": "));
                } else if (line.find("\"changeTime\":") != std::string::npos) {
                    auto time = parseIntValue(line, "\"changeTime\": ");
                    currentEntry.changeTime = std::chrono::system_clock::from_time_t(time);
                } else if (line.find("}") != std::string::npos) {
                    inactiveLog.push_back(currentEntry);
                    inTask = false;
                }
            }
        } else if (inSettings) {
            if (line.find("\"mainMode\":") != std::string::npos) {
                userSettings.mainMode = parseIntValue(line, "\"mainMode\": ");
            } else if (line.find("\"activeTaskMode\":") != std::string::npos) {
                userSettings.activeTaskMode = parseIntValue(line, "\"activeTaskMode\": ");
            } else if (line.find("\"historyMode\":") != std::string::npos) {
                userSettings.historyMode = parseIntValue(line, "\"historyMode\": ");
            } else if (line.find("\"windowColor\":") != std::string::npos) {
                userSettings.windowColor = parseIntValue(line, "\"windowColor\": ");
            } else if (line.find("\"textColor\":") != std::string::npos) {
                userSettings.textColor = parseIntValue(line, "\"textColor\": ");
            }
        }
    }
}

int DataManager::createTask(const std::string& name, 
                           const std::chrono::system_clock::time_point& start,
                           const std::chrono::system_clock::time_point& end) {
    Task task(name, start, end, nextTaskId++);
    tasks.push_back(task);
    activeLog.push_back(task.id);
    return task.id;
}

bool DataManager::updateTask(int taskId, const std::string& name,
                            const std::chrono::system_clock::time_point& start,
                            const std::chrono::system_clock::time_point& end) {
    for (auto& task : tasks) {
        if (task.id == taskId) {
            task.name = name;
            task.startTime = start;
            task.endTime = end;
            return true;
        }
    }
    return false;
}

bool DataManager::deleteTask(int taskId) {
    // Remove from tasks
    tasks.erase(std::remove_if(tasks.begin(), tasks.end(),
                              [taskId](const Task& task) { return task.id == taskId; }),
                tasks.end());
    
    // Remove from active log
    activeLog.erase(std::remove(activeLog.begin(), activeLog.end(), taskId), activeLog.end());
    
    // Remove from inactive log
    inactiveLog.erase(std::remove_if(inactiveLog.begin(), inactiveLog.end(),
                                    [taskId](const LogEntry& entry) { return entry.taskId == taskId; }),
                     inactiveLog.end());
    
    return true;
}

bool DataManager::changeTaskStatus(int taskId, TaskStatus newStatus) {
    for (auto& task : tasks) {
        if (task.id == taskId) {
            TaskStatus oldStatus = task.status;
            task.status = newStatus;
            
            if (newStatus == TaskStatus::ACTIVE) {
                // Moving to active
                activeLog.push_back(taskId);
                inactiveLog.erase(std::remove_if(inactiveLog.begin(), inactiveLog.end(),
                                               [taskId](const LogEntry& entry) { return entry.taskId == taskId; }),
                                inactiveLog.end());
            } else {
                // Moving to inactive
                activeLog.erase(std::remove(activeLog.begin(), activeLog.end(), taskId), activeLog.end());
                inactiveLog.emplace_back(taskId, newStatus, std::chrono::system_clock::now());
            }
            return true;
        }
    }
    return false;
}

std::vector<Task> DataManager::getActiveTasks(bool sortByStartTime) {
    std::vector<Task> activeTasks;
    for (const auto& task : tasks) {
        if (task.status == TaskStatus::ACTIVE) {
            activeTasks.push_back(task);
        }
    }
    
    if (sortByStartTime) {
        std::sort(activeTasks.begin(), activeTasks.end(),
                 [](const Task& a, const Task& b) { return a.startTime < b.startTime; });
    } else {
        std::sort(activeTasks.begin(), activeTasks.end(),
                 [](const Task& a, const Task& b) { return a.endTime > b.endTime; });
    }
    
    return activeTasks;
}

std::vector<Task> DataManager::getCompletedTasks() {
    std::vector<Task> completedTasks;
    for (const auto& task : tasks) {
        if (task.status == TaskStatus::COMPLETED) {
            completedTasks.push_back(task);
        }
    }
    
    std::sort(completedTasks.begin(), completedTasks.end(),
             [](const Task& a, const Task& b) { return a.endTime > b.endTime; });
    
    return completedTasks;
}

std::vector<Task> DataManager::getTerminatedTasks() {
    std::vector<Task> terminatedTasks;
    for (const auto& task : tasks) {
        if (task.status == TaskStatus::TERMINATED) {
            terminatedTasks.push_back(task);
        }
    }
    
    std::sort(terminatedTasks.begin(), terminatedTasks.end(),
             [](const Task& a, const Task& b) { return a.endTime > b.endTime; });
    
    return terminatedTasks;
}

std::vector<Task> DataManager::getTasksByPage(int page, int pageSize, bool activeOnly, bool sortByStartTime) {
    std::vector<Task> result;
    
    if (activeOnly) {
        result = getActiveTasks(sortByStartTime);
    } else {
        // For history mode, get completed or terminated tasks based on user settings
        if (userSettings.historyMode == 0) {
            result = getCompletedTasks();
        } else {
            result = getTerminatedTasks();
        }
    }
    
    // Pagination
    int startIndex = page * pageSize;
    if (startIndex >= result.size()) {
        return std::vector<Task>();
    }
    
    int endIndex = (std::min)(startIndex + pageSize, static_cast<int>(result.size()));
    return std::vector<Task>(result.begin() + startIndex, result.begin() + endIndex);
}

void DataManager::saveUserSettings() {
    saveToFile();
}