#pragma once
#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include <sstream>

namespace fs = std::filesystem;

class History {
    private:
    fs::path log_path;
    public:
    History(const fs::path path);
    ~History() = default;

    bool addToLogs(const std::string& who, const std::string& message);
    std::string readLogs() const;
};