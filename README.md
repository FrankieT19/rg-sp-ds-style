# DS Style for RG SP

DS Style is a Nintendo DS-inspired launcher originally made for the GBA. This port brings the feel of original hardware to the Anbernic RG SP.

DS Style uses your installed stock emulators, core choices, settings and saves.

*For the RG SP running stock OS.*

## Download and install

1. Download **DS-Style-RG-SP-v1.0.zip** from [Releases](https://github.com/FrankieT19/rg-sp-ds-style/releases/latest).
2. Extract the ZIP. Copy its **Roms** folder to your RG SP SD card's root, merging with your existing Roms folder.
3. Optionally set your username by editing **Roms → APPS → DS Style → username.txt**
4. Safely eject the card, put it back in the RG SP and boot stock OS.
5. Set the stock OS's theme to Style 1 or Style 2, not MU Style **Settings → Icon Settings → Style 1 / Style 2 / Return to old style**
6. Open **Apps → DS Style**.
7. Optionally enable **Settings → Startup → Autoboot** to use as your launcher.

Install on either SD1 or SD2. Games and apps on both cards are available. Autoboot needs SD1 present even when DS Style is installed on SD2.

## Features

- A DS style theme at the GBA's resolution for a retro feel.
- List, List + Art, horizontal and vertical carousel views.
- Colour themes, dark mode, artwork preferences.
- Optional LCD grid and Pixel Transparency filters for an authentic feel.
- Folder search; press **X** while browsing.
- Shared stock favourites and recent history.
- Stock RetroArch and Game Rooms launch routes; change a system's route with **START** on the Systems list. Core selections are read from stock when launching.
- Seven language choices: English, French, German, Spanish, Portuguese, Italian and Dutch.
- A secret Snake game tucked away in About.

## Customise

Inside `Roms/APPS/DSStyle`:

- **Folder Art/**: For alternative system folder artwork add `GBA.png`, `Game Boy Advance.jpg`, `PS.png`, etc. Matching is case-insensitive; PNG, JPEG and BMP are supported.
- **assets/**: editable backgrounds, themes, sounds and icons. Keep the original image dimensions; list icons use a 16 × 14 canvas with transparent spacing. WAV files must be 48 kHz, stereo, 16-bit PCM.
- **username.txt**: change the name shown in the title bar.

## Update or uninstall

**Update:** back up your existing DS Style folder. Merge the new package, keeping `state`, your configuration, Folder Art, username and any assets you edited. If an update changes an asset you customised, compare it with your backup rather than losing your edits.

**Uninstall:** first switch **Startup → Autoboot → Off**. Then use **System → Stock OS**, shut down safely and remove `Roms/APPS/DS Style.sh` and `Roms/APPS/DSStyle` from the card. Turning Autoboot off restores the previous boot override. Your ROMs, emulators and saves stay in their stock locations.

If an autoboot installation cannot open, create an empty file named **DISABLED** (no extension) inside `Roms/APPS/DSStyle` on the card. The next boot skips DS Style. Keep the installation present, launch it manually from stock Apps and disable Autoboot before removing it. [More on stock integration and recovery](docs/STOCK-INTEGRATION.md).

## Source, credits and contributing

This repository includes the source and editable assets. See [building and testing](docs/BUILDING.md) and [contributing](CONTRIBUTING.md).

DS Style is by **FrankieT19**. Its GBA roots are the [Omega](https://github.com/FrankieT19/omega-ds-style-kernel) and [Omega Definitive Edition](https://github.com/FrankieT19/omega-de-ds-style-kernel) projects. Filter acknowledgements include Gigaherz and jdgleaver (lcd1x inspiration), and Matt Akins (Pixel Transparency). Full artwork, library and runtime credits are in [CREDITS.md](CREDITS.md) and the bundled notices.

Project code is [Apache-2.0](LICENSE); third-party components and artwork retain their own licences.
