---
title: Tracker Effects
description: ChipNomad Manual - Tracker Effects
layout: single
---

# Tracker Effects

{{< toc >}}

## How FX work in ChipNomad

Each track can have multiple FX running simultaneously but only one instance of each FX per track. A few examples to explain this:

1. Put `PVB` in FX1 column in a phrase, and then put `PBN` in the same column lower in the phrase. Pitch bend won't stop the vibrato, both FX will run at the same time.
2. Put `PBN` in FX2 column in a phrase, then put another `PBN` lower in the phrase in FX1 column. The second `PBN` will simply update the pitch bend speed in the track. To stop pitch bend later you can put `PBN 00` in any FX column.

Effects can be stopped with the value 00. Only `TBL` and `TBX` effects are an exception, they're stopped with value FF.

FX in phrases take priority over FX in tables. For example, if you put `AYM 01` in a table, and then in phrase you put `AYM 03`, the value from the phrase will take over. Overall priority order: instrument table, aux table, phrase.

FX are fully reset when a new instrument is triggered. When you trigger a note with an empty instrument value, currently active FX will either continue (e.g. arpeggio, vibrato) or restart (e.g. pitch bend).

## Sequencer Effects

These effects are available for all instrument types.

### `ARP XY` — Arpeggio

Arpeggiate three notes: base note, +X steps, and +Y steps from the base note. Arpeggio speed and mode is controlled by `ARC` command.

Example: `ARP 37` will play a minor chord arpeggio.

### `ARC XY` — Arpeggio configuration

Arpeggio configuration. X is the arpeggio mode (up, down, up/down, octave range), Y is the speed in ticks.

### `PVB XY` — Vibrato

Pitch vibrato with the speed of X and Y depth. In linear pitch mode the depth is set in 10 cent steps. Vibrato period in ticks is calculated with the formula: `32 - Speed * 2`. So, a vibrato with the speed C will have period `32 - 12 * 2 = 8 ticks`.

### `PBN XX` — Pitch bend

Bend pitch by XX every phrase/table row. XX is a signed hex number, so FF = -1, etc.

### `PSL XX` — Pitch slide (portamento)

Slide from the previous pitch to the new pitch for XX ticks.

### `PIT XX` — Pitch offset (relative)

Offset tone pitch by XX pitch table steps. The offset is accumulated.

### `FIN XX` — Fine pitch offset (relative)

Offset tone pitch by XX. In linear pitch mode the offset is in cents. In regular mode the value is tone period offset (equivalent to `PRD` FX). The offset is accumulated.

### `PRD XX` — Tone period offset (relative)

Offset tone period by XX. The offset is accumulated.

### `VOL XX` — Volume offset (relative)

Offset volume by XX. The offset is accumulated.

### `VSL XX` — Volume slide

Change volume by XX every phrate/table row. XX is a signed hex number.

### `RET XY` — Retrigger

Retrigger note every Y tics. X controls volume change. Y=0 stops the effect.

### `DEL XX` — Delay note

Delay note start by XX tics. If XX is greater that the groove value, the note will be skipped.

### `OFF XX` — Note off

Turn note off after XX tics. Equivalent to putting `OFF` in the note column and delaying it with `DEL` FX. Triggers the release stage of ADSR envelopes.

### `KIL XX` — Kill note

Turns off note after XX tics. ADSR release stage is not be triggered, the note is fully silenced.

### `TIC XX` — Set table speed

Set table speed to XX ticks per row. Changes speed for all table FX columns.

### `TBL XX` — Set instrument table

Replace the instrument table with a different table. Value FF stops table.

### `TBX XX` — Set aux table

Set aux table. Aux table plays together with the instrument table. Value FF stops aux table.

### `THO XX` — Hop all table columns

Hop to row XX in the instrument table. When used in a table, all FX columns hop to this row.

### `TXH XX` — Hop all aux table columns

Hop to row XX in the aux table. Doesn't work in tables.

### `HOP XY` — Conditional jump

Conditional jumps in phrases and tables. In tables this FX hops only in the column where FX is placed (unlike THO which hops all columns).

Hops X times to row Y. If X = 0, it creates an infinite loop. You can create nested loops with HOP FX.

### `GRV XX` — Set track groove

Set groove XX for this track.

### `GGR XX` — Global groove

Set groove XX for all tracks.


## Modulation Effects

There are 4 commands for each of 4 modulation sources (n = 1-4) to modify modulation parameters.

### `MnA XX` — Modulation amount (relative)

Change the modulation amount by XX. XX is a hexadecimal signed value.

### `Mn1 XX` — Modulation parameter 1 (relative)

