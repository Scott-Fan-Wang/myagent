#!/usr/bin/env python3
"""
Simple Code Agent - A minimal implementation of an AI coding assistant
using OpenAI's API with function calling capabilities.
"""

import asyncio
import aiohttp
import json
import os
import subprocess
import sys
import argparse
import threading
import select
import termios
import tty
from typing import Dict, List, Any, Optional
from datetime import datetime


class Logger:
    """Simple logger for tracking agent operations."""

    def __init__(self, verbose: bool = True):
        self.verbose = verbose

    def log(self, message: str, level: str = "INFO"):
        """Log a message with timestamp."""
        timestamp = datetime.now().strftime("%H:%M:%S")
        color_codes = {
            "INFO": "\033[36m",  # Cyan
            "SUCCESS": "\033[32m",  # Green
            "ERROR": "\033[31m",  # Red
            "TOOL": "\033[33m",  # Yellow
            "RESET": "\033[0m"
        }
        color = color_codes.get(level, "")
        reset = color_codes["RESET"]
        print(f"{color}[{timestamp}] {level}: {message}{reset}")


class FileTools:
    """Tools for file operations."""

    @staticmethod
    def read_file(file_path: str) -> Dict[str, Any]:
        """Read a file and return its contents."""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            return {
                "success": True,
                "content": content,
                "message": f"Successfully read {len(content)} characters from {file_path}"
            }
        except Exception as e:
            return {
                "success": False,
                "error": str(e),
                "message": f"Failed to read {file_path}: {str(e)}"
            }

    @staticmethod
    def write_file(file_path: str, content: str) -> Dict[str, Any]:
        """Write content to a file."""
        try:
            # Create directory if it doesn't exist
            os.makedirs(os.path.dirname(file_path) if os.path.dirname(file_path) else '.', exist_ok=True)
            with open(file_path, 'w', encoding='utf-8') as f:
                f.write(content)
            return {
                "success": True,
                "message": f"Successfully wrote {len(content)} characters to {file_path}"
            }
        except Exception as e:
            return {
                "success": False,
                "error": str(e),
                "message": f"Failed to write {file_path}: {str(e)}"
            }

    @staticmethod
    def edit_file(file_path: str, old_string: str, new_string: str) -> Dict[str, Any]:
        """Replace old_string with new_string in a file."""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()

            if old_string not in content:
                return {
                    "success": False,
                    "message": f"Could not find the specified text in {file_path}"
                }

            new_content = content.replace(old_string, new_string, 1)

            with open(file_path, 'w', encoding='utf-8') as f:
                f.write(new_content)

            return {
                "success": True,
                "message": f"Successfully edited {file_path}"
            }
        except Exception as e:
            return {
                "success": False,
                "error": str(e),
                "message": f"Failed to edit {file_path}: {str(e)}"
            }

    @staticmethod
    def list_files(directory: str = ".") -> Dict[str, Any]:
        """List files in a directory."""
        try:
            files = []
            for item in os.listdir(directory):
                path = os.path.join(directory, item)
                files.append({
                    "name": item,
                    "type": "directory" if os.path.isdir(path) else "file",
                    "size": os.path.getsize(path) if os.path.isfile(path) else None
                })
            return {
                "success": True,
                "files": files,
                "message": f"Found {len(files)} items in {directory}"
            }
        except Exception as e:
            return {
                "success": False,
                "error": str(e),
                "message": f"Failed to list {directory}: {str(e)}"
            }


class ShellTools:
    """Tools for shell command execution."""

    @staticmethod
    def run_command(command: str, timeout: int = 30) -> Dict[str, Any]:
        """Execute a shell command and return the output."""
        try:
            result = subprocess.run(
                command,
                shell=True,
                capture_output=True,
                text=True,
                timeout=timeout
            )
            return {
                "success": result.returncode == 0,
                "stdout": result.stdout,
                "stderr": result.stderr,
                "returncode": result.returncode,
                "message": f"Command executed with return code {result.returncode}"
            }
        except subprocess.TimeoutExpired:
            return {
                "success": False,
                "error": "Command timed out",
                "message": f"Command timed out after {timeout} seconds"
            }
        except Exception as e:
            return {
                "success": False,
                "error": str(e),
                "message": f"Failed to execute command: {str(e)}"
            }


