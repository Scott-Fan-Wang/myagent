#include "code_agent.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <filesystem>
#include <cstdlib>
#include <array>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

namespace fs = std::filesystem;

namespace CodeAgent {

// ANSI color codes
const std::string RESET = "\033[0m";
const std::string CYAN = "\033[36m";
const std::string GREEN = "\033[32m";
const std::string RED = "\033[31m";
const std::string YELLOW = "\033[33m";
const std::string BLUE_BOLD = "\033[1;34m";
const std::string GREEN_BOLD = "\033[1;32m";
const std::string RED_BOLD = "\033[1;31m";

// ============================================================================
// JsonParser Implementation
// ============================================================================

std::string JsonParser::escape(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    for (char c : str) {
        switch (c) {
            case '\"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}

std::string JsonParser::unescape(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    bool escaped = false;
    for (char c : str) {
        if (escaped) {
            switch (c) {
                case '\"': result += '\"'; break;
                case '\\': result += '\\'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                default: result += c; break;
            }
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else {
            result += c;
        }
    }
    return result;
}

bool JsonParser::findString(const std::string& json, size_t& pos, const std::string& key, std::string& value) {
    std::string searchKey = "\"" + key + "\"";
    pos = json.find(searchKey, pos);
    if (pos == std::string::npos) return false;

    pos = json.find(':', pos);
    if (pos == std::string::npos) return false;

    pos = json.find('\"', pos);
    if (pos == std::string::npos) return false;
    pos++;

    size_t end = pos;
    bool escaped = false;
    while (end < json.length()) {
        if (json[end] == '\\' && !escaped) {
            escaped = true;
        } else if (json[end] == '\"' && !escaped) {
            break;
        } else {
            escaped = false;
        }
        end++;
    }

    value = json.substr(pos, end - pos);
    value = unescape(value);
    pos = end + 1;
    return true;
}

std::string JsonParser::extractString(const std::string& json, const std::string& key) {
    size_t pos = 0;
    std::string value;
    if (findString(json, pos, key, value)) {
        return value;
    }
    return "";
}

int JsonParser::extractInt(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return 0;

    pos = json.find(':', pos);
    if (pos == std::string::npos) return 0;
    pos++;

    while (pos < json.length() && std::isspace(json[pos])) pos++;

    size_t end = pos;
    while (end < json.length() && (std::isdigit(json[end]) || json[end] == '-')) end++;

    if (end > pos) {
        return std::stoi(json.substr(pos, end - pos));
    }
    return 0;
}

bool JsonParser::extractBool(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return false;

    pos = json.find(':', pos);
    if (pos == std::string::npos) return false;

    size_t truePos = json.find("true", pos);
    size_t falsePos = json.find("false", pos);

    if (truePos != std::string::npos && (falsePos == std::string::npos || truePos < falsePos)) {
        return true;
    }
    return false;
}

std::string JsonParser::extractObject(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return "{}";

    pos = json.find(':', pos);
    if (pos == std::string::npos) return "{}";

    pos = json.find('{', pos);
    if (pos == std::string::npos) return "{}";

    int braceCount = 1;
    size_t end = pos + 1;
    while (end < json.length() && braceCount > 0) {
        if (json[end] == '{') braceCount++;
        else if (json[end] == '}') braceCount--;
        end++;
    }

    return json.substr(pos, end - pos);
}

std::vector<std::string> JsonParser::extractArray(const std::string& json, const std::string& key) {
    std::vector<std::string> result;
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return result;

    pos = json.find(':', pos);
    if (pos == std::string::npos) return result;

    pos = json.find('[', pos);
    if (pos == std::string::npos) return result;

    size_t end = pos + 1;
    int bracketCount = 1;
    while (end < json.length() && bracketCount > 0) {
        if (json[end] == '[') bracketCount++;
        else if (json[end] == ']') bracketCount--;
        end++;
    }

    std::string arrayContent = json.substr(pos + 1, end - pos - 2);
    return splitJsonArray(arrayContent);
}

std::vector<std::string> JsonParser::splitJsonArray(const std::string& jsonArray) {
    std::vector<std::string> result;
    if (jsonArray.empty()) return result;

    size_t pos = 0;
    int braceCount = 0;
    int bracketCount = 0;
    bool inString = false;
    bool escaped = false;
    size_t itemStart = 0;

    while (pos < jsonArray.length()) {
        char c = jsonArray[pos];

        if (escaped) {
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else if (c == '\"') {
            inString = !inString;
        } else if (!inString) {
            if (c == '{') braceCount++;
            else if (c == '}') braceCount--;
            else if (c == '[') bracketCount++;
            else if (c == ']') bracketCount--;
            else if (c == ',' && braceCount == 0 && bracketCount == 0) {
                std::string item = jsonArray.substr(itemStart, pos - itemStart);
                item.erase(0, item.find_first_not_of(" \t\n\r"));
                item.erase(item.find_last_not_of(" \t\n\r") + 1);
                if (!item.empty()) {
                    result.push_back(item);
                }
                itemStart = pos + 1;
            }
        }
        pos++;
    }

    std::string lastItem = jsonArray.substr(itemStart);
    lastItem.erase(0, lastItem.find_first_not_of(" \t\n\r"));
    lastItem.erase(lastItem.find_last_not_of(" \t\n\r") + 1);
    if (!lastItem.empty()) {
        result.push_back(lastItem);
    }

    return result;
}

std::map<std::string, std::string> JsonParser::parseJsonObject(const std::string& json) {
    std::map<std::string, std::string> result;

    // Remove outer braces if present
    std::string content = json;
    size_t start = content.find('{');
    size_t end = content.rfind('}');
    if (start != std::string::npos && end != std::string::npos) {
        content = content.substr(start + 1, end - start - 1);
    }

    // Parse key-value pairs
    size_t pos = 0;
    bool escaped = false;
    int braceCount = 0;
    int bracketCount = 0;

    while (pos < content.length()) {
        // Skip whitespace
        while (pos < content.length() && std::isspace(content[pos])) pos++;
        if (pos >= content.length()) break;

        // Expect a quoted key
        if (content[pos] != '\"') {
            pos++;
            continue;
        }

        // Extract key
        pos++; // skip opening quote
        size_t keyStart = pos;
        escaped = false;
        while (pos < content.length()) {
            if (escaped) {
                escaped = false;
            } else if (content[pos] == '\\') {
                escaped = true;
            } else if (content[pos] == '\"') {
                break;
            }
            pos++;
        }

        std::string key = content.substr(keyStart, pos - keyStart);
        key = unescape(key);
        pos++; // skip closing quote

        // Skip whitespace and colon
        while (pos < content.length() && (std::isspace(content[pos]) || content[pos] == ':')) pos++;

        // Extract value
        std::string value;
        if (pos < content.length()) {
            if (content[pos] == '\"') {
                // String value
                pos++; // skip opening quote
                size_t valueStart = pos;
                escaped = false;
                while (pos < content.length()) {
                    if (escaped) {
                        escaped = false;
                    } else if (content[pos] == '\\') {
                        escaped = true;
                    } else if (content[pos] == '\"') {
                        break;
                    }
                    pos++;
                }
                value = content.substr(valueStart, pos - valueStart);
                value = unescape(value);
                pos++; // skip closing quote
            } else if (content[pos] == '{') {
                // Object value
                size_t objStart = pos;
                braceCount = 1;
                pos++;
                while (pos < content.length() && braceCount > 0) {
                    if (content[pos] == '{') braceCount++;
                    else if (content[pos] == '}') braceCount--;
                    pos++;
                }
                value = content.substr(objStart, pos - objStart);
            } else if (content[pos] == '[') {
                // Array value
                size_t arrStart = pos;
                bracketCount = 1;
                pos++;
                while (pos < content.length() && bracketCount > 0) {
                    if (content[pos] == '[') bracketCount++;
                    else if (content[pos] == ']') bracketCount--;
                    pos++;
                }
                value = content.substr(arrStart, pos - arrStart);
            } else {
                // Number, boolean, or null
                size_t valueStart = pos;
                while (pos < content.length() && content[pos] != ',' && content[pos] != '}') {
                    pos++;
                }
                value = content.substr(valueStart, pos - valueStart);
                // Trim whitespace
                size_t valueEnd = value.find_last_not_of(" \t\n\r");
                if (valueEnd != std::string::npos) {
                    value = value.substr(0, valueEnd + 1);
                }
            }
        }

        if (!key.empty()) {
            result[key] = value;
        }

        // Skip to next pair (skip comma)
        while (pos < content.length() && (std::isspace(content[pos]) || content[pos] == ',')) pos++;
    }

    return result;
}

// ============================================================================
// JsonValue Implementation
// ============================================================================

std::string JsonValue::toJson() const {
    std::ostringstream oss;
    oss << "{";
    bool first = true;

    for (const auto& [key, value] : strings) {
        if (!first) oss << ",";
        oss << "\"" << key << "\":\"" << JsonParser::escape(value) << "\"";
        first = false;
    }

    for (const auto& [key, value] : ints) {
        if (!first) oss << ",";
        oss << "\"" << key << "\":" << value;
        first = false;
    }

    for (const auto& [key, value] : bools) {
        if (!first) oss << ",";
        oss << "\"" << key << "\":" << (value ? "true" : "false");
        first = false;
    }

    for (const auto& [key, arr] : arrays) {
        if (!first) oss << ",";
        oss << "\"" << key << "\":[";
        bool firstItem = true;
        for (const auto& item : arr) {
            if (!firstItem) oss << ",";
            oss << "{";
            bool firstField = true;
            for (const auto& [k, v] : item) {
                if (!firstField) oss << ",";
                oss << "\"" << k << "\":\"" << JsonParser::escape(v) << "\"";
                firstField = false;
            }
            oss << "}";
            firstItem = false;
        }
        oss << "]";
        first = false;
    }

    oss << "}";
    return oss.str();
}

JsonValue JsonValue::fromJson(const std::string& json) {
    JsonValue result;
    result.strings["raw"] = json;
    return result;
}

// ============================================================================
// Logger Implementation
// ============================================================================

Logger::Logger(bool verbose) : verbose_(verbose) {}

std::string Logger::getTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::tm tm;
    localtime_r(&time, &tm);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    return oss.str();
}

std::string Logger::getColorCode(const std::string& level) const {
    if (level == "INFO") return CYAN;
    if (level == "SUCCESS") return GREEN;
    if (level == "ERROR") return RED;
    if (level == "TOOL") return YELLOW;
    return "";
}

void Logger::log(const std::string& message, const std::string& level) {
    if (!verbose_) return;

    std::string color = getColorCode(level);
    std::string timestamp = getTimestamp();
    std::cout << color << "[" << timestamp << "] " << level << ": " << message << RESET << std::endl;
}

// ============================================================================
// FileTools Implementation
// ============================================================================

JsonValue FileTools::readFile(const std::string& filePath) {
    JsonValue result;
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            result.bools["success"] = false;
            result.strings["error"] = "Failed to open file";
            result.strings["message"] = "Failed to read " + filePath + ": Unable to open file";
            return result;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();

        result.bools["success"] = true;
        result.strings["content"] = content;
        result.strings["message"] = "Successfully read " + std::to_string(content.size()) +
                                    " characters from " + filePath;
    } catch (const std::exception& e) {
        result.bools["success"] = false;
        result.strings["error"] = e.what();
        result.strings["message"] = "Failed to read " + filePath + ": " + e.what();
    }
    return result;
}

JsonValue FileTools::writeFile(const std::string& filePath, const std::string& content) {
    JsonValue result;
    try {
        fs::path path(filePath);
        if (path.has_parent_path()) {
            fs::create_directories(path.parent_path());
        }

        std::ofstream file(filePath);
        if (!file.is_open()) {
            result.bools["success"] = false;
            result.strings["error"] = "Failed to open file for writing";
            result.strings["message"] = "Failed to write " + filePath + ": Unable to open file";
            return result;
        }

        file << content;
        file.close();

        result.bools["success"] = true;
        result.strings["message"] = "Successfully wrote " + std::to_string(content.size()) +
                                    " characters to " + filePath;
    } catch (const std::exception& e) {
        result.bools["success"] = false;
        result.strings["error"] = e.what();
        result.strings["message"] = "Failed to write " + filePath + ": " + e.what();
    }
    return result;
}

JsonValue FileTools::editFile(const std::string& filePath, const std::string& oldString,
                               const std::string& newString) {
    JsonValue result;
    try {
        std::ifstream fileIn(filePath);
        if (!fileIn.is_open()) {
            result.bools["success"] = false;
            result.strings["error"] = "Failed to open file";
            result.strings["message"] = "Failed to edit " + filePath + ": Unable to open file";
            return result;
        }

        std::stringstream buffer;
        buffer << fileIn.rdbuf();
        std::string content = buffer.str();
        fileIn.close();

        size_t pos = content.find(oldString);
        if (pos == std::string::npos) {
            result.bools["success"] = false;
            result.strings["message"] = "Could not find the specified text in " + filePath;
            return result;
        }

        content.replace(pos, oldString.length(), newString);

        std::ofstream fileOut(filePath);
        if (!fileOut.is_open()) {
            result.bools["success"] = false;
            result.strings["error"] = "Failed to open file for writing";
            result.strings["message"] = "Failed to write " + filePath + ": Unable to open file";
            return result;
        }

        fileOut << content;
        fileOut.close();

        result.bools["success"] = true;
        result.strings["message"] = "Successfully edited " + filePath;
    } catch (const std::exception& e) {
        result.bools["success"] = false;
        result.strings["error"] = e.what();
        result.strings["message"] = "Failed to edit " + filePath + ": " + e.what();
    }
    return result;
}

JsonValue FileTools::listFiles(const std::string& directory) {
    JsonValue result;
    try {
        std::vector<std::map<std::string, std::string>> files;

        for (const auto& entry : fs::directory_iterator(directory)) {
            std::map<std::string, std::string> fileInfo;
            fileInfo["name"] = entry.path().filename().string();
            fileInfo["type"] = entry.is_directory() ? "directory" : "file";

            if (entry.is_regular_file()) {
                fileInfo["size"] = std::to_string(fs::file_size(entry));
            }

            files.push_back(fileInfo);
        }

        result.bools["success"] = true;
        result.arrays["files"] = files;
        result.strings["message"] = "Found " + std::to_string(files.size()) + " items in " + directory;
    } catch (const std::exception& e) {
        result.bools["success"] = false;
        result.strings["error"] = e.what();
        result.strings["message"] = "Failed to list " + directory + ": " + e.what();
    }
    return result;
}

// ============================================================================
// ShellTools Implementation
// ============================================================================

JsonValue ShellTools::runCommand(const std::string& command, int timeout) {
    JsonValue result;

    try {
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            result.bools["success"] = false;
            result.strings["error"] = "Failed to create pipe";
            result.strings["message"] = "Failed to execute command: pipe creation failed";
            return result;
        }

        pid_t pid = fork();
        if (pid == -1) {
            close(pipefd[0]);
            close(pipefd[1]);
            result.bools["success"] = false;
            result.strings["error"] = "Failed to fork process";
            result.strings["message"] = "Failed to execute command: fork failed";
            return result;
        }

        if (pid == 0) {
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[1]);

            execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
            exit(127);
        }

        close(pipefd[1]);

        std::string output;
        char buffer[4096];
        ssize_t bytesRead;

        auto startTime = std::chrono::steady_clock::now();
        while (true) {
            auto currentTime = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();

            if (elapsed >= timeout) {
                kill(pid, SIGKILL);
                close(pipefd[0]);
                waitpid(pid, nullptr, 0);
                result.bools["success"] = false;
                result.strings["error"] = "Command timed out";
                result.strings["message"] = "Command timed out after " + std::to_string(timeout) + " seconds";
                return result;
            }

            bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1);
            if (bytesRead <= 0) break;

            buffer[bytesRead] = '\0';
            output += buffer;
        }

        close(pipefd[0]);

        int status;
        waitpid(pid, &status, 0);
        int returnCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

        result.bools["success"] = (returnCode == 0);
        result.strings["stdout"] = output;
        result.strings["stderr"] = "";
        result.ints["returncode"] = returnCode;
        result.strings["message"] = "Command executed with return code " + std::to_string(returnCode);

    } catch (const std::exception& e) {
        result.bools["success"] = false;
        result.strings["error"] = e.what();
        result.strings["message"] = "Failed to execute command: " + std::string(e.what());
    }

    return result;
}

// ============================================================================
// HttpClient Implementation
// ============================================================================

HttpClient::Response HttpClient::post(const std::string& url,
                                       const std::map<std::string, std::string>& headers,
                                       const std::string& body) {
    Response response;

    // Use curl command line tool as a fallback
    std::string curlCommand = "curl -s -w '\\n%{http_code}' -X POST '" + url + "'";

    for (const auto& [key, value] : headers) {
        curlCommand += " -H '" + key + ": " + value + "'";
    }

    std::string tempFile = "/tmp/curl_body_" + std::to_string(getpid()) + ".json";
    std::ofstream temp(tempFile);
    temp << body;
    temp.close();

    curlCommand += " -d @" + tempFile;

    FILE* pipe = popen(curlCommand.c_str(), "r");
    if (!pipe) {
        response.success = false;
        response.statusCode = 0;
        response.body = "Failed to execute curl command";
        return response;
    }

    std::string result;
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

    pclose(pipe);
    std::remove(tempFile.c_str());

    size_t lastNewline = result.find_last_of('\n');
    if (lastNewline != std::string::npos) {
        std::string statusStr = result.substr(lastNewline + 1);
        result = result.substr(0, lastNewline);

        if (lastNewline > 0 && result[lastNewline - 1] == '\n') {
            result = result.substr(0, lastNewline - 1);
        }

        try {
            response.statusCode = std::stoi(statusStr);
        } catch (...) {
            response.statusCode = 0;
        }
    } else {
        response.statusCode = 0;
    }

    response.body = result;
    response.success = (response.statusCode == 200);

    return response;
}

// ============================================================================
// Message Implementation
// ============================================================================

std::string Message::toJson() const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"role\":\"" << JsonParser::escape(role) << "\"";

