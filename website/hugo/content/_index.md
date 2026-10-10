---
title: ChipNomad
description: ChipNomad - a chiptune tracker for handheld consoles
---

{{< youtube hzf-BgJmqYU >}}

**ChipNomad** is a chiptune tracker for handheld consoles ([Anbernic](https://anbernic.com), [TrimUI](https://trimui.net), [Retroid](https://www.goretroid.com), etc) and desktop platforms.
It is heavily inspired by [LSDJ](https://www.littlesounddj.com) and [Dirtywave M8 tracker](https://dirtywave.com).
ChipNomad emulates real sound chips and allows making music that can be played on retro platforms such as ZX Spectrum, Amstrad CPC, Atari ST, etc. It also gives more freedom within reasonable limitations inspired by those platforms.

<p class="cn-download"><a href="https://github.com/Megus/chipnomad-tracker/releases">Download ChipNomad</a></p>

ChipNomad supports these chips:

- [AY-3-8910/YM2149F](/chips/ay-3-8910/) (ZX Spectrum, Amstrad CPC, Atari ST, MSX)

More chips planned:

- YM3812 / OPL2 (AdLib, Sound Blaster)
- YMF262 / OPL3 (Sound Blaster Pro/16)
- YM2151 / OPM (Sharp X1, X68000, Commander X16)
- YM2203 / OPN (TSFM card)
- YM2612 / OPN2 (Sega Genesis, FM Towns)
- MOS 6581/8580 SID (Commodore 64)

ChipNomad is available for multiple platforms:

- All devices supported by [PortMaster](https://portmaster.games)
- Pre-2024 Anbernic RG35xx with [GarlicOS 1](https://www.patreon.com/blackseraph/posts/garlicos-for-76561333)
- MiyooMini+ (MiyooPorts)
- Windows
- macOS
- Linux (x86 64-bit) — can be run on Steam Deck
- Android — handhelds like Retroid Pocket 5, smartphones, and tablets

## GenAI Disclaimer

Between *April 26, 2025* and *September 15, 2026*, I had used GenAI coding tools to assist with making ChipNomad.
I stopped and **I will _never_ use any GenAI tools for ChipNomad development again**. I'm taking a strict
**No GenAI** policy for this project. GenAI-assisted pull requests will not be accepted too.

The use was mostly limited to routine tasks such as bootstrapping UI for new screens, unit tests, and file parsers.
The core of ChipNomad was always written without any AI usage. This was an experiment to explore the possibilities
of GenAI coding tools, and I have to admit that it went for too long. I'm fully aware of the enviromental impact,
ethical issues, and the sentiment of the creative community towards GenAI. As the project evolves, all AI-assisted
code will be eventually removed or rewritten.
