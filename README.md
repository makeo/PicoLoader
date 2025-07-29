# PicoLoader
This is a modchip for the Nintendo GameCube to boot homebrew.\
Its price and functionality are very similar to PicoBoot but has multiple advantages.

Join the [Discord server](https://discord.gg/YtA9aU3BKZ) to get support and discuss the mod!

This mod works by emulating a disk drive during boot, executing a homebrew app, and afterwards re-enabling the disk drive.

## Features
- Open source
- Cheap and easy to get components (Raspberry Pi Pico + Flex PCB + Diode)
- Simple to boot any homebrew app using an [online converter](https://makeo.github.io/PicoLoader/converter/)
- No soldering directly to the GameCube or to tiny pads
- Running Pico at 200Mhz for better compatibility with clone boards
- No overdriving of signals
- Easy to remove

## Installation
Get started by looking at the [installation instructions]([https://github.com/makeo/PicoLoader/wiki](https://github.com/makeo/PicoLoader/wiki/Hardware-Installation))!

## Special Thanks
- [silversteel](https://github.com/silverstee1) for his help, especially for making the flex PCB and mount
- [TeamOffBroadway](https://github.com/OffBroadway) for the amazing idea to use a flex PCB to intercept the drive signals
