---
title: Manual
description: ChipNomad Manual
layout: single
---

# ChipNomad User Manual

**ChipNomad** is a multi-platform chiptune [tracker](https://en.wikipedia.org/wiki/Music_tracker). It is heavily inspired by [LSDj](https://www.littlesounddj.com/lsd/index.php) and [Dirtywave M8](https://dirtywave.com). While ChipNomad is available for desktop platforms (Windows, macOS, Linux), it is designed for handheld consoles, such as Anbernic RG35xx, TrimUI Brick, etc.

ChipNomad currently supports only [AY-3-8910/YM2149F](https://en.wikipedia.org/wiki/General_Instrument_AY-3-8910) chips with more chips coming in the future.

- [Introduction](#introduction)
- [Installation](installation/)
- [Common Controls](#common-controls)
- [Song Screen](song-screen/)
- [Chain Screen](chain-screen/)
- [Phrase Screen](phrase-screen/)
- [Groove Screen](groove-screen/)
- [Instrument Screen](instrument-screen/)
- [Table Screen](table-screen/)
- [Project Screen](project-screen/)
- [Settings Screen](settings-screen/)
- [Tracker FX reference](tracker-fx/)

## Introduction

The core UI concept and the song structure is the same as in LSDj or M8 Tracker, so if you used them, then you will feel at home. LSDj-style trackers use different song structure compared to traditional trackers. In a traditional tracker the song is built as a list of patterns where each pattern has multiple tracks and a fixed number of rows. ChipNomad, like all LSDj-style trackers, uses a hierarchical structure: each song track is a list of *chains*, chains are groups of *phrases*, phrases contain notes and effects (commands). This structure works really well with small screens of handheld consoles.

ChipNomad screens are laid out in the map. Each screen is dedicated to a singe function.

```
P G
SCPIT
S  P
```

- [**P**roject](project-screen/) settings (chip type, tick rate, etc)
- [**S**ong](song-screen/) sequencing
- [**S**ettings](settings-screen/)
- [**C**hain](chain-screen/) editor
- [**P**hrase](phrase-screen/) editor
- [**I**nstrument](instrument-screen/) editor and Instrument **P**ool
- [**T**able](table-screen/) editor
- [**G**roove](groove-screen/) editor

## Common controls

If you're familiar with LSDj or M8, you can expect most shortcuts to work the same in ChipNomad.

ChipNomad uses 8 logical buttons. The default mapping on consoles and desktops:

| Button     | Consoles    | Desktop      |
|------------|-------------|--------------|
| **LEFT**   | D-Pad Left  | Cursor Left  |
| **RIGHT**  | D-Pad Right | Cursor Right |
| **UP**     | D-Pad Up    | Cursor Up    |
| **DOWN**   | D-Pad Down  | Cursor Down  |
| **EDIT**   | A           | Z            |
| **OPT**    | B           | X            |
| **PLAY**   | Start       | Space        |
| **SHIFT**  | Select      | Shift        |

You can define your own keys and have up to 3 physical buttons mapped to each logical button.

To quit ChipNomad use the button at the Setings screen or press MENU + X on consoles.

### Navigation

- **DIRECTION**: move cursor
- Hold **SHIFT** + \[**DIRECTION**\]: screen navigation
- Hold **OPT** + \[**DIRECTION**\]: context navigation (screen-specific)

### Editing

- **EDIT**: insert/enter value
- Double-tap **EDIT**: create new item (chain, phrase, instrument)
- **EDIT** + \[**LEFT** or **RIGHT**\]: change value (fine)
- **EDIT** + \[**UP** or **DOWN**\]: change value (coarse)
- **OPT** + **EDIT**: cut value

### Selection and Clipboard

- **SHIFT** + **OPT**: enter selection mode
- **EDIT** + \[**DIRECTION**\]: multi-edit — edit all values in selection
- **OPT**: copy selection
- **OPT** + **EDIT**: cut selection
- **SHIFT** + **EDIT**: paste

### Playback

- **PLAY**: play from cursor
- **PLAY** (while playing): stop playback
- **SHIFT** + **PLAY**: play all tracks (outside Song screen)

## Project limits

- 256 song positions (00-FF)
- 255 chains (00-FE)
- 1024 phrases (000-3FF)
- Up to 10 tracks for multi-chip setups
- 128 instruments (00-7F)
- 255 tables: 00-7F - instrument tables, 80-FE - aux tables
- 32 grooves (00-1F)
