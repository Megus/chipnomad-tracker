CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I/opt/homebrew/include/freetype2
LIBS = -lfreetype -L/opt/homebrew/lib

TARGET = generate
SOURCE = generate.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE) $(LIBS)

clean:
	rm -f $(TARGET) font*.c font_preview.png

test: $(TARGET)
	./$(TARGET) ../font-generator/MxPlus_IBM_VGA_9x16.ttf

.PHONY: all clean test