    if (!content.empty()) {
        oss << ",\"content\":\"" << JsonParser::escape(content) << "\"";
    }

    if (!toolCallId.empty()) {
        oss << ",\"tool_call_id\":\"" << toolCallId << "\"";
    }

    if (!toolCalls.empty()) {
        oss << ",\"tool_calls\":[";
        for (size_t i = 0; i < toolCalls.size(); ++i) {
            if (i > 0) oss << ",";
            oss << "{";
            oss << "\"id\":\"" << toolCalls[i].id << "\",";
            oss << "\"type\":\"function\",";
            oss << "\"function\":{";
            oss << "\"name\":\"" << toolCalls[i].name << "\",";
            oss << "\"arguments\":\"{";
            bool first = true;
            for (const auto& [key, value] : toolCalls[i].arguments) {
                if (!first) oss << ",";
                oss << "\\\"" << key << "\\\":\\\"" << JsonParser::escape(value) << "\\\"";
                first = false;
            }
            oss << "}\"";
            oss << "}";
            oss << "}";
        }
        oss << "]";
    }

    oss << "}";
    return oss.str();
}

// ============================================================================
// Agent Implementation
// ============================================================================

const std::string Agent::TOOLS_JSON = R"TOOLS([
{
    "type": "function",
    "function": {
        "name": "read_file",
        "description": "Read the contents of a file",
        "parameters": {
            "type": "object",
            "properties": {
                "file_path": {
                    "type": "string",
                    "description": "The path to the file to read"
                }
            },
            "required": ["file_path"]
        }
    }
},
{
    "type": "function",
    "function": {
        "name": "write_file",
        "description": "Write content to a file (creates or overwrites)",
        "parameters": {
            "type": "object",
            "properties": {
                "file_path": {
                    "type": "string",
                    "description": "The path to the file to write"
                },
                "content": {
                    "type": "string",
                    "description": "The content to write to the file"
                }
            },
            "required": ["file_path", "content"]
        }
    }
},
{
    "type": "function",
    "function": {
        "name": "edit_file",
        "description": "Edit a file by replacing old text with new text",
        "parameters": {
            "type": "object",
            "properties": {
                "file_path": {
                    "type": "string",
                    "description": "The path to the file to edit"
                },
                "old_string": {
                    "type": "string",
                    "description": "The text to find and replace"
                },
                "new_string": {
                    "type": "string",
                    "description": "The new text to replace with"
                }
            },
            "required": ["file_path", "old_string", "new_string"]
        }
    }
},
{
    "type": "function",
    "function": {
        "name": "list_files",
        "description": "List files and directories in a given path",
        "parameters": {
            "type": "object",
            "properties": {
                "directory": {
                    "type": "string",
                    "description": "The directory path to list (defaults to current directory)"
                }
            },
            "required": []
        }
    }
},
{
    "type": "function",
    "function": {
        "name": "run_command",
        "description": "Execute a shell command and return the output",
        "parameters": {
            "type": "object",
            "properties": {
                "command": {
                    "type": "string",
                    "description": "The shell command to execute"
                },
                "timeout": {
                    "type": "integer",
                    "description": "Timeout in seconds (default: 300)"
                }
            },
            "required": ["command"]
        }
    }
}
])TOOLS";

