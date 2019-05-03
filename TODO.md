#  TODO

## other

- warn if media can't be used due to machine configuration
- fix dragging files out of C64
- sharing
  - add share button
  - allow selecting media, images in all game views
  - share whole game
- organize machine parts into sections
- documentation
- disk select: sub views for game / tools (inbox?)
- disk select: show currently selected (other drives too)
- rewrite machine configuration view as collection view
- turn off disk drives (via status bar)
- no two mice (Vice limitation)
- tape status view left border too wide (led view too wide?)
- sharing of whole game
- create blank disk images
- t64 with only one file: inject (maybe even with multiple files?)
- can't add disks to ongoing drag session
- pause emulation when losing focus
- tape controls (tapping on tape status)
- interlacing not working (Crest - Deus Ex Machina)
- improve game info view
    - select button
    - choose file to autostart from disk
    - save states
- MIDI support
- Cheetah Annihilator Joystick (C64GS) (2nd button connecting POTX to VCC, nut supported by Vice)
- allow removing border from screenshots
- use Metal to draw image, support shaders for CRT effects
- software keyboard
    - visual feedback for key presses
    - shift lock
- support more input devices
    - paddle
        - sliding area
- map input device buttons to key / joystick presses
- adapt view layouts for compact width
- support x64, p64 (UTI, document type, DiskImage)
    
## less important

- option to disable disk autoload by injection
- check why decoding some g64 images fails
- choose title image (from screenshots, Photos, files)
- GameInfo: copy/paste media, title image, screenshots
- fix init/shutdown of sfx_soundexpander and reenable

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

- multiple tape images

## cleanup

- protect controller mapping access via mutex
- move keypress forwarding to EmulatorViewController

## temporary debug changes

- Strip Linked Product was Yes, changed to No
- Strip Swift Symbols was Yes, changed to No