class CodeAgent:
    """Main agent class that interfaces with OpenAI API."""

    # Tool definitions for OpenAI function calling
    TOOLS = [
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
                            "description": "Timeout in seconds (default: 30)"
                        }
                    },
                    "required": ["command"]
                }
            }
        }
    ]

    def __init__(self, base_url: str, api_key: str, model: str = "gpt-4-turbo-preview", verbose: bool = True, max_iterations: int = 99):
        self.base_url = base_url
        self.api_key = api_key
        self.model = model
        self.max_iterations = max_iterations
        self.logger = Logger(verbose)
        self.conversation_history: List[Dict[str, Any]] = []
        self.file_tools = FileTools()
        self.shell_tools = ShellTools()
        self.should_stop = False
        self.keyboard_thread = None

    def _execute_tool(self, tool_name: str, arguments: Dict[str, Any]) -> Dict[str, Any]:
        """Execute a tool based on its name and arguments."""
        self.logger.log(f"Executing tool: {tool_name} with args: {json.dumps(arguments, indent=2)}", "TOOL")

        if tool_name == "read_file":
            return self.file_tools.read_file(arguments["file_path"])
        elif tool_name == "write_file":
            return self.file_tools.write_file(arguments["file_path"], arguments["content"])
        elif tool_name == "edit_file":
            return self.file_tools.edit_file(
                arguments["file_path"],
                arguments["old_string"],
                arguments["new_string"]
            )
        elif tool_name == "list_files":
            directory = arguments.get("directory", ".")
            return self.file_tools.list_files(directory)
        elif tool_name == "run_command":
            timeout = arguments.get("timeout", 30)
            return self.shell_tools.run_command(arguments["command"], timeout)
        else:
            return {"success": False, "error": f"Unknown tool: {tool_name}"}

    async def call_openai(self, messages: List[Dict[str, Any]]) -> Dict[str, Any]:
        """Make an async call to OpenAI API."""
        url = f"{self.base_url}/chat/completions"
        headers = {
            "Authorization": f"Bearer {self.api_key}",
            "Content-Type": "application/json"
        }

        payload = {
            "model": self.model,
            "messages": messages,
            "tools": self.TOOLS,
            "tool_choice": "auto"
        }

        async with aiohttp.ClientSession() as session:
            async with session.post(url, headers=headers, json=payload) as response:
                if response.status != 200:
                    error_text = await response.text()
                    raise Exception(f"OpenAI API error: {response.status} - {error_text}")
                return await response.json()

    async def process_message(self, user_message: str) -> str:
        """Process a user message and handle tool calls."""
        # Add user message to history
        self.conversation_history.append({
            "role": "user",
            "content": user_message
        })

        iteration = 0

        while iteration < self.max_iterations and not self.should_stop:
            iteration += 1

            # Call OpenAI API
            self.logger.log(f"Calling OpenAI API (iteration {iteration})...")
            response = await self.call_openai(self.conversation_history)

            message = response["choices"][0]["message"]
            finish_reason = response["choices"][0]["finish_reason"]

            # Add assistant message to history
            self.conversation_history.append(message)

            # Check if there are tool calls
            if finish_reason == "tool_calls" and message.get("tool_calls"):
                self.logger.log(f"Assistant requested {len(message['tool_calls'])} tool call(s)", "INFO")

                # Execute each tool call
                for tool_call in message["tool_calls"]:
                    # Check if ESC was pressed before executing each tool
                    if self.should_stop:
                        return "[Agent stopped by user request]"
                    
                    tool_name = tool_call["function"]["name"]
                    tool_args = json.loads(tool_call["function"]["arguments"])

                    # Execute the tool
                    result = self._execute_tool(tool_name, tool_args)

                    # Log result
                    if result.get("success"):
                        self.logger.log(result.get("message", "Tool executed successfully"), "SUCCESS")
                    else:
                        self.logger.log(result.get("message", "Tool execution failed"), "ERROR")

                    # Add tool result to conversation
                    self.conversation_history.append({
                        "role": "tool",
                        "tool_call_id": tool_call["id"],
                        "content": json.dumps(result)
                    })

                # Continue loop to get assistant's next response
                continue

            # If no tool calls, return the assistant's message
            if message.get("content"):
                return message["content"]
            else:
                return "[Assistant provided no text response]"

        if self.should_stop:
            return "[Agent stopped by user request]"
        else:
            return "[Max iterations reached - the assistant may need more steps to complete the task]"

    def _start_keyboard_monitor(self):
        """Start a background thread to monitor keyboard input for ESC key."""
        def monitor_esc():
            # Save terminal settings
            old_settings = termios.tcgetattr(sys.stdin)
            try:
                tty.setraw(sys.stdin.fileno())
                while not self.should_stop:
                    if select.select([sys.stdin], [], [], 0.1)[0]:
                        key = sys.stdin.read(1)
                        if key == '\x1b':  # ESC key
                            self.should_stop = True
                            self.logger.log("ESC key detected - stopping agent", "INFO")
                            break
            finally:
                # Restore terminal settings
                termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)

        self.keyboard_thread = threading.Thread(target=monitor_esc, daemon=True)
        self.keyboard_thread.start()

    def _stop_keyboard_monitor(self):
        """Stop the keyboard monitoring thread."""
        self.should_stop = True
        if self.keyboard_thread and self.keyboard_thread.is_alive():
            self.keyboard_thread.join(timeout=1.0)

    async def run(self):
        """Run the interactive CLI loop."""
        print("\n" + "="*60)
        print("Simple Code Agent - OpenAI powered coding assistant")
        print("="*60)
        print("Commands:")
        print("  - Type your request and press Enter")
        print("  - Type 'exit' or 'quit' to exit")
        print("  - Type 'clear' to clear conversation history")
        print("  - Press ESC key to stop current agent work")
        print("="*60 + "\n")

        # Add system prompt
        system_prompt = """You are a helpful coding assistant with access to file operations and shell commands.
You can read, write, and edit files, as well as execute shell commands.
Always explain what you're doing before using tools.
When you complete a task, summarize what was done."""

        self.conversation_history.append({
            "role": "system",
            "content": system_prompt
        })

        while True:
            try:
                # Reset stop flag for new request
                self.should_stop = False
                
                user_input = input("\n\033[1;34mYou:\033[0m ").strip()

                if not user_input:
                    continue

                if user_input.lower() in ['exit', 'quit']:
                    print("\nGoodbye!")
                    break

                if user_input.lower() == 'clear':
                    self.conversation_history = [{
                        "role": "system",
                        "content": system_prompt
                    }]
                    print("Conversation history cleared.")
                    continue

                # Start keyboard monitoring for ESC key
                self._start_keyboard_monitor()

                # Process the message
                response = await self.process_message(user_input)

                # Stop keyboard monitoring
                self._stop_keyboard_monitor()

                # Print assistant response
                print(f"\n\033[1;32mAssistant:\033[0m {response}")

            except KeyboardInterrupt:
                print("\n\nInterrupted. Type 'exit' to quit.")
                continue
            except Exception as e:
                self.logger.log(f"Error: {str(e)}", "ERROR")
                print(f"\n\033[1;31mError:\033[0m {str(e)}")


async def main():
    """Main entry point."""
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description="Simple Code Agent - OpenAI powered coding assistant")
    parser.add_argument("-m", "--max_iter", type=int, default=99,
                        help="Maximum number of iterations (default: 99)")
    args = parser.parse_args()

    # Get API key from environment or user input
    api_key = os.environ.get("API_KEY", "your-api-key-here")
    base_url = os.environ.get("BASE_URL", "https://api.deepseek.com/v1")

    if not api_key:
        print("OpenAI API key not found in environment.")
        api_key = input("Please enter your OpenAI API key: ").strip()

    if not api_key:
        print("Error: API key is required.")
        sys.exit(1)

    # Get model from environment or use default
    model = os.environ.get("OPENAI_MODEL", "deepseek-chat")

    # Create and run agent
    agent = CodeAgent(base_url=base_url, api_key=api_key, model=model, verbose=True, max_iterations=args.max_iter)
    await agent.run()


if __name__ == "__main__":
    asyncio.run(main())
