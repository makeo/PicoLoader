# PicoLoader
This is an ODE based modchip for the Nintendo GameCube to boot homebrew.\
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
- No soldering directly to the GameCube or to tiny pads
- No permanent modifications to the GameCube
- Running Pico at 200Mhz for better compatibility with clone boards
- No overdriving of signals
- Easy to remove
- Alternative [low-profile install option](https://github.com/makeo/PicoLoader/wiki/4.-Advanced-Section#low-profile-install-option)

## Installation & Documentation
Get started by looking at the [wiki](https://github.com/makeo/PicoLoader/wiki/)!

## Special Thanks
- [silversteel](https://github.com/silverstee1) for his help, especially for making the flex PCB and mount
- [TeamOffBroadway](https://github.com/OffBroadway) for the amazing idea to use a flex PCB to intercept the drive signals
