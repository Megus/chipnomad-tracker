# ChipNomad Tracker

ChipNomad is a multi-platform tracker with LSDJ-like interface designed for creating chiptune music. Primary target platforms are handheld game consoles like Anbernic RG35xx.

[ChipNomad manual](https://chipnomad.org/manual/)

[Join ChipNomad Discord server](https://discord.gg/PJarAn2QCW)

## Currently supported platforms

- [PortMaster](https://portmaster.games) ([list of supported devices](https://portmaster.games/supported-devices.html))
- Pre-2024 Anbernic RG35xx with GarlicOS 1.4
- macOS
- Windows
- Linux (x86_64 and ARM)
- Android (works best on handhelds, phone and tablet touch screen input has issues)

## Building

See [tracker/README.md](tracker/README.md) for build instructions.

## Hardware Requirements

ChipNomad is written in C/C++ and can be ported to any platform that satisfies these requirements:

- Display capable of 40x20 characters
- 8 buttons: LEFT, RIGHT, UP, DOWN, A, B, START, SELECT
- Stereo 16-bit audio output
- CPU capable of running chip emulation or a platform with real chips

## Background

I (Megus) started this project because I want to make real chiptune music on the go. LSDj is amazing but it's only
for GameBoy music. I come from ZX Spectrum scene, so I want to make music for AY-3-8910/YM2149F chips. I have M8 Tracker
and I love it. This is how I chose the approach to the UI.

## Acknowledgements

ChipNomad wouldn't be possible without:

- [LSDj](https://www.littlesounddj.com/lsd/index.php) by Johan Kotlinski
- [Dirtywave M8 Tracker](https://dirtywave.com) by Trash80
- [Ayumi AY chip emulator](https://github.com/true-grue/ayumi) by Peter Sovietov
