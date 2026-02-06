# FreeType Font Generator

TTF to ChipNomad bitmap font converter using FreeType library.

## Build

```bash
make
```

## Usage

```bash
./generate font.ttf output.cnfont
```

## Output

Generates a `.cnfont` file with 5 resolutions: 12x16, 16x24, 24x36, 32x48, 48x54.
Also generates `font_preview.png` showing all resolutions.
