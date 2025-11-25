CC = gcc
CFLAGS = -std=c99 -O2 -I/opt/homebrew/include -I/opt/homebrew/include/freetype2
LDFLAGS = -L/opt/homebrew/lib -lSDL2 -lfreetype -lm

# Directories
TRACKER_DIR = ../tracker
BUILD_DIR = build

# Include paths
INCLUDES = -Isrc \
           -I$(TRACKER_DIR)/src \
           -I$(TRACKER_DIR)/src/chipnomad_lib \
           -I$(TRACKER_DIR)/src/chips \
           -I$(TRACKER_DIR)/src/corelib \
           -I$(TRACKER_DIR)/external/ayumi

# Find source files (wildcards with exclusions)
SOURCES = src/main.c \
          src/audio.c \
          src/visuals.c \
          $(filter-out %/export_psg.c %/export_wav.c, $(wildcard $(TRACKER_DIR)/src/chipnomad_lib/*.c)) \
          $(TRACKER_DIR)/src/chips/chip_ay.c \
          $(wildcard $(TRACKER_DIR)/external/ayumi/*.c) \
          $(TRACKER_DIR)/platforms/shared/corelib_file.c \
          $(TRACKER_DIR)/src/utils.c

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

