# PicoLoader
This is an ODE-based modchip for the Nintendo GameCube to boot homebrew.\
Its price and functionality are very similar to [PicoBoot](https://github.com/webhdx/PicoBoot) but it has multiple advantages.

Join the [Discord server](https://discord.gg/YtA9aU3BKZ) to get support and discuss the mod!

## How does it work?
This mod works by emulating a disk drive during boot, executing a homebrew app, and afterwards re-enabling the disk drive.\
This is achieved using a flex PCB similar to FlippyDrive.
However, it is not a full optical drive emulator and only allows executing small homebrew apps (e.g. [swiss](https://github.com/emukidid/swiss-gc)) from flash just like PicoBoot.

## Features
- Open source
- Cheap and easy to get components (Raspberry Pi Pico + Flex PCB + Diode)
- You can keep the disk drive
- Simple to boot most homebrew apps using an [online converter](https://makeo.github.io/PicoLoader/converter/)
- No soldering directly to the GameCube
- Easy soldering
- No permanent modifications to the GameCube
- Running Pico at 200Mhz for better compatibility with clone boards
- Easy to remove
- Alternative [low-profile install option](https://github.com/makeo/PicoLoader/wiki/4.-Advanced-Section#low-profile-install-option)

## Planned Features
- Solder-free module
- Pico 2 support
- Panasonic Q Flex

## Installation & Documentation
> [!IMPORTANT]
> Please follow the [installation instructions](https://github.com/makeo/PicoLoader/wiki/2.-Hardware-Installation) in the wiki.\
> When not following the instructions, there is a chance of breaking the flex PCB during installation.

Get started by looking at the 📖[Wiki](https://github.com/makeo/PicoLoader/wiki/1.-Home)!

## Gallery
<div style="display: flex; gap: 10px; flex-wrap: nowrap; justify-content: center;">
  <img style="width: 23%; height: auto;" alt="1" src="https://github.com/user-attachments/assets/9dbe6a59-c3cd-4a4b-9462-4ebc6618a6cf" />
  <img style="width: 23%; height: auto;" alt="2" src="https://github.com/user-attachments/assets/87efdaf1-e2e6-4f9f-9bec-5a52d9c549fe" />
  <img style="width: 23%; height: auto;" alt="3" src="https://github.com/user-attachments/assets/02900e41-325a-48d2-bb01-8081845d7696" />
  <img style="width: 23%; height: auto;" alt="4" src="https://github.com/user-attachments/assets/9d39575d-f7d6-4cbe-b33c-1c6f40ef0f82" />
</div>

## Special Thanks
- [silversteel](https://github.com/silverstee1) for his help, especially for making the flex PCBs and mounts
- [TeamOffBroadway](https://github.com/OffBroadway) for the amazing idea to use a flex PCB to intercept the drive signals
- [Extrems](https://github.com/Extrems), [emukidid](https://github.com/emukidid) and everyone involved in creating swiss
- [novenary (9ary)](https://github.com/9ary) for gekkoboot

## Acknowledgements
The source of [gbi.hdr](https://github.com/makeo/PicoLoader/blob/main/picoloader/data/gbi.hdr) is licensed under GPL-2.0 and available [here](https://github.com/silverstee1/cubeboot-tools)
