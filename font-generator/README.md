# Bitmap Font Generator

This tool generates bitmap fonts from TTF files for the ChipNomad tracker project.

## Features

- Generates bitmap fonts in multiple sizes: 12x16, 16x24, 24x36, 32x48, 48x54 pixels
- Full ASCII character set (codes 32-127)
- Outputs C files compatible with ChipNomad font format
- Row-by-row bitmap data with 8 pixels per byte

## Installation

```bash
npm install
```

## Usage

```bash
node generate.js <font-file.ttf> [output-directory]
```

### Examples

```bash
# Generate fonts from Arial.ttf to ./output directory
node generate.js Arial.ttf

# Generate fonts to specific directory
node generate.js /path/to/font.ttf ./fonts
```

## Output

The tool generates C files with the following naming convention:
- `font_12x16.c`
- `font_16x24.c`
- `font_24x36.c`
- `font_32x48.c`
- `font_48x54.c`

Each file contains a `uint8_t` array with the bitmap data for all ASCII characters (32-127).

## Font Format

The generated bitmap format matches the existing ChipNomad font format:
- Row-by-row scanning
- Each byte represents 8 horizontal pixels
- MSB is the leftmost pixel
- 1 = pixel on, 0 = pixel off

## Requirements

- Node.js
- Canvas library (automatically installed via npm)
- TTF font file