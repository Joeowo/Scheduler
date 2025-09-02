#include "DataManager.h"
#include <algorithm>
#include <sstream>
#include <windows.h>
#include <iostream>

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
    // std::cout << "=== 开始加载数据文件 ===" << std::endl;
    // std::cout << "文件路径: " << DATA_FILE << std::endl;
    
    std::ifstream file(DATA_FILE);
    if (!file.is_open()){
        // std::cout << "警告: 无法打开数据文件" << std::endl;
        return;
    } 
    
    // std::cout << "文件打开成功，开始解析..." << std::endl;
    
    // 清空现有数据
    // std::cout << "清空现有数据..." << std::endl;
    tasks.clear();
    activeLog.clear();
    inactiveLog.clear();
    
    std::string line;
    bool inTasks = false;
    bool inTask = false;
    bool inActiveLog = false;
    bool inInactiveLog = false;
    bool inSettings = false;
    
    Task currentTask("", std::chrono::system_clock::now(), std::chrono::system_clock::now());
    LogEntry currentEntry(0, TaskStatus::ACTIVE, std::chrono::system_clock::now());
    
    int lineNumber = 0;
    while (std::getline(file, line)) {
        lineNumber++;
        std::string originalLine = line;
        line.erase(0, line.find_first_not_of(" \t"));
        
        // std::cout << "第" << lineNumber << "行: 原始内容 = \"" << originalLine << "\"" << std::endl;
        // std::cout << "第" << lineNumber << "行: 处理后 = \"" << line << "\"" << std::endl;
        
        if (line.find("\"tasks\":") != std::string::npos) {
            // std::cout << "第" << lineNumber << "行: 进入tasks区域" << std::endl;
            inTasks = true;
            inTask = false;
            inActiveLog = false;
            inInactiveLog = false;
            inSettings = false;
        } else if (line.find("\"activeLog\":") != std::string::npos) {
            // std::cout << "第" << lineNumber << "行: 进入activeLog区域" << std::endl;
            inTasks = false;
            inActiveLog = true;
            inInactiveLog = false;
            inSettings = false;
        } else if (line.find("\"inactiveLog\":") != std::string::npos) {
            // std::cout << "第" << lineNumber << "行: 进入inactiveLog区域" << std::endl;
            inTasks = false;
            inActiveLog = false;
            inInactiveLog = true;
            inSettings = false;
        } else if (line.find("\"userSettings\":") != std::string::npos) {
            // std::cout << "第" << lineNumber << "行: 进入userSettings区域" << std::endl;
            inTasks = false;
            inActiveLog = false;
            inInactiveLog = false;
            inSettings = true;
        }
        
        // std::cout << "第" << lineNumber << "行: 当前状态 - inTasks=" << inTasks 
        //           << ", inTask=" << inTask 
        //           << ", inActiveLog=" << inActiveLog 
        //           << ", inInactiveLog=" << inInactiveLog 
        //           << ", inSettings=" << inSettings << std::endl;
        
        if (inTasks) {
            // std::cout << "第" << lineNumber << "行: 在tasks区域内" << std::endl;
            
            if (line.find("{") != std::string::npos && !inTask) {
                // std::cout << "第" << lineNumber << "行: 发现开始大括号，开始新任务" << std::endl;
                inTask = true;
                currentTask = Task("", std::chrono::system_clock::now(), std::chrono::system_clock::now());
            } else if (inTask) {
                // std::cout << "第" << lineNumber << "行: 正在解析任务字段" << std::endl;
                
                if (line.find("\"id\":") != std::string::npos) {
                    currentTask.id = parseIntValue(line, "\"id\": ");
                    // std::cout << "第" << lineNumber << "行: 解析任务ID = " << currentTask.id << std::endl;
                } else if (line.find("\"name\":") != std::string::npos) {
                    // 修复：改进字符串解析，直接提取引号内的内容
                    size_t start = line.find("\"name\": \"") + 9; // "name": " 的长度
                    size_t end = line.find_last_of("\"");
                    if (start != std::string::npos && end != std::string::npos && end > start) {
                        currentTask.name = line.substr(start, end - start);
                        // std::cout << "第" << lineNumber << "行: 解析任务名称 = \"" << currentTask.name << "\"" << std::endl;
                    } else {
                        // std::cout << "第" << lineNumber << "行: 任务名称解析失败" << std::endl;
                    }
                } else if (line.find("\"startTime\":") != std::string::npos) {
                    auto time = parseIntValue(line, "\"startTime\": ");
                    currentTask.startTime = std::chrono::system_clock::from_time_t(time);
                    // std::cout << "第" << lineNumber << "行: 解析开始时间 = " << time << std::endl;
                } else if (line.find("\"endTime\":") != std::string::npos) {
                    auto time = parseIntValue(line, "\"endTime\": ");
                    currentTask.endTime = std::chrono::system_clock::from_time_t(time);
                    // std::cout << "第" << lineNumber << "行: 解析结束时间 = " << time << std::endl;
                } else if (line.find("\"status\":") != std::string::npos) {
                    currentTask.status = static_cast<TaskStatus>(parseIntValue(line, "\"status\": "));
                    // std::cout << "第" << lineNumber << "行: 解析任务状态 = " << static_cast<int>(currentTask.status) << std::endl;
                } else if (line.find("}") != std::string::npos) {
                    // std::cout << "第" << lineNumber << "行: 发现结束大括号，任务解析完成" << std::endl;
                    tasks.push_back(currentTask);
                    nextTaskId = (std::max)(nextTaskId, currentTask.id + 1);
                    // std::cout << "第" << lineNumber << "行: 任务已添加到列表，当前任务总数: " << tasks.size() << std::endl;
                    inTask = false;
                } else {
                    // std::cout << "第" << lineNumber << "行: 未识别的任务字段: " << line << std::endl;
                }
            } else {
                // std::cout << "第" << lineNumber << "行: 在tasks区域但不在任务解析状态" << std::endl;
            }
        } else if (inActiveLog) {
            // std::cout << "第" << lineNumber << "行: 在activeLog区域内" << std::endl;
            
            if (line.find("[") != std::string::npos) {
                // std::cout << "第" << lineNumber << "行: 发现activeLog数组开始" << std::endl;
            } else if (line.find("]") != std::string::npos) {
                // std::cout << "第" << lineNumber << "行: 发现activeLog数组结束" << std::endl;
            }
            
            // 修复：改进数字解析逻辑，过滤掉非数字字符
            std::string numbers = line;
            std::string cleanNumbers = "";
            
            // 只保留数字和逗号
            for (char c : numbers) {
                if (std::isdigit(c) || c == ',') {
                    cleanNumbers += c;
                }
            }
            
            // std::cout << "第" << lineNumber << "行: 清理后的数字字符串 = \"" << cleanNumbers << "\"" << std::endl;
            
            // 解析逗号分隔的数字
            size_t pos = 0;
            while ((pos = cleanNumbers.find(",")) != std::string::npos) {
                std::string numStr = cleanNumbers.substr(0, pos);
                if (!numStr.empty()) {
                    try {
                        int id = std::stoi(numStr);
                        activeLog.push_back(id);
                        // std::cout << "第" << lineNumber << "行: 添加活跃日志ID = " << id << std::endl;
                    } catch (...) {
                        // std::cout << "第" << lineNumber << "行: 解析活跃日志ID失败: " << numStr << std::endl;
                    }
                }
                cleanNumbers.erase(0, pos + 1);
            }
            // 处理最后一个数字
            if (!cleanNumbers.empty()) {
                try {
                    int id = std::stoi(cleanNumbers);
                    activeLog.push_back(id);
                    // std::cout << "第" << lineNumber << "行: 添加活跃日志ID = " << id << std::endl;
                } catch (...) {
                    // std::cout << "第" << lineNumber << "行: 解析活跃日志ID失败: " << cleanNumbers << std::endl;
                }
            }
        } else if (inInactiveLog) {
            // std::cout << "第" << lineNumber << "行: 在inactiveLog区域内" << std::endl;
            
            // 修复：改进inactiveLog解析逻辑
            if (line.find("{") != std::string::npos && !inTask) {
                // std::cout << "第" << lineNumber << "行: 开始解析非活跃日志条目" << std::endl;
                inTask = true;
                currentEntry = LogEntry(0, TaskStatus::ACTIVE, std::chrono::system_clock::now());
            } else if (inTask) {
                // std::cout << "第" << lineNumber << "行: 正在解析日志条目字段" << std::endl;
                
                if (line.find("\"taskId\":") != std::string::npos) {
                    currentEntry.taskId = parseIntValue(line, "\"taskId\": ");
                    // std::cout << "第" << lineNumber << "行: 解析日志任务ID = " << currentEntry.taskId << std::endl;
                } else if (line.find("\"status\":") != std::string::npos) {
                    currentEntry.status = static_cast<TaskStatus>(parseIntValue(line, "\"status\": "));
                    // std::cout << "第" << lineNumber << "行: 解析日志状态 = " << static_cast<int>(currentEntry.status) << std::endl;
                } else if (line.find("\"changeTime\":") != std::string::npos) {
                    auto time = parseIntValue(line, "\"changeTime\": ");
                    currentEntry.changeTime = std::chrono::system_clock::from_time_t(time);
                    // std::cout << "第" << lineNumber << "行: 解析变更时间 = " << time << std::endl;
                } else if (line.find("}") != std::string::npos) {
                    inactiveLog.push_back(currentEntry);
                    // std::cout << "第" << lineNumber << "行: 非活跃日志条目解析完成，当前总数: " << inactiveLog.size() << std::endl;
                    inTask = false;
                } else {
                    // std::cout << "第" << lineNumber << "行: 未识别的日志字段: " << line << std::endl;
                }
            } else {
                // std::cout << "第" << lineNumber << "行: 在inactiveLog区域但不在条目解析状态" << std::endl;
            }
        } else if (inSettings) {
            // std::cout << "第" << lineNumber << "行: 在userSettings区域内" << std::endl;
            if (line.find("\"nextTaskId\":") != std::string::npos) {
                nextTaskId = parseIntValue(line, "\"nextTaskId\": ");
                // std::cout << "第" << lineNumber << "行: 解析nextTaskId = " << nextTaskId << std::endl;
            } else if (line.find("\"mainMode\":") != std::string::npos) {
                userSettings.mainMode = parseIntValue(line, "\"mainMode\": ");
                // std::cout << "第" << lineNumber << "行: 解析主模式 = " << userSettings.mainMode << std::endl;
            } else if (line.find("\"activeTaskMode\":") != std::string::npos) {
                userSettings.activeTaskMode = parseIntValue(line, "\"activeTaskMode\": ");
                // std::cout << "第" << lineNumber << "行: 解析活跃任务模式 = " << userSettings.activeTaskMode << std::endl;
            } else if (line.find("\"historyMode\":") != std::string::npos) {
                userSettings.historyMode = parseIntValue(line, "\"historyMode\": ");
                // std::cout << "第" << lineNumber << "行: 解析历史模式 = " << userSettings.historyMode << std::endl;
            } else if (line.find("\"windowColor\":") != std::string::npos) {
                // 修复：使用正确的键名
                userSettings.windowColor = parseIntValue(line, "\"windowColor\": ");
                // std::cout << "第" << lineNumber << "行: 解析窗口颜色 = " << userSettings.windowColor << std::endl;
            } else if (line.find("\"textColor\":") != std::string::npos) {
                userSettings.textColor = parseIntValue(line, "\"textColor\": ");
                // std::cout << "第" << lineNumber << "行: 解析文本颜色 = " << userSettings.textColor << std::endl;
            } else if (line.find("{") != std::string::npos || line.find("}") != std::string::npos) {
                // 忽略大括号行
                // std::cout << "第" << lineNumber << "行: 忽略大括号行" << std::endl;
            } else {
                // std::cout << "第" << lineNumber << "行: 未识别的设置字段: " << line << std::endl;
            }
        } else {
            // std::cout << "第" << lineNumber << "行: 不在任何解析区域内" << std::endl;
        }
    }
    
    // std::cout << "=== 数据加载完成 ===" << std::endl;
    // std::cout << "  任务数量: " << tasks.size() << std::endl;
    // std::cout << "  活跃日志数量: " << activeLog.size() << std::endl;
    // std::cout << "  非活跃日志数量: " << inactiveLog.size() << std::endl;
    // std::cout << "  下一个任务ID: " << nextTaskId << std::endl;
    
    // 验证数据一致性
    // std::cout << "=== 数据一致性检查 ===" << std::endl;
    if (activeLog.size() > 0 && tasks.size() == 0) {
        // std::cout << "警告: 有活跃日志但没有任务，数据不一致！" << std::endl;
        // std::cout << "活跃日志内容: ";
        // for (int id : activeLog) {
        //     std::cout << id << " ";
        // }
        // std::cout << std::endl;
    }
    
    if (nextTaskId == 1 && tasks.size() > 0) {
        // std::cout << "警告: nextTaskId被重置为1，但实际有任务存在！" << std::endl;
        // std::cout << "任务ID列表: ";
        // for (const auto& task : tasks) {
        //     std::cout << task.id << " ";
        // }
        // std::cout << std::endl;
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
                 [](const Task& a, const Task& b) { return a.endTime < b.endTime; });
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
    std::cout << "=== 开始保存用户设置 ===" << std::endl;
    std::cout << "  主模式: " << userSettings.mainMode << std::endl;
    std::cout << "  活跃任务模式: " << userSettings.activeTaskMode << std::endl;
    std::cout << "  历史模式: " << userSettings.historyMode << std::endl;
    std::cout << "  窗口颜色: " << userSettings.windowColor << std::endl;
    std::cout << "  文本颜色: " << userSettings.textColor << std::endl;
    
    saveToFile();
    std::cout << "用户设置保存完成" << std::endl;
}