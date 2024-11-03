## DoomBIOS
* Play your Original Xbox the way Bill Gates truly intended with DoomBIOS.
* This is a 256kB bios image that replaces the OG Xbox BIOS to do one thing. Play DOOM.
* 100% opensource with no copyright code.

https://github.com/user-attachments/assets/ad5efb7f-79c7-41e3-9849-dc8d458fa6fb

## Todo
* Fix conexant video encoder issues
* Fix fatx writes? Seems to work on xemu but not hw
* Test Xcalibur, Focus alot more
* Support locked Xbox hard drives

## Quickstart:
* You need to have your xbox harddrive in an unlocked state (FIXME!)
* Download the [Shareware version](https://github.com/Daivuk/PureDOOM/blob/master/doom1.wad).
* Place at `E:/doom/doom1.wad`.
* Flash bios to your modchip of choice and run.

## Compile
```
sudo apt install git build-essential clang nasm cmake python3 python3-pip
pip install objutils lz4 pyelftools
mkdir build && cd build
cmake ..
cmake --build .
```

This creates a 256kB binary that you use as an Original Xbox BIOS image.

## General features
Although we run doom, there's alot of boilerplate to make this work. The following are features I think are neat

* Uses FreeRTOS kernel for multithreading support
* Custom ATA/ATAPI driver with full DMA support up to UDMA6 (auto negotiated speeds)
* Seemlessly read/write files on FAT16,FAT32,exFAT and FATX filesystems using standard c library calls (fopen etc)
* ISO9660 support for reading DVD roms
* USB Mass Storage support
* PCM audio driver from nxdk
* Compresses the bios at compile time, and decompresses to RAM at run time using LZ4 for significant ROM savings.
* Uses the visor exploit which runs on any xbox version
* Comprehensive C library included using picolibc.

## License:
* Some code is based on [Cromwell](https://github.com/XboxDev/cromwell) which is licensed under GPL2.
* Code written by me is licensed under either MIT of CC0-1.0. See respective source files.
* I use a bunch of external libraries. See below for licensing.

## Third Party Libraries:
* PureDOOM https://github.com/Daivuk/PureDOOM
* Arith64 https://github.com/glitchub/arith64
* FATFS http://elm-chan.org/fsw/ff/
* FreeRTOS https://github.com/freertos
* lib9660 https://github.com/erincandescent/lib9660
* FATX https://github.com/mborgerson/fatx
* LZ4 https://github.com/lz4/lz4
* Midiplay https://github.com/fawtytoo/Midiplay/tree/main
* Picolibc https://github.com/picolibc/picolibc
* TinyUSB https://github.com/hathach/tinyusb
* Xinput https://github.com/Ryzee119/tusb_xinput

## References:
* https://github.com/XboxDev/cromwell
* https://github.com/mborgerson/xqemu-kernel
* https://github.com/Ernegien/xdecode
* https://wiki.osdev.org/
