# ChipNomad Tracker

ChipNomad will be a multi-chip tracker with LSDJ-like interface for handheld game consoles like Anbernic RG35xx.
As it's a pure C application built with SDL1.2, it can also be built for any other supported platform. Even SDL1.2
is not a strict requirement and it is possible to replace the small core library and build it for virtually anything.

Hardware requirements:

- 8 buttons: D-pad, A, B, Start, Select
- Can display 40x20 characters on the screen
- Can play stereo 16-bit audio

[ChipNomad Features and UI ideas](docs/ideas.md)

## Background

I (Megus) started this project because I want to make real chiptune music on the go. LSDJ is amazing but it's only
for GameBoy music. I come from ZX Spectrum scene, so I want to make music for AY-3-8910/YM2149F chips. I use M8 Tracker
every day and love it. This is how I chose the approach to the UI. I considered making an app for iOS/Android, but I find
using touchscreen for music making a painful experience and prefer a device with physical buttons.
I have Anbernic RG35xx with Garlic OS which allows installing 3rd party native apps. This is how I chose the platform.

## Acknowledgements

ChipNomad wouldn't be possible without:

- [LSDJ](https://www.littlesounddj.com/lsd/index.php) by Johan Kotlinski
- [M8 Tracker](https://dirtywave.com) by Trash80
- [RG35xx app template](https://github.com/anderson-/simplegfx) by Anderson Antunes
- [Ayumi AY chip emulator](https://github.com/true-grue/ayumi) by Peter Sovietov
