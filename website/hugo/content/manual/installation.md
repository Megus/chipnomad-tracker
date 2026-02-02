---
title: Installation
description: ChipNomad Manual - Installation
layout: single
---

# ChipNomad Installation

[Download ChipNomad](https://github.com/Megus/chipnomad-tracker/releases) build for your platform.

## Desktop (Windows, macOS, Linux)

ChipNomad doesn't require special installation, just unzip the distribution package to a convenient folder and run the exectuable file.

## PortMaster

This build works on a wide variety of Linux-based handheld consoles. Ths list of [supported devices](https://portmaster.games/supported-devices.html).

As ChipNomad is not currently published to [PortMaster](https://portmaster.games) repository, you will need to manually install it. PortMaster website gives [the following instructions](https://portmaster.games/faq.html#do-i-have-to-use-portmaster-to-install-ports):

> ...Copy the zip file into the PortMaster Autoinstall folder. Then you just run the PortMaster Application and PortMaster will install the Port for you.
>
> Here are the locations for the autoinstall folder for the
>
> - AmberELEC, ROCKNIX, uOS, Jelos: `/roms/ports/PortMaster/autoinstall/`
> - muOS: `/mmc/MUOS/PortMaster/autoinstall/`
> - ArkOS: `/roms/tools/PortMaster/autoinstall/`
> - Knulli: `/userdata/system/.local/share/PortMaster/autoinstall`
>
> If that does not work you can also unzip the contents of the port into the ports folders of each cfw, note that this may break the port and ports may no longer start.
>
> - AmberELEC, ROCKNIX, uOS, Jelos: `/roms/ports/`
> - muOS: `/mmc/ports/` for the folders and `/mnt/mmc/ROMS/Ports/` for the .sh files
> - ArkOS: `/roms/tools/PortMaster/autoinstall/`
> - Knulli: `/userdata/system/.local/share/PortMaster/autoinstall`

## Pre-2024 Anbernic RG35xx build

This build will only work on Pre-2024 RG35xx with GarlicOS 1.4 installed. Create a `ChipNomad` folder in `Roms/APPS` and unzip the distribution package to this folder.
