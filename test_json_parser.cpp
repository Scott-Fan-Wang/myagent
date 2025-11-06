#include "code_agent.hpp"
#include <iostream>
#include <cassert>

using namespace CodeAgent;

void testParseJsonObject() {
    std::cout << "Testing JSON parser..." << std::endl;

    // Test case 1: Simple string values
    std::string json1 = R"({"command": "ls -la", "timeout": "30"})";
    auto result1 = JsonParser::parseJsonObject(json1);

    std::cout << "Test 1 - Simple values:" << std::endl;
    for (const auto& [key, value] : result1) {
        std::cout << "  " << key << " = " << value << std::endl;
    }
    assert(result1["command"] == "ls -la");
    assert(result1["timeout"] == "30");
    std::cout << "✓ Test 1 passed" << std::endl;

    // Test case 2: File path
    std::string json2 = R"({"file_path": "/home/user/test.txt"})";
    auto result2 = JsonParser::parseJsonObject(json2);

    std::cout << "\nTest 2 - File path:" << std::endl;
    for (const auto& [key, value] : result2) {
        std::cout << "  " << key << " = " << value << std::endl;
    }
    assert(result2["file_path"] == "/home/user/test.txt");
    std::cout << "✓ Test 2 passed" << std::endl;

    // Test case 3: Multiple fields
    std::string json3 = R"({"file_path": "test.txt", "old_string": "hello", "new_string": "world"})";
    auto result3 = JsonParser::parseJsonObject(json3);

    std::cout << "\nTest 3 - Multiple fields:" << std::endl;
    for (const auto& [key, value] : result3) {
        std::cout << "  " << key << " = " << value << std::endl;
    }
    assert(result3["file_path"] == "test.txt");
    assert(result3["old_string"] == "hello");
    assert(result3["new_string"] == "world");
    std::cout << "✓ Test 3 passed" << std::endl;

    // Test case 4: Empty object
    std::string json4 = R"({})";
    auto result4 = JsonParser::parseJsonObject(json4);

    std::cout << "\nTest 4 - Empty object:" << std::endl;
    std::cout << "  Size: " << result4.size() << std::endl;
    assert(result4.empty());
    std::cout << "✓ Test 4 passed" << std::endl;

    // Test case 5: With escaped quotes
    std::string json5 = R"({"content": "Hello \"World\""})";
    auto result5 = JsonParser::parseJsonObject(json5);

    std::cout << "\nTest 5 - Escaped quotes:" << std::endl;
    for (const auto& [key, value] : result5) {
        std::cout << "  " << key << " = " << value << std::endl;
    }
    assert(result5["content"] == "Hello \"World\"");
    std::cout << "✓ Test 5 passed" << std::endl;

    std::cout << "\n✓ All tests passed!" << std::endl;
}

int main() {
    try {
        testParseJsonObject();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
