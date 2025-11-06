# Makefile for Simple Code Agent (C++) - Using clang++

CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2
LDFLAGS = -lpthread -lstdc++fs

TARGET = code_agent
SOURCES = main.cpp code_agent.cpp
HEADERS = code_agent.hpp
OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all clean rebuild test install

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET) with clang++..."
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile source files
%.o: %.cpp $(HEADERS)
	@echo "Compiling $< with clang++..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(OBJECTS) $(TARGET)
	rm -rf build
	@echo "Clean complete"

# Rebuild from scratch
rebuild: clean all

# Test compilation (compile only, no link)
test-compile: $(OBJECTS)
	@echo "Compilation test successful"

# Install to /usr/local/bin
install: $(TARGET)
	@echo "Installing $(TARGET) to /usr/local/bin..."
	install -m 755 $(TARGET) /usr/local/bin/
	@echo "Installation complete"

# Uninstall from /usr/local/bin
uninstall:
	@echo "Uninstalling $(TARGET)..."
	rm -f /usr/local/bin/$(TARGET)
	@echo "Uninstall complete"

# Debug build
debug: CXXFLAGS += -g -DDEBUG
debug: clean $(TARGET)
	@echo "Debug build complete"

# Show help
help:
	@echo "Simple Code Agent - Makefile (clang++ version)"
	@echo ""
	@echo "Usage:"
	@echo "  make            - Build the project"
	@echo "  make clean      - Remove build artifacts"
	@echo "  make rebuild    - Clean and rebuild"
	@echo "  make debug      - Build with debug symbols"
	@echo "  make install    - Install to /usr/local/bin"
	@echo "  make uninstall  - Remove from /usr/local/bin"
	@echo "  make help       - Show this help message"
	@echo ""
	@echo "Environment variables:"
	@echo "  CXX             - C++ compiler (default: clang++)"
	@echo "  CXXFLAGS        - Compiler flags"
	@echo "  LDFLAGS         - Linker flags"
