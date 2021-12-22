# README : Wii64 / Cube64

> LICENSE:
>     This software is licensed under the GNU General Public License v2
>       which is available at: http://www.gnu.org/licenses/gpl-2.0.txt
>     This requires any released modifications to be licensed similarly,
>       and to have the source available.
 

**Wii64/Cube64 and their respective logos are trademarks of Team Wii64 and should not be used in unofficial builds.**

## QUICK USAGE:
 * ROMs can be z64 (big-endian) or v64 (little endian), or .n64, of any size
 * To install: Extract the contents of the latest release zip to the root of your SD card
 * For SD/USB: Put ROMs in the directory named /wii64/roms,
    All save types will automatically be placed in /wii64/saves
 * For DVD: ROMs may be anywhere on the disc (requires DVDxV2 on Wii)
 * Load the desired executable from the HBC or in the loader of your choice, the emulator is shipped with 2 graphics plugins now.
	 * **Rice GFX Plugin version** 
		 * Slightly faster and has fixed sky boxes in games but isn't as refined as glN64
		 * Supports hi-res texture pak via Wii64 specific [texture packer](https://github.com/emukidid/Wii64/releases/tag/texturepacker_1.2)
	 * **glN64 GFX Plugin version**
		 * Slightly slower than Rice GFX on certain games
		 * FrameBuffer texture support (OoT subscreen)
		 * Optional 2xSaI texture filtering
		 * Less buggy, more progressed port (e.g. emulates fog)
 * Once loaded, select 'New ROM' and select the ROM to load and it will automatically start
 * The game can be exited any time by pressing X and Y together on a GC pad or Classic Controller,
   1 and 2 together on a Wiimote (only with Nunchuck attached), or the reset button
     (Note: this must be done to save your game; it will not be done automatically)

### Controls:
* Controls are fully configurable so any button on your controller can be mapped
* The controller configuration screen presents each N64 button and allows you to toggle through sources
* There are 4 configuration slots for each type of controller
	* To load a different, previously saved configuration, select the slot, and click 'Load'
	* After configuring the controls as desired, select the slot, and click 'Save'
	* After saving different configurations to the slots, be sure to save your configs in the input tab of the settings frame
* Clicking 'Next Pad' will cycle through the N64 controllers assigned
* There is an option to invert the Y axis of the N64's analog stick; by default this is 'Normal Y'
* The 'Menu Combo' configuration allows you to select a button combination to return to the menu

### Settings:
* General
	* Native Saves Device: Choose where to load and save native game saves
	* Save States Device: Choose where to load and save save states
	* Select CPU Core: Choose whether to play games with pure interpreter (better compatibility) or dynarec (better speed)
	* Save settings.cfg: Save all of these settings either SD or USB (to be loaded automatically next time)
* Video
	* Show FPS: Display the framerate in the top-left corner of the screen
	* Screen Mode: Select the aspect ratio of the display; 'Force 16:9' will pillar-box the in-game display
	* CPU Framebuffer: Enable for games which only draw directly to the framebuffer (this will only need to be set for some homebrew demos)
	* 2xSaI Tex: Scale and Interpolate in-game textures (unstable on GC, not supported in Rice GFX)
	* FB Textures: Enable framebuffer textures (necessary for some games to render everything correctly (e.g. Zelda Subscreen), but can impact performance; unstable on GC, not supported in Rice GFX)
* Input
	* Configure Input: Select controllers to use in game
	* Configure Paks: Select which controller paks to use in which controllers
	* Configure Buttons: Enter the controller configuration screen described above
	* Save Button Configs: Save all of the controller configuration slots to SD or USB
	* Auto Load Slot: Select which slot to automatically be loaded for each type of controller
* Audio
	* Disable Audio: Select to mute the sound
* Saves
	* Auto Save Native Saves: When enabled, the emulator will automatically load
     saves from the selected device on ROM load and save when returning to the menu or
     turning off the console
## ADVANCED USAGE
### GameCube Version(s)
Wii64 also exists as Cube64, a version of the emulator with the same UI and features albeit with tighter memory restrictions and less CPU power. This version has significantly less memory available and requires heavy paging of ROM data from your storage medium into a small ARAM cache and then into main memory, this is denoted by every time you see a cartridge icon in the top right hand corner of the screen.

There's also 3 versions per graphics plugin shipped (6 .dol files total). 

The "-exp.dol" versions attempt to support the expansion pak by removing the mini menu, boxart and further reducing various caches, this should only really be used for expansion pak required games as it will likely crash more often due to less memory being available.

The "-basic.dol" versions remove boxart support and the mini menu in an attempt to have more free memory available to avoid crashes.

### "WiiVC" Version
This version isn't really supported much due to the niche nature of it. It enables DRC (Wii U GamePad) support, and also takes advantage of unlocked CPU multiplier support if enabled. The gist of it is that you can run Wii64 on a Wii U in "Wii mode" but with Wii U Game Pad support and (optionally) the CPU multiplier unlocked. To boot this version you can either just use the vWii mode to use the GamePad, or to unlock the CPU multiplier you'll need to be well versed in Wii U homebrew setups (essentially there's a process that exists to inject homebrew into a WiiVC title - e.g. Wii titles were available on the Wii U via the eShop) and there's a thing called [sign_c2w_patcher](https://github.com/FIX94/sign_c2w_patcher) that you must boot before loading this title to unlock the CPU multiplier. This version is shipped as "Wii64 | Rice GFX | WiiVC" and "Wii64 | glN64 GFX | WiiVC" in the archive.

### Boot time arguments
The following can be passed in via wiiload or by editing the meta.xml to override settings. They can also be changed via the settings.cfg that's created upon booting up the emulator for the first time.
* **MiniMenu** - Which menu style should the emulator default to.
	 * 0 = Don't boot to Mini Menu
	 * 1 = Boot to mini menu (default)
* **Audio** - Audio toggle
	 * 0 = Disabled
	 * 1 = Enabled (default)
 * **FPS** - FPS display toggle
	 * 0 = Disabled (default)
	 * 1 = Enabled
 * **FBTex** - FrameBuffer textures for glN64 (e.g. OoT pause screen)
	 * 0 = Disabled (default)
	 * 1 = Enabled
 * **2xSaI** - 2xSaI texture upscaling for glN64
	 * 0 = Disabled (default)
	 * 1 = Enabled
 * **ScreenMode**
	 * 0 = 4:3
	 * 1 = 16:9
	 * 2 = 16:9 Pillar box
 * **VideoMode**
	 * 0 = VIDEOMODE_AUTO (default)
	 * 1 = VIDEOMODE_PAL60
	 * 2 = VIDEOMODE_240P
	 * 3 = VIDEOMODE_480P
	 * 4 = VIDEOMODE_PAL
	 * 5 = VIDEOMODE_288P
	 * 6 = VIDEOMODE_576P
 * **Core**
	 * 0 = Pure Interpreter
	 * 1 = Dynamic Recompiler (default)
 * **CountPerOp**
	 * 0 = 1 per Op (default for WiiVC)
	 * 1 = 2 per Op (default for Wii)
	 * 2 = 3 per Op (default for GameCube)
 * **NativeDevice** - Which device to use for Native (SRAM/FlashRAM/EEPROM) saves
	 * 0 = SD
	 * 1 = USB
	 * 2 = Memory Card A
	 * 3 = Memory Card B
 *  **StatesDevice** - Which device to use for Save States
	 * 0 = SD
	 * 1 = USB
 * **AutoSave** - Whether or not to automatically save native saves when returning to the menu
	 * 0 = Disabled
	 * 1 = Enabled (default)
 * **LimitVIs** - How to cap emulation speed
	 * 0 = No VI limit
	 * 1 = Wait for VI (default)
	 * 2 = Wait for Frame
 * **Pak1 / Pak2 / Pak3 / Pak4** - What's inserted in each Controller Pak slot
	 * 0 = Memory Pak (default)
	 * 1 = Rumble Pak
 * **LoadButtonSlot** - Which slot to load button mappings from
	 * 0 to 3 = Button slots 0 to 3
	 * 4 = Default

## COMPATIBILITY
Report and view any open issues on the [issue tracker](https://github.com/emukidid/Wii64/issues).

## CREDITS
 * Core Coder: tehpola
 * Graphics & Menu Coder: sepp256
 * General Coder & current maintainer: emu_kidid
 * Original mupen64: Hactarux
 * [Not64](https://github.com/extremscorner/not64): [Extrems](extremscorner.org)
 * WiiVC/DRG stuff: [FIX94](https://github.com/FIX94/)
 * Artwork: drmr
 * Wii64 Demo ROM: marshallh
 * Compiled using [devKitPro](https://devkitpro.org/) and "libOGC2" ([unofficial](https://github.com/emukidid/libogc))
 * Visit the official code repo on [GitHub](https://github.com/emukidid/Wii64)