Agent::Agent(const std::string& baseUrl, const std::string& apiKey,
             const std::string& model, bool verbose, int maxIterations)
    : baseUrl_(baseUrl), apiKey_(apiKey), model_(model), maxIterations_(maxIterations), logger_(verbose) {

    std::string systemPrompt = "You are a helpful coding assistant with access to file operations and shell commands. "
                               "You can read, write, and edit files, as well as execute shell commands. "
                               "Always explain what you're doing before using tools. "
                               "When you complete a task, summarize what was done.";

    Message systemMsg;
    systemMsg.role = "system";
    systemMsg.content = systemPrompt;
    conversationHistory_.push_back(systemMsg);
}

JsonValue Agent::executeToolInternal(const std::string& toolName,
                                     const std::map<std::string, std::string>& arguments) {
    std::ostringstream argsJson;
    argsJson << "{";
    bool first = true;
    for (const auto& [key, value] : arguments) {
        if (!first) argsJson << ", ";
        argsJson << "\"" << key << "\": \"" << value << "\"";
        first = false;
    }
    argsJson << "}";

    logger_.log("Executing tool: " + toolName + " with args: " + argsJson.str(), "TOOL");

    if (toolName == "read_file") {
        auto it = arguments.find("file_path");
        if (it == arguments.end()) {
            JsonValue error;
            error.bools["success"] = false;
            error.strings["error"] = "Missing required argument: file_path";
            return error;
        }
        return fileTools_.readFile(it->second);
    } else if (toolName == "write_file") {
        auto filePathIt = arguments.find("file_path");
        auto contentIt = arguments.find("content");
        if (filePathIt == arguments.end() || contentIt == arguments.end()) {
            JsonValue error;
            error.bools["success"] = false;
            error.strings["error"] = "Missing required arguments for write_file";
            return error;
        }
        return fileTools_.writeFile(filePathIt->second, contentIt->second);
    } else if (toolName == "edit_file") {
        auto filePathIt = arguments.find("file_path");
        auto oldStringIt = arguments.find("old_string");
        auto newStringIt = arguments.find("new_string");
        if (filePathIt == arguments.end() || oldStringIt == arguments.end() || newStringIt == arguments.end()) {
            JsonValue error;
            error.bools["success"] = false;
            error.strings["error"] = "Missing required arguments for edit_file";
            return error;
        }
        return fileTools_.editFile(filePathIt->second, oldStringIt->second, newStringIt->second);
    } else if (toolName == "list_files") {
        std::string directory = ".";
        auto it = arguments.find("directory");
        if (it != arguments.end() && !it->second.empty()) {
            directory = it->second;
        }
        return fileTools_.listFiles(directory);
    } else if (toolName == "run_command") {
        auto commandIt = arguments.find("command");
        if (commandIt == arguments.end()) {
            JsonValue error;
            error.bools["success"] = false;
            error.strings["error"] = "Missing required argument: command";
            return error;
        }
        int timeout = 300;
        auto timeoutIt = arguments.find("timeout");
        if (timeoutIt != arguments.end()) {
            try {
                timeout = std::stoi(timeoutIt->second);
            } catch (...) {
                timeout = 300;
            }
        }
        return shellTools_.runCommand(commandIt->second, timeout);
    } else {
        JsonValue result;
        result.bools["success"] = false;
        result.strings["error"] = "Unknown tool: " + toolName;
        return result;
    }
}

