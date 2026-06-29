CXX = c++
CFLAGS = -std=c++17 -O2 -I/opt/homebrew/include -I/opt/homebrew/include/freetype2
LDFLAGS = -L/opt/homebrew/lib -lSDL2 -lfreetype -lm

# Directories
BUILD_DIR = build

# Include paths
INCLUDES = -Isrc -I../chipnomad_lib -I../chipnomad_lib/chips -I../chipnomad_lib/external/ayumi

# Find source files
SOURCES = $(wildcard src/*.cpp) \
          $(wildcard ../chipnomad_lib/*.cpp) \
          $(wildcard ../chipnomad_lib/chips/*.cpp) \
          $(wildcard ../chipnomad_lib/external/ayumi/*.cpp)

# Generate object files (flattened to build directory)
OBJECTS = $(addprefix $(BUILD_DIR)/, $(notdir $(SOURCES:.cpp=.o)))

# Target
TARGET = chipnomad_player

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

# Generic build rule
$(BUILD_DIR)/%.o:
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CFLAGS) $(INCLUDES) -c $(filter %/$(basename $(notdir $@)).cpp, $(SOURCES)) -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

