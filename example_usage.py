#!/usr/bin/env python3
"""
Example usage of the Simple Code Agent
"""

import asyncio
import os
from simple_code_agent import CodeAgent


async def example_programmatic_usage():
    """Example of using the agent programmatically."""

    # Get API key
    api_key = os.environ.get("OPENAI_API_KEY")
    if not api_key:
        print("Please set OPENAI_API_KEY environment variable")
        return

    # Create agent
    agent = CodeAgent(api_key=api_key, model="gpt-4-turbo-preview", verbose=True)

    print("="*60)
    print("Example 1: Create a Python file")
    print("="*60)

    response = await agent.process_message(
        "Create a Python file called 'greeting.py' that has a function to greet a user by name"
    )
    print(f"\nResponse: {response}\n")

    print("="*60)
    print("Example 2: Read the file")
    print("="*60)

    response = await agent.process_message(
        "Read the greeting.py file and show me its contents"
    )
    print(f"\nResponse: {response}\n")

    print("="*60)
    print("Example 3: Run Python code")
    print("="*60)

    response = await agent.process_message(
        "Run the greeting.py file with Python"
    )
    print(f"\nResponse: {response}\n")

    print("="*60)
    print("Example 4: List files")
    print("="*60)

    response = await agent.process_message(
        "List all Python files in the current directory"
    )
    print(f"\nResponse: {response}\n")


async def example_batch_operations():
    """Example of performing multiple operations."""

    api_key = os.environ.get("OPENAI_API_KEY")
    if not api_key:
        print("Please set OPENAI_API_KEY environment variable")
        return

    agent = CodeAgent(api_key=api_key, model="gpt-4-turbo-preview", verbose=True)

    # Create a mini project
    response = await agent.process_message("""
        Create a simple project structure:
        1. Create a directory called 'myproject'
        2. Inside it, create a file 'main.py' with a simple "Hello World" program
        3. Create a 'README.md' file describing the project
        4. List all files to confirm they were created
    """)

    print(f"\nFinal Response: {response}\n")


if __name__ == "__main__":
    print("Running programmatic usage example...\n")
    asyncio.run(example_programmatic_usage())

    print("\n" + "="*60)
    print("Running batch operations example...")
    print("="*60 + "\n")
    asyncio.run(example_batch_operations())
