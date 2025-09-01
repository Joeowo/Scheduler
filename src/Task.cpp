#include "Task.h"
#include <sstream>
#include <iomanip>
#include <Windows.h>

std::wstring Task::formatDateTime(const std::chrono::system_clock::time_point& time) {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    tm tm_struct;
    localtime_s(&tm_struct, &time_t);
    
    std::wostringstream woss;
    woss << std::put_time(&tm_struct, L"%Y-%m-%d %H:%M");
    return woss.str();
}

std::chrono::system_clock::time_point Task::parseDateTime(const std::string& dateStr) {
    tm tm_struct = {};
    std::istringstream iss(dateStr);
    iss >> std::get_time(&tm_struct, "%Y-%m-%d %H:%M");
    
    auto time_t = mktime(&tm_struct);
    return std::chrono::system_clock::from_time_t(time_t);
}

std::chrono::system_clock::time_point Task::parseDateTime(const std::wstring& dateStr) {
    tm tm_struct = {};
    std::wistringstream wiss(dateStr);
    wiss >> std::get_time(&tm_struct, L"%Y-%m-%d %H:%M");
    
    auto time_t = mktime(&tm_struct);
    return std::chrono::system_clock::from_time_t(time_t);
}