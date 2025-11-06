#include "code_agent.hpp"
#include <iostream>
#include <cstdlib>
#include <string>

int main(int /* argc */, char* /* argv */[]) {
    // Get API key from environment or use default
    const char* apiKeyEnv = std::getenv("API_KEY");
    std::string apiKey = apiKeyEnv ? apiKeyEnv : "your-api-key-here";

    // Get base URL from environment or use default
    const char* baseUrlEnv = std::getenv("BASE_URL");
    std::string baseUrl = baseUrlEnv ? baseUrlEnv : "https://api.deepseek.com/v1";

    // Check if API key is valid
    if (apiKey.empty() || apiKey == "your-api-key-here") {
        std::cout << "OpenAI API key not found in environment.\n";
        std::cout << "Please enter your API key: ";
        std::getline(std::cin, apiKey);

        if (apiKey.empty()) {
            std::cerr << "Error: API key is required.\n";
            return 1;
        }
    }

    // Get model from environment or use default
    const char* modelEnv = std::getenv("OPENAI_MODEL");
    std::string model = modelEnv ? modelEnv : "deepseek-chat";

    try {
        // Create and run agent
        CodeAgent::Agent agent(baseUrl, apiKey, model, true);
        agent.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