std::string Agent::createRequestPayload(const std::vector<Message>& messages) const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"model\":\"" << model_ << "\",";
    oss << "\"messages\":[";

    for (size_t i = 0; i < messages.size(); ++i) {
        if (i > 0) oss << ",";
        oss << messages[i].toJson();
    }

    oss << "],";
    oss << "\"tools\":" << TOOLS_JSON << ",";
    oss << "\"tool_choice\":\"auto\"";
    oss << "}";

    return oss.str();
}

std::vector<ToolCall> Agent::parseToolCalls(const std::string& responseJson) const {
    std::vector<ToolCall> toolCalls;

    size_t toolCallsPos = responseJson.find("\"tool_calls\"");
    if (toolCallsPos == std::string::npos) return toolCalls;

    size_t arrayStart = responseJson.find('[', toolCallsPos);
    if (arrayStart == std::string::npos) return toolCalls;

    size_t arrayEnd = arrayStart + 1;
    int bracketCount = 1;
    while (arrayEnd < responseJson.length() && bracketCount > 0) {
        if (responseJson[arrayEnd] == '[') bracketCount++;
        else if (responseJson[arrayEnd] == ']') bracketCount--;
        arrayEnd++;
    }

    std::string arrayContent = responseJson.substr(arrayStart + 1, arrayEnd - arrayStart - 2);
    std::vector<std::string> toolCallObjects = JsonParser::splitJsonArray(arrayContent);

    for (const auto& toolCallJson : toolCallObjects) {
        ToolCall tc;
        tc.id = JsonParser::extractString(toolCallJson, "id");

        std::string functionJson = JsonParser::extractObject(toolCallJson, "function");
        tc.name = JsonParser::extractString(functionJson, "name");

        std::string argumentsStr = JsonParser::extractString(functionJson, "arguments");

        // Parse the arguments JSON string into key-value pairs
        tc.arguments = JsonParser::parseJsonObject(argumentsStr);

        if (!tc.id.empty() && !tc.name.empty()) {
            toolCalls.push_back(tc);
        }
    }

    return toolCalls;
}

