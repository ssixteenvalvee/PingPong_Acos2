#include "history.hpp"
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <vector>
#include <sstream>

namespace fs = std::filesystem;

History::History(const fs::path path) {
    if (path.empty()) {
        this->log_path = fs::current_path() / "logs.txt";
    } else {
        this->log_path = path / "logs.txt";
    }
}

bool History::addToLogs(const std::string& who, const std::string& message) {
    std::ofstream file(this->log_path, std::ios::app);
    if (!file.is_open()) {
        std::cerr << "[HIST] Failed to open log file: " << this->log_path << std::endl;
        return false;
    }
    file << "[" << who << "]: " << message << "\n";
    file.close();
    return true;
}

#include <vector>
#include <sstream>

std::string History::readLogs() const {
    std::ifstream file(this->log_path);
    if (!file.is_open()) {
        return "History is empty.";
    }
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    if (lines.empty()) {
        return "History is empty.";
    }
    const size_t max_lines = 10;
    size_t start = (lines.size() > max_lines) ? (lines.size() - max_lines) : 0;
    std::ostringstream result;
    for (size_t i = start; i < lines.size(); ++i) {
        result << lines[i];
        if (i + 1 < lines.size()) {
            result << "\n";
        }
    }
    return result.str();
}

