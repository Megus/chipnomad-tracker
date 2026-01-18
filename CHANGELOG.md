# ChipNomad changelog

## v0.1.0b (Not yet released)

- *PLATFORM*: Linux x86_64 package for Linux desktops and Steam Deck
- Vortex Tracker 2 tracks (.vt2) import (by [Pator](https://github.com/paator))
- Gamepad support for desktop builds
- Support QWERTZ and QWERTY keyboard layouts for desktop builds (by [koppi](https://github.com/koppi))
- Support for 2x and 3x AY/YM chips
- Linear pitch option (pitch tables are defined in cents)
- SNG FX to jump between song positions
- HOP FX now supports conditional loops both in Tables and Phrases
- ARP should work with octaves of any size, not just 12 notes
- *BREAKING CHANGE*: PIT now sets offset in semitones. New FIN command sets fine pitch offset
- Schematic waveform display
- Mute/solo tracks (B + Select/Start on Song screen. Release B first to keep mute/solo)
- Clean up of unused instruments, unused/duplicate phrases and chains
- Chain and Phrase screens show asterisk next to chain/phrase number if it's used elsewhere in the song
- B+A on an empty cell at the Song screen now moves the whole column up (same as in LSDJ and M8)
- Project and Instrument save functions now check for empty filename before saving
- AY/YM emulator filter quality setting (lower quality - lower CPU load)
- Looping cursor in the file browser
- Color theme edit, load, and save
- *FIX*: Chip settings were not initialized when loading a project
- *FIX*: UI was monochrome in RG35xx build
- *FIX*: All saved values are correctly reset on loading or creating a new project now
- *FIX*: Multi edit bug on FX columns in phrases and tables
- *FIX*: Project title is lost when you load a project with an empty author
- *FIX*: Chain deep clone created one more copy of a chain

## v0.0.3a (November 22, 2025)

- *PLATFORM*: proper macOS app bundle with the icon
- Added support for different screen resolutions (bonus: Mac Retina display support)
- New font and a convenient bitmap font generator for all screen resolutions from TTF fonts
- Copy/cut/paste functionality on Song, Chain, Phrase, Table screens
- Deep cloning chains (clones both chain and phrases in the chain)
- Edit multiple values in selection mode on Song, Chain, Phrase, Table screens
- Copy/paste instruments
- Save/load instruments
- Vortex Tracker 2 instruments (.vts) import (by [Pator](https://github.com/paator))
- Instrument pool screen with instrument reordering functionality
- Instrument preview on Instrument and Instrument Pool screens (A + Start)
- Additional FX help on FX selection screen
- New TXH effect: same as THO, but for aux table. THO now jumps in the instrument table only.
- Special case for TIC: when TIC is on the last row in the table, it sets the column speed on table start
- Special case for NOA: when NOA value is FF, it stops noise period output in the track/instrument. Convenient for resolving noise period conflicts
- Improved logic for finding next empty chain/phrase - looking for item not yet referenced in the project
- Settings screen with two functions: AY phasing conflict highlight, and Quit button
- Lowercase character entry in all text fields
- Create folder function in the file browser
- Show BPM for tick rate (only for default groove with 6 ticks per phrase row)
- 0.75MHz AY/YM clock option
- Export to WAV and PSG (AY register dump) formats
- *FIX*: Crash on deleting a chain under the playhead during playback
- *FIX*: OFF in note field stopped active track FX
- *FIX*: Playback now stops on project load and song position is reset to start
- *FIX*: Cursor could be drawn incorrectly at some screens
- *FIX*: Chain transpose column color could be wrong
- *FIX*: GGR command was working incorrectly

## v0.0.2a (October 11, 2025)

- *PLATFORM*: PortMaster build
- Project settings screen
- AY/YM chip settings
- Project save/load
- Pitch table save/load
- ARP and ARC effects for arpeggio (by [laamaa](https://github.com/laamaa))
- *FIX*: Random crash on app startup because of audio callback race condition (by [Alexander Kovalenko](https://github.com/alexanderk23))
- *FIX*: Random loss of instrument and volume values in Phrase editor
- *FIX*: App crash when deleting a chain or a phrase under playhead during playback
- *FIX*: Instruments of NONE type now don't output any sound
- *FIX*: PVB started from the lowest pitch offset instead of zero

## v0.0.1a (May 8, 2025)

- *PLATFORM*: pre-2024 Anbernic RG35xx with GarlicOS 1, Windows, macOS
- Core editing functionality
- Project auto save/load
