# Simple Code Agent

A minimal implementation of an AI coding assistant using OpenAI's API with function calling capabilities. Built with pure Python and aiohttp.

## Features

- **File Operations**
  - Read files
  - Write/create files
  - Edit files (find and replace)
  - List directory contents

- **Shell Commands**
  - Execute shell commands with timeout support
  - Capture stdout and stderr

- **Interactive CLI**
  - Conversational interface
  - Command history maintenance
  - Colored logging output

## Installation

1. Install dependencies:
```bash
pip install -r requirements.txt
```

2. Set your OpenAI API key:
```bash
export OPENAI_API_KEY='your-api-key-here'
```

Optionally, set a specific model (default is gpt-4-turbo-preview):
```bash
export OPENAI_MODEL='gpt-4-turbo-preview'
```

## Usage

Run the agent:
```bash
python simple_code_agent.py
```

Or make it executable:
```bash
chmod +x simple_code_agent.py
./simple_code_agent.py
```

### Example Commands

Once running, you can ask the agent to:

- **"Read the file example.txt"**
- **"Create a Python script called hello.py that prints 'Hello World'"**
- **"List all files in the current directory"**
- **"Run the command 'ls -la'"**
- **"Edit test.py and change 'foo' to 'bar'"**
- **"Create a calculator function in calc.py"**

### CLI Commands

- Type your request and press Enter
- Type `exit` or `quit` to exit
- Type `clear` to clear conversation history

## Architecture

The implementation consists of:

1. **CodeAgent**: Main class that handles OpenAI API communication
2. **FileTools**: Implements file read/write/edit/list operations
3. **ShellTools**: Implements shell command execution
4. **Logger**: Provides colored, timestamped logging

The agent uses OpenAI's function calling feature to:
1. Receive user requests
2. Decide which tools to use
3. Execute tools locally
4. Return results to the LLM
5. Generate natural language responses

## How It Works

1. User sends a message
2. Message is sent to OpenAI API with tool definitions
3. If OpenAI requests tool calls, they are executed locally
4. Results are sent back to OpenAI
5. Process repeats until OpenAI provides a final text response
6. Response is shown to the user

## Supported OpenAI Models

- gpt-4-turbo-preview (default)
- gpt-4-1106-preview
- gpt-3.5-turbo-1106
- Any other OpenAI model that supports function calling

## Safety Notes

- The agent can execute arbitrary shell commands - use with caution
- File operations are unrestricted - be careful with write/edit operations
- Always review code before running it
- Consider running in a sandboxed environment for testing

## Example Session

```
You: Create a file called test.txt with the content "Hello World"