std::string Agent::extractContent(const std::string& responseJson) const {
    size_t messagePos = responseJson.find("\"message\"");
    if (messagePos == std::string::npos) return "";

    std::string messageObj = JsonParser::extractObject(responseJson, "message");
    return JsonParser::extractString(messageObj, "content");
}

std::string Agent::extractFinishReason(const std::string& responseJson) const {
    return JsonParser::extractString(responseJson, "finish_reason");
}

std::string Agent::callOpenAI(const std::vector<Message>& messages) {
    std::string payload = createRequestPayload(messages);

    std::map<std::string, std::string> headers;
    headers["Authorization"] = "Bearer " + apiKey_;
    headers["Content-Type"] = "application/json";

    std::string url = baseUrl_ + "/chat/completions";

    auto response = HttpClient::post(url, headers, payload);

    if (!response.success) {
        throw std::runtime_error("OpenAI API error: " + std::to_string(response.statusCode) +
                               " - " + response.body);
    }

    return response.body;
}

std::string Agent::processMessage(const std::string& userMessage) {
    Message userMsg;
    userMsg.role = "user";
    userMsg.content = userMessage;
    conversationHistory_.push_back(userMsg);

    int iteration = 0;

    while (iteration < maxIterations_) {
        iteration++;

        logger_.log("Calling OpenAI API (iteration " + std::to_string(iteration) + ")...", "INFO");

        std::string responseJson = callOpenAI(conversationHistory_);

        size_t choicesPos = responseJson.find("\"choices\"");
        if (choicesPos == std::string::npos) {
            return "[Error: Invalid API response]";
        }

        std::vector<std::string> choices = JsonParser::extractArray(responseJson, "choices");
        if (choices.empty()) {
            return "[Error: No choices in API response]";
        }

        std::string firstChoice = choices[0];
        std::string messageJson = JsonParser::extractObject(firstChoice, "message");
        std::string finishReason = JsonParser::extractString(firstChoice, "finish_reason");

        Message assistantMsg;
        assistantMsg.role = "assistant";
        assistantMsg.content = JsonParser::extractString(messageJson, "content");

        if (finishReason == "tool_calls") {
            assistantMsg.toolCalls = parseToolCalls(messageJson);
            conversationHistory_.push_back(assistantMsg);

            logger_.log("Assistant requested " + std::to_string(assistantMsg.toolCalls.size()) +
                       " tool call(s)", "INFO");

            for (const auto& toolCall : assistantMsg.toolCalls) {
                JsonValue result = executeToolInternal(toolCall.name, toolCall.arguments);

                if (result.bools.count("success") && result.bools.at("success")) {
                    logger_.log(result.strings.at("message"), "SUCCESS");
                } else {
                    logger_.log(result.strings.count("message") ? result.strings.at("message") :
                               "Tool execution failed", "ERROR");
                }

                Message toolResultMsg;
                toolResultMsg.role = "tool";
                toolResultMsg.toolCallId = toolCall.id;
                toolResultMsg.content = result.toJson();
                conversationHistory_.push_back(toolResultMsg);
            }

            continue;
        }

        conversationHistory_.push_back(assistantMsg);

        if (!assistantMsg.content.empty()) {
            return assistantMsg.content;
        } else {
            return "[Assistant provided no text response]";
        }
    }

    return "[Max iterations reached - the assistant may need more steps to complete the task]";
}

