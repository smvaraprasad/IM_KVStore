
# Compiler & Flags
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -pthread -Isrc

# Directories
SRC_DIR   := src
BUILD_DIR := build
DATA_DIR  := data

# Target Binary
TARGET   := storage_engine

# Automatically collect all .cpp files in src/
SRCS     := $(wildcard $(SRC_DIR)/*.cpp)
OBJS     := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))

# Default Rule
all: create_dirs $(TARGET)

# Create build and data directories if they don't exist
create_dirs:
	@mkdir -p $(BUILD_DIR) $(DATA_DIR)

# Link object files into final binary
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "[Build Complete] Output binary: ./$(TARGET)"

# Compile source files from src/ into build/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | create_dirs
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean object files and binary executable
clean:
	rm -rf $(BUILD_DIR) $(TARGET)
	@echo "[Clean Complete] Removed build artifacts."

# Reset build artifacts AND clear data logs
clean-all: clean
	rm -rf $(DATA_DIR)/*.wal $(DATA_DIR)/*.tmp
	@echo "[Reset Complete] Cleared database logs in $(DATA_DIR)/."

.PHONY: all create_dirs clean clean-all
