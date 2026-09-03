
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++20
TARGET = framereader

# Source files (automatically finds all .cpp files in the directory)
SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)

# Default target (runs when you just type 'make')
all: debug

# --- Debug Target ---
# Adds -g for debugging, -O0 to disable optimization (keeps code linear for GDB)
debug: CXXFLAGS += -g -O0
debug: $(TARGET)

# --- Release (Non-Debug) Target ---
# Adds -O2 for high performance optimization, -DNDEBUG disables assert() macros
release: CXXFLAGS += -O2 -DNDEBUG
release: clean $(TARGET)

# Link the object files into the final executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Compile source files into object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all debug release clean