void Agent::clearHistory() {
    std::string systemPrompt = "You are a helpful coding assistant with access to file operations and shell commands. "
                               "You can read, write, and edit files, as well as execute shell commands. "
                               "Always explain what you're doing before using tools. "
                               "When you complete a task, summarize what was done.";

    conversationHistory_.clear();
    Message systemMsg;
    systemMsg.role = "system";
    systemMsg.content = systemPrompt;
    conversationHistory_.push_back(systemMsg);
}

void Agent::run() {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Simple Code Agent - OpenAI powered coding assistant (C++)\n";
    std::cout << std::string(60, '=') << "\n";
    std::cout << "Commands:\n";
    std::cout << "  - Type your request and press Enter\n";
    std::cout << "  - Type 'exit' or 'quit' to exit\n";
    std::cout << "  - Type 'clear' to clear conversation history\n";
    std::cout << std::string(60, '=') << "\n\n";

    while (true) {
        try {
            std::cout << "\n" << BLUE_BOLD << "You:" << RESET << " ";
            std::string userInput;
            std::getline(std::cin, userInput);

            if (userInput.empty()) {
                continue;
            }

            if (userInput == "exit" || userInput == "quit") {
                std::cout << "\nGoodbye!\n";
                break;
            }

            if (userInput == "clear") {
                clearHistory();
                std::cout << "Conversation history cleared.\n";
                continue;
            }

            std::string response = processMessage(userInput);

            std::cout << "\n" << GREEN_BOLD << "Assistant:" << RESET << " " << response << "\n";

        } catch (const std::exception& e) {
            logger_.log(std::string("Error: ") + e.what(), "ERROR");
            std::cout << "\n" << RED_BOLD << "Error:" << RESET << " " << e.what() << "\n";
        }
    }
}

} // namespace CodeAgent
