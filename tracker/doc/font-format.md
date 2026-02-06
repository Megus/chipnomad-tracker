# ChipNomad Font Format

ChipNomad supports custom fonts in `.cnfont` text format. You can create your own fonts or use the included fonts from the `fonts/` folder.

## Loading Fonts

Go to Settings → Load font and browse for a `.cnfont` file. The selected font will be saved and loaded automatically on app start.

## File Format (.cnfont)

ChipNomad font files use a simple text format:

```
name: Font Name

resolution: 12x16
20 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
06 00 06 00 06 00 06 00 06 00 06 00 04 00 04 00 00 00 00 00 00 00 06 00 00 00 00 00 00 00 00 00
...

resolution: 24x36
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
...
```

### Format Rules:
- First line: `name: <font name>` (max 15 characters)
- Blank line separator
- Each resolution starts with `resolution: <width>x<height>`
- Followed by 95 lines of hex data (one per character, space-separated bytes)
- Characters are in ASCII order (32-126): space, !, ", #, ..., ~
- Each line contains (width/8 rounded up) × height bytes
- Blank line between resolutions
- Comments start with `#` and are ignored
- Empty lines are ignored

### Creating Custom Fonts

You can create custom fonts from TTF files using the font generator tool in the `font-generator/` directory. Not all resolutions need to be provided - ChipNomad will use the best available resolution for the screen size.
