# Simple Code Agent - C++ Implementation

A C++ implementation of the Simple Code Agent, providing an AI-powered coding assistant with file operations and shell command execution capabilities. This is a complete port of the Python version using modern C++17 features and standard libraries.

## Features

All features from the Python implementation have been ported to C++:

### Core Components

1. **Logger** - Colored console logging with timestamps
   - INFO (Cyan)
   - SUCCESS (Green)
   - ERROR (Red)
   - TOOL (Yellow)

2. **FileTools** - Complete file operations
   - `read_file` - Read file contents
   - `write_file` - Write/create files (with automatic directory creation)
   - `edit_file` - Find and replace text in files
   - `list_files` - List directory contents with metadata

3. **ShellTools** - Shell command execution
   - `run_command` - Execute shell commands with timeout support
   - Captures stdout/stderr
   - Returns exit codes

4. **CodeAgent** - Main agent with OpenAI API integration
   - Conversation history management
   - Tool calling support
   - Interactive CLI loop
   - Automatic tool execution

## Implementation Details

### Standard Libraries Used

- `<filesystem>` - File system operations (C++17)
- `<chrono>` - Timestamps and timeout handling
- `<string>`, `<sstream>` - String manipulation
- `<vector>`, `<map>` - Data structures
- `<fstream>` - File I/O
- `<iostream>` - Console I/O
- `<algorithm>` - Utility functions

### System Libraries Used

- POSIX `fork()`, `pipe()`, `execl()` - Process management for shell commands
- `curl` (command-line) - HTTP client for API calls

### Key Differences from Python Version

1. **HTTP Client**: Uses `curl` command-line tool instead of aiohttp (to avoid external C++ dependencies)
2. **JSON Parsing**: Custom lightweight JSON parser (no external JSON library needed)
3. **Synchronous**: Not async (C++ standard library doesn't have built-in async HTTP)
4. **Process Execution**: Uses POSIX fork/exec for better control and timeout handling

## Building

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, or MSVC 2017+)
- CMake 3.10 or later
- `curl` command-line tool (usually pre-installed on Linux/macOS)

### Build Instructions

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build
cmake --build .

# Or use make directly
make

# Optional: Install
sudo make install
```

### Alternative Build (without CMake)

```bash
# Direct compilation with g++
g++ -std=c++17 -o code_agent main.cpp code_agent.cpp -lpthread -lstdc++fs

# Or with clang++
clang++ -std=c++17 -o code_agent main.cpp code_agent.cpp -lpthread -lstdc++fs
```

## Usage

### Setting Up Environment Variables

```bash
# Required: Set your API key
export API_KEY="your-api-key-here"

# Optional: Set base URL (defaults to DeepSeek)
export BASE_URL="https://api.deepseek.com/v1"

# Optional: Set model name
export OPENAI_MODEL="deepseek-chat"
```

### Running the Agent

```bash
# If installed
code_agent

# If running from build directory
./code_agent

# The agent will prompt for API key if not set in environment
```

### Interactive Commands

Once running:
- Type your requests naturally
- Type `exit` or `quit` to exit
- Type `clear` to clear conversation history

### Example Session

```
============================================================
Simple Code Agent - OpenAI powered coding assistant (C++)
============================================================
Commands:
  - Type your request and press Enter
  - Type 'exit' or 'quit' to exit
  - Type 'clear' to clear conversation history
============================================================

You: List all Python files in the current directory

[10:30:45] INFO: Calling OpenAI API (iteration 1)...
[10:30:46] TOOL: Executing tool: list_files with args: {"directory": "."}
[10:30:46] SUCCESS: Found 5 items in .