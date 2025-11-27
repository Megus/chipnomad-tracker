CC = gcc
CFLAGS = -std=c99 -O2 -I/opt/homebrew/include -I/opt/homebrew/include/freetype2
LDFLAGS = -L/opt/homebrew/lib -lSDL2 -lfreetype -lm

# Directories
BUILD_DIR = build

# Include paths
INCLUDES = -Isrc -I../chipnomad_lib

# Find source files
SOURCES = $(wildcard src/*.c) \
          $(filter-out %/export_psg.c %/export_wav.c, $(wildcard ../chipnomad_lib/*.c)) \
          $(wildcard ../chipnomad_lib/chips/*.c) \
          $(wildcard ../chipnomad_lib/external/ayumi/*.c) \
          ../tracker/platforms/shared/corelib_file.c

# Generate object files (flattened to build directory)
OBJECTS = $(addprefix $(BUILD_DIR)/, $(notdir $(SOURCES:.c=.o)))

# Target
TARGET = chipnomad_player

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# Generic build rule
$(BUILD_DIR)/%.o: 
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $(filter %/$(basename $(notdir $@)).c, $(SOURCES)) -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

