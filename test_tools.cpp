#include "code_agent.hpp"
#include <iostream>

using namespace CodeAgent;

int main() {
    std::cout << "Testing FileTools and ShellTools..." << std::endl;

    // Test 1: Read file
    std::cout << "\n=== Test 1: Read file ===" << std::endl;
    auto result1 = FileTools::readFile("test_file.txt");
    std::cout << "Success: " << (result1.bools["success"] ? "true" : "false") << std::endl;
    std::cout << "Message: " << result1.strings["message"] << std::endl;
    if (result1.bools["success"]) {
        std::cout << "Content preview: " << result1.strings["content"].substr(0, 50) << "..." << std::endl;
    }

    // Test 2: Write file
    std::cout << "\n=== Test 2: Write file ===" << std::endl;
    auto result2 = FileTools::writeFile("test_output.txt", "This is a test output file.\n");
    std::cout << "Success: " << (result2.bools["success"] ? "true" : "false") << std::endl;
    std::cout << "Message: " << result2.strings["message"] << std::endl;

    // Test 3: List files
    std::cout << "\n=== Test 3: List files ===" << std::endl;
    auto result3 = FileTools::listFiles(".");
    std::cout << "Success: " << (result3.bools["success"] ? "true" : "false") << std::endl;
    std::cout << "Message: " << result3.strings["message"] << std::endl;
    if (result3.bools["success"]) {
        std::cout << "Files found: " << result3.arrays["files"].size() << std::endl;
        std::cout << "First 5 files:" << std::endl;
        for (size_t i = 0; i < std::min(size_t(5), result3.arrays["files"].size()); i++) {
            auto& file = result3.arrays["files"][i];
            std::cout << "  - " << file["name"] << " (" << file["type"] << ")" << std::endl;
        }
    }

    // Test 4: Run command
    std::cout << "\n=== Test 4: Run command ===" << std::endl;
    auto result4 = ShellTools::runCommand("echo 'Hello from shell'", 5);
    std::cout << "Success: " << (result4.bools["success"] ? "true" : "false") << std::endl;
    std::cout << "Message: " << result4.strings["message"] << std::endl;
    std::cout << "Output: " << result4.strings["stdout"] << std::endl;

    // Test 5: Edit file
    std::cout << "\n=== Test 5: Edit file ===" << std::endl;
    auto result5 = FileTools::editFile("test_output.txt", "test output", "MODIFIED OUTPUT");
    std::cout << "Success: " << (result5.bools["success"] ? "true" : "false") << std::endl;
    std::cout << "Message: " << result5.strings["message"] << std::endl;

    // Verify edit
    std::cout << "\n=== Verify edit ===" << std::endl;
    auto result6 = FileTools::readFile("test_output.txt");
    if (result6.bools["success"]) {
        std::cout << "Modified content: " << result6.strings["content"] << std::endl;
    }

    std::cout << "\n✓ All tool tests completed!" << std::endl;

    return 0;
}
