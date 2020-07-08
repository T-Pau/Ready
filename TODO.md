#  TODO

## Support for Multiple Computers

- move Audio.m, ringbuffer.c to Emulator
- split machine specific ui callbacks into their own files
- render: More efficient partial updates (especially refresh only)
- rewrite machine configuration, embed in scroll view
- hierarchical configuration
- don't split media in games by type, use single list

## VIC-20

- Lightpen support
- VIC-1001 with jpkernal: keyboard strange in lower case mode

## other

- multiple tape images
- warn if media can't be used due to machine configuration
- controller auto-mapping: input device priority per input type
- fix dragging files out of Ready
- sharing
  - add share button
  - allow selecting media, images in all game views
  - share whole game
- documentation
- disk select: sub views for game / tools (inbox?)
- disk select: show currently selected (other drives too)
- turn off disk drives (via status bar)
- tape status view left border too wide (led view too wide?)
- sharing of whole game
- create blank disk images
- can't add disks to ongoing drag session
- pause emulation when losing focus
- tape controls (tapping on tape status)
- interlacing not working (Crest - Deus Ex Machina)
- improve game info view
    - select button
    - choose file to autostart from disk
    - save states
- MIDI support
- allow removing border from screenshots
- use Metal to draw image, support shaders for CRT effects
- software keyboard
    - keyboard sounds
    - visual feedback for key presses
- map input device buttons to key / joystick presses
- adapt view layouts for compact width
- support x64, p64 (UTI, document type, DiskImage)

## Vice enhancements

- multiple mice/paddles
- Commodore 1350
- Superpad64 (snespad for user port)
- all 12 buttons on SNES controller
- 3.5" drive sounds
    
## less important

- option to disable disk autoload by injection
- check why decoding some g64 images fails
- choose title image (from screenshots, Photos, files)
- GameInfo: copy/paste media, title image, screenshots
- fix init/shutdown of sfx_soundexpander and reenable
- t64 with only one file: inject (maybe even with multiple files?)

## warts

- add status bar hiding to navigation bar animation
- drag images: set file name
- GameView: allow scrolling further up if keyboard is shown
- Input Mapping Popup too high

## OS warnings

- fix Vice memory warnings

## compact width

- dismiss sorting popup (and other popups)
- single column Game View (info below title image, disks below cartridge/tape; or as sub views?)
- bios settings look awful

## maybe


## cleanup

- protect controller mapping access via mutex
- move keypress forwarding to EmulatorViewController

## temporary debug changes

- Strip Linked Product was Yes, changed to No
- Strip Swift Symbols was Yes, changed to No
