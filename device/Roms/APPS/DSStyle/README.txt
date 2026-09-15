DS Style for RG SP - v1.0
========================

Thanks for downloading DS Style for RG SP!

DS Style is a Nintendo DS-inspired launcher originally made for the GBA. This
port brings the feel of original hardware to the Anbernic RG SP.

DS Style uses your installed stock emulators, core choices, settings and
saves.

For the RG SP running stock OS.

DOWNLOAD AND INSTALL

1. Download DS-Style-RG-SP-v1.0.zip from Releases
   (https://github.com/FrankieT19/rg-sp-ds-style/releases/latest).
2. Extract the ZIP. Copy its Roms folder to your RG SP SD card's root, merging
   with your existing Roms folder.
3. Optionally set your username by editing Roms > APPS > DSStyle >
   username.txt
4. Safely eject the card, put it back in the RG SP and boot stock OS.
5. For Autoboot to work set the stock OS's theme to Style 1 or Style 2 under
   Settings > Icon Settings > Style 1 / Style 2 / Return to old style
6. Open Apps > DS Style.
7. Optionally enable Settings > Startup > Autoboot to use as your default
   launcher.

Install on either SD1 or SD2. Games and apps on both cards are available.
Autoboot needs SD1 present even when DS Style is installed on SD2.

For settings explanations select a setting in DS Style and press X.

DS Style uses the same game artwork as stock OS. Install it as usual in each
system's Imgs folder (for example, Roms/GBA/Imgs), using filenames that match
your games. To automate artwork downloads, you can use Skraper, which uses the
ScreenScraper database - configure it to save images in each system's Imgs
folder with filenames matching your games.

FEATURES

- A Nintendo DS style theme at the GBA's resolution for a retro feel.
- List, List + Art, horizontal and vertical carousel views.
- Colour themes, dark mode, artwork preferences.
- Optional LCD grid and Pixel Transparency filters for an authentic feel.
- Folder search; press X while browsing.
- Shared stock favourites and recent history.
- Stock RetroArch and Game Rooms launch routes; change a system's route with
  START on the Systems list. Core selections are read from stock when
  launching.
- Seven language choices: English, French, German, Spanish, Portuguese,
  Italian and Dutch.
- A secret Snake game tucked away in About.

CUSTOMISE

Inside Roms/APPS/DSStyle:

- Folder Art/: For alternative system folder artwork add GBA.png, Game Boy
  Advance.jpg, PS.png, etc. Matching is case-insensitive; PNG, JPEG and BMP
  are supported.
- assets/: editable backgrounds, themes, sounds and icons. Keep the original
  image dimensions; list icons use a 16 × 14 canvas with transparent spacing.
  WAV files must be 48 kHz, stereo, 16-bit PCM.
- username.txt: change the name shown in the title bar.

UPDATE OR UNINSTALL

Update: Before updating, back up your DSStyle folder. Copy the new package
over your installation, then restore any artwork, sounds or username file you
customised from your backup - these files will be overwritten during the
update. Your saved settings in the state folder are retained automatically.

Uninstall: first switch Startup > Autoboot > Off. Then use System > Stock OS,
shut down safely and remove Roms/APPS/DS Style.sh and Roms/APPS/DSStyle from
the card. Turning Autoboot off restores the previous boot override. Your ROMs,
emulators and saves stay in their stock locations.

If an autoboot installation cannot open, create an empty file named DISABLED
(no extension) inside Roms/APPS/DSStyle on the card. The next boot skips DS
Style. Keep the installation present, launch it manually from stock Apps and
disable Autoboot before removing it. More on stock integration and recovery
(https://github.com/FrankieT19/rg-sp-ds-style/blob/main/docs/STOCK-INTEGRATION.md).

SOURCE, CREDITS AND CONTRIBUTING

This repository includes the source and editable assets. See building and
testing
(https://github.com/FrankieT19/rg-sp-ds-style/blob/main/docs/BUILDING.md) and
contributing
(https://github.com/FrankieT19/rg-sp-ds-style/blob/main/CONTRIBUTING.md).

Filter credits: Gigaherz and jdgleaver (lcd1x inspiration), and Matt Akins
(Pixel Transparency). See CREDITS.md
(https://github.com/FrankieT19/rg-sp-ds-style/blob/main/CREDITS.md).

Project code is Apache-2.0
(https://github.com/FrankieT19/rg-sp-ds-style/blob/main/LICENSE); third-party
components and artwork retain their own licences.

Repository: https://github.com/FrankieT19/rg-sp-ds-style

Enjoy DS Style!
