#include "code_agent.hpp"
#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>

// Simple command-line argument parser
struct Options {
    int maxIterations = 99;
    bool showHelp = false;
};

Options parseArgs(int argc, char* argv[]) {
    Options opts;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-m" || arg == "--max_iter") {
            if (i + 1 < argc) {
                try {
                    opts.maxIterations = std::stoi(argv[i + 1]);
                    ++i; // Skip next argument since we used it
                } catch (...) {
                    std::cerr << "Error: Invalid value for " << arg << std::endl;
                    opts.showHelp = true;
                }
            } else {
                std::cerr << "Error: " << arg << " requires a value" << std::endl;
                opts.showHelp = true;
            }
        } else if (arg == "-h" || arg == "--help") {
            opts.showHelp = true;
        } else {
            std::cerr << "Error: Unknown option " << arg << std::endl;
            opts.showHelp = true;
        }
    }

    return opts;
}

void printHelp(const char* programName) {
    std::cout << "Usage: " << programName << " [options]\n"
              << "Options:\n"
              << "  -m, --max_iter <num>  Maximum number of iterations (default: 99)\n"
              << "  -h, --help            Show this help message\n";
}

int main(int argc, char* argv[]) {
    Options opts = parseArgs(argc, argv);

    if (opts.showHelp) {
        printHelp(argv[0]);
        return opts.showHelp && argc > 1 ? 1 : 0;
    }
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
        CodeAgent::Agent agent(baseUrl, apiKey, model, true, opts.maxIterations);
        agent.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
