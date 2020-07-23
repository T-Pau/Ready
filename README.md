## A Home Computer Emulator for iPad.
## Almost Like the Real Thing.

![Screenshot](screenshot.png)

Ready is an open source emulator for the following 8 bit home computers:

- [Commodore 64](https://en.wikipedia.org/wiki/Commodore_64)
- [Commodore VIC-20](https://en.wikipedia.org/wiki/Commodore_VIC-20)
- [Commodore 16, Plus/4](https://en.wikipedia.org/wiki/Commodore_Plus/4)
- [Sinclair ZX Spectrum](https://en.wikipedia.org/wiki/ZX_Spectrum) (16k, 48k, Spectrum+, 128k, +2)

With preliminary support for these computers:

- [Atari 600XL, 800XL](https://en.wikipedia.org/wiki/Atari_8-bit_family)
- [Commodore 128](https://en.wikipedia.org/wiki/Commodore_128)
- [Commander X16](https://www.commanderx16.com/)

It is based on [atari800](https://atari800.github.io), [fuse](http://fuse-emulator.sourceforge.net/), [Vice](http://vice-emu.sourceforge.net), and [x16-emulator](https://github.com/commanderx16/x16-emulator), which provide accurate emulation of many hardware variants and peripherals.

It requires at least iPadOS 13.5.

It aims to approximate the feeling of using actual hardware: 
- Rather than configuring abstract settings, you select hardware components.
- The software keyboard is a faccimile of the original, reflecting the different existing styles.
- Even the noise of the disk drive is emulated.

The **Library** section offers a way to organize a large collection of games, demos and programs. The emulated hardware can be customized for each entry.

The **Inbox** provides a place to quickly try new programs. You can easily move the ones you want to keep to the library.

**Tools** mode allows attaching a cartridge like Action Replay or Final Cartridge and a collection of disks with your favorite tools, to help you dig into the programs, like back in the day.

### Documentation

The beginnings of a [User Manual](https://github.com/Spider-Lab/C64/wiki/User%20Manual) can be found in the wiki. Any help on expanding it would be greatly appreciated.

### Contact

Visit the website [spiderlab.at/c64](http://spiderlab.at/c64/) for screenshots.

You can join us on [Discord](https://discord.gg/4GuSY5e).

The authors can be contacted at ready@tpau.group.

### Installing

Since emulators where users can add their own programs are not allowed in the App Store, you will have to install it yourself. You will need [Xcode](https://developer.apple.com/xcode/) and a [developer certificate](https://developer.apple.com/account/).

You can build it from source yourself. Ready has no external dependencies. 

Or you can use [iOS App Signer](https://dantheman827.github.io/ios-app-signer/) to install a pre-built binary release (`.ipa` file).