Change the first modulation parameter by XX:

- ADSR: Attack
- AHD: Attack
- LFO: Shape

### `Mn2 XX` — Modulation parameter 2 (relative)

Change the second modulation parameter by XX:

- ADSR: Decay
- AHD: Hold
- LFO: Trig

### `Mn3 XX` — Modulation parameter 3 (relative)

Change the third modulation parameter by XX:

- ADSR: Sustain
- AHD: Decay
- LFO: Period

### `Mn4 XX` — Modulation parameter 4 (relative)

Change the fourth modulation parameter by XX:

- ADSR: Release
- AHD: N/A
- LFO: N/A


## Effects common for all AY Instruments

### `AYM XY` — AY mixer

Update AY mixer settings for the instrument:

- X - envelope shape,
- Y - tone/noise control: 0 - off, 1 - tone, 2 - noise, 3 - tone+noise

### `NOI XX` — Noise period (relative)

Offsets noise period by XX. The value is accumulated.

### `NOA XX` — Noise period (absolute)

Sets absolute noise period value to XX. When XX = FF, disables sending noise period from this track to make noise period values from previous tracks take priority.

### `ERT --` — Retrig envelope

Retrigger amplitude envelope (writes the current envelope shape value to register 13).

### `EAU XY` — Auto-envelope settings

Set auto envelope rate to X:Y. If X = 0, auto envelope is turned off.


## AY Classic Instrument Effects

### `EVB XY` — Envelope vibrato

Envelope vibrato with X speed and Y depth. Similar to `PVB` command but affects only envelope period.

### `EBN XX` — Envelope pitch bend

Bend envelope period by XX per phrase/table row. Similar to `PBN` command but affects only envelope period.

### `ESL XX` — Envelope pitch slide (portamento)

Slide from the previous to the new envelope period value for XX ticks. Similar to `PSL` command but affects only envelope period.

### `ENT XX` — Envelope pitch as a note

Set envelope period to a value matching a note. See the bottom line on the screen for the note name.

### `EPT XX` — Envelope period offset (relative)

Offset envelope period by XX. The value is accumulated.

### `EPH XX` — Envelope period high

Set the high byte of envelope period.

### `EPL XX` — Envelope period low

Set the low byte of envelope period.

## AY Plus Instrument Effects

### `TNN XX` — Set tone oscillator to a note

Set tone oscillator to a specific note, ignoring phrase note and pitch offsets in the instrument.

### `TNP XX` — Tone oscillator pitch offset (relative)

Offset tone oscillator pitch by XX pitch table steps. The value is accumulated.

### `TNF XX` — Tone oscillator fine offset (relative)

Offset tone oscillator fine pitch by XX (cents/period). The value is accumulated.

### `TRT --` — Retrigger tone oscillator

To be implemented.

### `ENN XX` — Set envelope oscillator to a note

Set envelope oscillator to a specific note, ignoring phrase note and pitch offsets in the instrument.

### `ENP XX` — Envelope oscillator pitch offset (relative)

Offset envelope oscillator pitch by XX pitch table steps. The value is accumulated.

### `ENF XX` — Envelope oscillator fine offset (relative)

Offset envelope oscillator fine pitch by XX (cents/period). The value is accumulated.

### `SFT XX` — Set software oscillator type

Change software oscillator type:

- 0 — None
- 1 — Pulse
- 2 — Sync Tone
- 3 — Sync Env
- 4 — Wavetable
- 5 — Tone FM
- 6 — Env FM

### `SFN XX` — Set software oscillator to a note

Set software oscillator to a specific note, ignoring phrase note and pitch offsets in the instrument.

### `SFP XX` — Software oscillator pitch offset

Offset software oscillator pitch by XX pitch table steps. The value is accumulated.

### `SFF XX` — Software oscillator fine offset

Offset software oscillator fine pitch by XX (cents/period). The value is accumulated.

### `SRT --` — Retrigger software oscillator

Reset software oscillator phase.

### `SFM XX` — Software oscillator FM offset (relative)

Offset software oscillator FM depth by XX. The value is accumulated.

### `PWM XX` — Software oscillator pulse width offset (relative)

Offset software oscillator pulse width by XX. The value is accumulated.

### `SPL XX` — Software Pulse oscillator low value offset (relative)

Offset software Pulse oscillator low value by XX. The value is accumulated.

### `SWT XX` — Software oscillator wavetable index offset (relative)

Offset software Wavetable oscillator wave index by XX. The value is accumualted.


## AY Sample Instrument Effects

AY Sample shares most effects with the AY Plus instrument.

### `SNS XX` — Sample start

Sets the sample start position to `XX * 64`.