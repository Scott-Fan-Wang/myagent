# Usage Guide for Simple Code Agent

## Quick Start

### 1. Installation

```bash
# Install dependencies
pip install -r requirements.txt

# Set your OpenAI API key
export OPENAI_API_KEY='sk-...'
```

### 2. Run the Agent

```bash
python simple_code_agent.py
```

## Features in Detail

### File Operations

#### Read Files
```
You: Read the file config.json
```
The agent will use the `read_file` tool to read and display the contents.

#### Write Files
```
You: Create a new file called hello.py that prints "Hello, World!"
```
The agent will use the `write_file` tool to create the file with appropriate content.

#### Edit Files
```
You: In hello.py, change "World" to "Python"
```
The agent will use the `edit_file` tool to make the replacement.

#### List Files
```
You: What files are in this directory?
```
The agent will use the `list_files` tool to show directory contents.

### Shell Commands

#### Run Commands
```
You: Run ls -la to see all files
```

```
You: Execute pytest to run the tests
```

```
You: Run python hello.py
```

The agent will use the `run_command` tool to execute shell commands and show you the output.

### Complex Tasks

The agent can handle multi-step tasks:

```
You: Create a Python project with the following:
1. A main.py file with a greeting function
2. A test_main.py file with a test for the greeting function
3. A requirements.txt with pytest
4. Run the tests
```

The agent will:
1. Create main.py with the function
2. Create test_main.py with the test
3. Create requirements.txt
4. Execute pytest

## Architecture Overview

### Components

1. **CodeAgent**
   - Main orchestrator
   - Communicates with OpenAI API using aiohttp
   - Manages conversation history
   - Executes tool calls

2. **FileTools**
   - `read_file(file_path)` - Read file contents
   - `write_file(file_path, content)` - Write to file
   - `edit_file(file_path, old_string, new_string)` - Edit file
   - `list_files(directory)` - List directory contents

3. **ShellTools**
   - `run_command(command, timeout)` - Execute shell commands

4. **Logger**
   - Colored, timestamped logging
   - Different levels: INFO, SUCCESS, ERROR, TOOL

### Function Calling Flow

```
User Input
    ↓
OpenAI API (with tool definitions)
    ↓
[If tool needed]
    ↓
Execute Tool Locally
    ↓
Send Result to OpenAI
    ↓
[Repeat if more tools needed]
    ↓
Final Response to User
```

## Advanced Usage

### Programmatic Usage

```python
import asyncio
from simple_code_agent import CodeAgent

async def main():
    api_key = "your-api-key"
    agent = CodeAgent(api_key=api_key, model="gpt-4-turbo-preview")

    response = await agent.process_message(
        "Create a Python file that calculates fibonacci numbers"
    )
    print(response)

asyncio.run(main())
```

### Custom Tool Extensions

You can extend the agent by:

1. Adding new tool definitions to `CodeAgent.TOOLS`
2. Implementing the tool logic in `_execute_tool()`
3. Creating a new tool class (like FileTools or ShellTools)

Example:

```python
# Add to TOOLS list
{
    "type": "function",
    "function": {
        "name": "search_web",
        "description": "Search the web for information",
        "parameters": {
            "type": "object",
            "properties": {
                "query": {"type": "string", "description": "Search query"}
            },
            "required": ["query"]
        }
    }
}

# Add to _execute_tool()
elif tool_name == "search_web":
    return self.web_tools.search(arguments["query"])
```

## Configuration

### Environment Variables

- `OPENAI_API_KEY` - Your OpenAI API key (required)
- `OPENAI_MODEL` - Model to use (default: gpt-4-turbo-preview)

### Supported Models

Any OpenAI model with function calling support:
- gpt-4-turbo-preview (recommended)
- gpt-4-1106-preview
- gpt-3.5-turbo-1106

## Troubleshooting

### "Module not found: aiohttp"
```bash
pip install aiohttp
```

### "OpenAI API error: 401"
Check that your API key is set correctly:
```bash
echo $OPENAI_API_KEY
```

### "Command timed out"
Increase the timeout parameter or use a faster command:
```python
# Default timeout is 30 seconds
# The agent can request longer timeouts if needed
```

### Tool execution fails
- Check file permissions
- Verify file paths are correct
- Ensure commands are valid for your OS

## Safety Considerations

⚠️ **Important Safety Notes:**

1. **Shell Command Execution**: The agent can run ANY shell command. Be careful what you ask it to do.

2. **File Operations**: The agent can overwrite files. Always backup important data.

3. **API Costs**: OpenAI API calls cost money. Monitor your usage.

4. **Sandboxing**: Consider running in a Docker container or VM for safety.

5. **Code Review**: Always review generated code before running it in production.

## Examples

### Example 1: Create a Web Scraper

```
You: Create a Python web scraper that fetches the title from a URL using requests and beautifulsoup4
```

### Example 2: Data Analysis Script

```
You: Create a script that reads a CSV file, calculates statistics, and saves a summary
```

### Example 3: Project Setup

```
You: Set up a new Flask project with:
- app.py with a hello world route
- requirements.txt
- A simple HTML template
- Test that it works with curl
```

## Tips for Best Results

1. **Be Specific**: "Create a function that validates email addresses using regex" is better than "make an email checker"

2. **Break Down Complex Tasks**: For very complex tasks, break them into steps

3. **Provide Context**: "In the Flask app we just created, add a new route..." works better with conversation history

4. **Verify Results**: Ask the agent to "show me the file" or "run tests" to verify

5. **Use Clear Commands**: The clearer your request, the better the result

## Comparison with Claude Code

This is a simplified version focusing on core features:

| Feature | Simple Code Agent | Claude Code |
|---------|------------------|-------------|
| File Read/Write | ✓ | ✓ |
| Shell Commands | ✓ | ✓ |
| Code Execution | ✓ (via shell) | ✓ |
| Error Recovery | Basic | Advanced |
| Context Management | Basic | Advanced |
| Security Sandboxing | ❌ | ✓ |
| Interactive Debugging | ❌ | ✓ |
| Git Integration | Via shell | Native |

## Next Steps

- Add more sophisticated file operations (patch-based editing)
- Implement code execution sandboxing
- Add support for multiple conversation threads
- Implement token usage tracking
- Add streaming responses
- Create a web UI

## License

This is example code for educational purposes.
