---
title: Settings Screen
description: ChipNomad Manual - Settings Screen
layout: single
---

# Settings Screen

![](settings.png)

Setting screen has a few useful settings:

- **Pitch conflict warning**. Because the tone phase is unpredictable on AY, playing sounds with the same pitch on different tracks can create unpleasant phasing effects. When this option is enabled, such conflicts will be highlighted during playback to help with detecting and fixing them.
- **Mix volume**. This is master mix volume and you may need to lower it if you use multiple chips. 60% works well for 1 AY chip and 40% is good for 2 AY chips. When audio overload happens, it will be highlighted in the playback preview area.
- **Quality**. Changes the resampling filter quality for Ayumi AY emulation engine. Most platforms should be fine with the BEST, but pre-2024 RG35xx may need a lower setting.
- **Key mapping**. Opens [key mapping screen](#key-mapping).
- **Load Font**. ChipNomad comes with a few bitmap fonts and it's possible to create your own.
- **Edit color theme**. Opens [color theme screen](#color-theme).

## Color Theme

![](color-theme.png)

Edit, load, and save color themes. Make ChipNomad truly yours!

## Key Mapping

![](key-mapping.png)

Redefine standard keys and map up to 3 physical keys to each logical key. On desktop platforms you can also map gamepad buttons. In case you messed up the key mapping, you can press any unmapped button 5 times to reset to default key mapping. Or you can delete `settings.txt` file after closing the tracker — ChipNomad will re-create it with the default key mapping on the next launch.