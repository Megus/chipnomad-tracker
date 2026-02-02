---
title: Tracker Effects
description: ChipNomad Manual - Tracker Effects
layout: single
---

# Tracker Effects

{{< toc >}}

## General Effects

### `ARP XY` – Arpeggio

Arpeggiate three notes: base note, +X semitones, and +Y semitones from the base note. Arpeggio speed and mode is controlled by `ARC` command.

Example: `ARP 37` will play a minor chord arpeggio.

### `ARC XY` – Arpeggio configuration

Arpeggio configuration. X is the arpeggio mode (up, down, up/down, octave range), Y is the speed in ticks.

### `PVB XY` – Vibrato

Pitch vibrato with the speed of X and Y depth. In linear pitch mode the depth is set in 10 cent steps.

### `PBN XX` – Pitch bend

Bend pitch by XX every phrase/table row. XX is a signed hex number, so FF = -1, etc.

### `PSL XX` – Pitch slide (portamento)

Slide from the previous pitch to the new pitch for XX ticks.

### `PIT XX` – Pitch offset

Offset tone pitch by XX semitones. The offset is accumulated.

### `FIN XX` - Fine pitch offset

Offset tone pitch by XX. In linear pitch mode the offset is in cents. In regular mode the value is tone period offset (equivalent to PRD FX). The offset is accumulated.

### `PRD XX` - Tone period offset

Offset tone period by XX. The offset is accumulated.

### `VOL XX` – Volume offset

Offset volume by XX. The offset is accumulated.

### `RET XY` – Retrigger

Retrigger note every Y tics. X controls volume change.

### `DEL XX` – Delay note

Delay note start by XX tics. If XX is bigger that the groove value, the note will be skipped.

### `OFF XX` – Note off

Turn note off after XX tics. Equivalent to putting OFF in the note column and delaying it with DEL FX. Triggers release stage of ADSR envelopes.

### `KIL XX` – Kill note

Turns off note after XX tics. ADSR release stage is not be triggered, the note is fully silenced.

### `TIC XX` – Set table speed

Set table speed to XX ticks per row.

### `TBL XX` – Set instrument table

Replace the instrument table with a different table.

### `TBX XX` – Set aux table

Set aux table. Aux table plays together with the instrument table.

### `THO XX` – Hop all table columns

Hop to row XX in the instrument table. When used in a table, all FX columns hop to this row.

### `TXH XX` — Hop all aux table columns

Hop to row XX in the aux table.

### `HOP XY`

Conditional jumps in phrases and tables. In tables this FX hops only in the column where FX is placed (unlike THO which hops all columns).

Hops X times to row Y. If X = 0, it creates an infinite loop. You can create nested loops with HOP FX.

### `GRV XX` – Set track groove

Set groove XX for this track.

### `GGR XX` – Global groove

Set groove XX for all tracks.

## AY-Specific Effects

### `AYM XY` – AY mixer

Update AY mixer settings for the instrument: X - envelope shape, Y - tone/noise control (0 - off, 1 - tone, 2 - noise, 3 - tone+noise)

### `ERT --` – Retrig envelope

Retrigger amplitude envelope (writes the current envelope shape value to register 13).

### `NOI XX` – Noise period (relative)

Offsets noise period by XX. The value is accumulated.

### `NOA XX` – Noise period (absolute)

Sets absolute noise period value to XX. When XX = FF, disables sending noise period from this track to make noise period values from previous tracks take priority.

### `EAU XY` – Auto-envelope settings

Set auto envelope rate to X:Y. If X = 0, auto envelope is turned off.

### `EVB XY` – Envelope vibrato

Envelope vibrato with X speed and Y depth. Similar to PVB command but affects only envelope period.

### `EBN XX` – Envelope pitch bend

Bend envelope period by XX per phrase/table row. Similar to PBN command but affects only envelope period.

### `ESL XX` – Envelope pitch slide (portamento)

Slide from the previous to the new envelope period value for XX ticks. Similar to PSL command but affects only envelope period.

### `ENT XX` – Envelope pitch as a note

Set envelope period to a value matching a note. See the bottom line on the screen for the note name.

### `EPT XX` – Envelope period offset

Offset envelope period by XX. The value is accumulated.

### `EPH XX` — Envelope period high

Set the high byte of envelope period.

### `EPL XX` — Envelope period low

Set the low byte of envelope period.
