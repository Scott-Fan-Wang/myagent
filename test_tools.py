#!/usr/bin/env python3
"""
Test script to verify tool functionality without needing OpenAI API key
"""

import os
import tempfile
from simple_code_agent import FileTools, ShellTools, Logger

def test_file_tools():
    """Test file operations."""
    logger = Logger(verbose=True)
    file_tools = FileTools()

    logger.log("Testing File Tools", "INFO")

    # Create a temporary file for testing
    with tempfile.NamedTemporaryFile(mode='w', delete=False, suffix='.txt') as f:
        test_file = f.name

    try:
        # Test write
        logger.log("Test 1: Writing file", "INFO")
        result = file_tools.write_file(test_file, "Hello, World!\nThis is a test.")
        print(f"  Result: {result}")
        assert result['success'], "Write failed"

        # Test read
        logger.log("Test 2: Reading file", "INFO")
        result = file_tools.read_file(test_file)
        print(f"  Content: {result.get('content', 'N/A')[:50]}...")
        assert result['success'], "Read failed"
        assert "Hello, World!" in result['content'], "Content mismatch"

        # Test edit
        logger.log("Test 3: Editing file", "INFO")
        result = file_tools.edit_file(test_file, "World", "Python")
        print(f"  Result: {result}")
        assert result['success'], "Edit failed"

        # Verify edit
        result = file_tools.read_file(test_file)
        assert "Hello, Python!" in result['content'], "Edit didn't work"

        # Test list files
        logger.log("Test 4: Listing files", "INFO")
        result = file_tools.list_files(os.path.dirname(test_file) or ".")
        print(f"  Found {len(result.get('files', []))} items")
        assert result['success'], "List files failed"

        logger.log("All file tool tests passed!", "SUCCESS")

    finally:
        # Cleanup
        if os.path.exists(test_file):
            os.unlink(test_file)


def test_shell_tools():
    """Test shell command execution."""
    logger = Logger(verbose=True)
    shell_tools = ShellTools()

    logger.log("Testing Shell Tools", "INFO")

    # Test simple command
    logger.log("Test 1: Running 'echo' command", "INFO")
    result = shell_tools.run_command("echo 'Hello from shell'")
    print(f"  Output: {result.get('stdout', '').strip()}")
    assert result['success'], "Echo command failed"
    assert "Hello from shell" in result['stdout'], "Output mismatch"

    # Test command with output
    logger.log("Test 2: Running 'ls' command", "INFO")
    result = shell_tools.run_command("ls -la")
    print(f"  Lines of output: {len(result.get('stdout', '').split(chr(10)))}")
    assert result['success'], "ls command failed"

    # Test Python command
    logger.log("Test 3: Running Python command", "INFO")
    result = shell_tools.run_command("python3 -c 'print(2 + 2)'")
    print(f"  Output: {result.get('stdout', '').strip()}")
    assert result['success'], "Python command failed"
    assert "4" in result['stdout'], "Math didn't work"

    logger.log("All shell tool tests passed!", "SUCCESS")


def main():
    """Run all tests."""
    print("\n" + "="*60)
    print("Testing Simple Code Agent Tools")
    print("="*60 + "\n")

    try:
        test_file_tools()
        print()
        test_shell_tools()

        print("\n" + "="*60)
        print("✓ All tests passed successfully!")
        print("="*60 + "\n")

    except AssertionError as e:
        print(f"\n✗ Test failed: {e}\n")
        return 1
    except Exception as e:
        print(f"\n✗ Unexpected error: {e}\n")
        return 1

    return 0


if __name__ == "__main__":
    exit(main())
