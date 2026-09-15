========================================
       DS Style for RG SP - v1.0
========================================

Thanks for downloading DS Style for RG SP!

Originally a Nintendo DS-inspired GBA frontend for EZ-FLASH Omega
and Omega Definitive Edition, DS Style now brings the
flashcart feel to the Anbernic RG SP.

This launcher sits over stock OS and uses its emulators, settings
and saves.

For the RG SP running stock OS.
No games, BIOS files, emulator cores or firmware are included.

INSTALL
1. Shut down your handheld and connect its ROM card to your PC.
2. Copy the extracted Roms folder to the card root, merging folders.
3. Safely eject, reinsert the card and boot the handheld.
4. Open Apps > DS Style in stock OS.

Install on either SD1 or SD2; one copy is enough for both cards.
To start automatically, use Settings > Startup > Autoboot.
Autoboot also requires SD1. Follow any stock menu-style prompt.

UPDATE
Back up your DSStyle folder, then merge the new package. Keep your
state folder, config, Folder Art, username.txt and edited assets.
The release does not include personal preferences or game history.

UNINSTALL
1. Turn Startup > Autoboot OFF first. This restores the prior boot
   override; do not delete an autoboot installation before doing so.
2. Choose System > Stock OS, then shut down safely.
3. On the card, delete Roms/APPS/DS Style.sh and Roms/APPS/DSStyle.
Your stock ROMs, emulators and saves remain in their own locations.

RECOVERY
Hold physical START during boot to bypass automatic game launching.
To skip DS Style altogether, create an empty file called DISABLED
(no extension) inside Roms/APPS/DSStyle. Then boot stock, open
DS Style manually and turn Autoboot off before uninstalling.
If DS Style cannot open manually, see the repository recovery guide.

CUSTOMISE
Folder Art: system pictures such as GBA.png or Game Boy Advance.jpg.
assets: editable graphics, icons, themes and sounds.
username.txt: your title-bar name.
state: created on first use; keep this when updating.
bin, scripts and config: required components; keep these present.

CREDITS
DS Style by FrankieT19. GBA roots: EZ-FLASH and Sterophonick's
SimpleLight. LCD grid inspiration: Gigaherz and jdgleaver.
Pixel Transparency: Matt Akins. Third-party artwork, library and
runtime credits and licences are included in DSStyle/licenses.
Project code: Apache-2.0; third-party materials retain their licences.
An independent project, not endorsed by the hardware makers.

Source, full instructions, controls, credits and issue reports:
https://github.com/FrankieT19/rg-sp-ds-style

Enjoy DS Style!
