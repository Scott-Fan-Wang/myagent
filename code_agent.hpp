#ifndef CODE_AGENT_HPP
#define CODE_AGENT_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <optional>

namespace CodeAgent {

// JSON-like structure for results
struct JsonValue {
    std::map<std::string, std::string> strings;
    std::map<std::string, int> ints;
    std::map<std::string, bool> bools;
    std::map<std::string, std::vector<std::map<std::string, std::string>>> arrays;

    std::string toJson() const;
    static JsonValue fromJson(const std::string& json);
};

// Logger class for tracking operations
class Logger {
public:
    explicit Logger(bool verbose = true);
    void log(const std::string& message, const std::string& level = "INFO");

private:
    bool verbose_;
    std::string getTimestamp() const;
    std::string getColorCode(const std::string& level) const;
};

// File operations tools
class FileTools {
public:
    static JsonValue readFile(const std::string& filePath);
    static JsonValue writeFile(const std::string& filePath, const std::string& content);
    static JsonValue editFile(const std::string& filePath, const std::string& oldString,
                             const std::string& newString);
    static JsonValue listFiles(const std::string& directory = ".");
};

// Shell command execution tools
class ShellTools {
public:
    static JsonValue runCommand(const std::string& command, int timeout = 30);
};

// Tool call structure
struct ToolCall {
    std::string id;
    std::string name;
    std::map<std::string, std::string> arguments;
};

// Message structure for conversation
struct Message {
    std::string role;
    std::string content;
    std::vector<ToolCall> toolCalls;
    std::string toolCallId;

    std::string toJson() const;
};

// HTTP client for API calls
class HttpClient {
public:
    struct Response {
        int statusCode;
        std::string body;
        bool success;
    };

    static Response post(const std::string& url,
                        const std::map<std::string, std::string>& headers,
                        const std::string& body);
};

// Main agent class
class Agent {
public:
    Agent(const std::string& baseUrl, const std::string& apiKey,
          const std::string& model = "gpt-4-turbo-preview", bool verbose = true);

    std::string processMessage(const std::string& userMessage);
    void run();
    void clearHistory();

private:
    std::string baseUrl_;
    std::string apiKey_;
    std::string model_;
    Logger logger_;
    std::vector<Message> conversationHistory_;
    FileTools fileTools_;
    ShellTools shellTools_;

    static const std::string TOOLS_JSON;

    JsonValue executeToolInternal(const std::string& toolName,
                                  const std::map<std::string, std::string>& arguments);
    std::string callOpenAI(const std::vector<Message>& messages);
    std::string createRequestPayload(const std::vector<Message>& messages) const;
    std::vector<ToolCall> parseToolCalls(const std::string& responseJson) const;
    std::string extractContent(const std::string& responseJson) const;
    std::string extractFinishReason(const std::string& responseJson) const;
};

// JSON parsing utilities
class JsonParser {
public:
    static std::string escape(const std::string& str);
    static std::string unescape(const std::string& str);
    static std::string extractString(const std::string& json, const std::string& key);
    static int extractInt(const std::string& json, const std::string& key);
    static bool extractBool(const std::string& json, const std::string& key);
    static std::vector<std::string> extractArray(const std::string& json, const std::string& key);
    static std::string extractObject(const std::string& json, const std::string& key);
    static std::vector<std::string> splitJsonArray(const std::string& jsonArray);
    static bool findString(const std::string& json, size_t& pos, const std::string& key, std::string& value);
    static std::map<std::string, std::string> parseJsonObject(const std::string& json);
};

} // namespace CodeAgent

#endif // CODE_AGENT_HPP
