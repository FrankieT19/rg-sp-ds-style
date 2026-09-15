# Stock integration and recovery

DS Style is a frontend on top of the RG SP stock OS, not a replacement firmware or an emulator package. It reads the installed stock launch tables for RetroArch and Game Rooms. System launch-mode choices are made with START on the Systems list.

The launcher uses the stock game, thumbnail, favourites and recent-history locations. It retains its own interface preferences under `DSStyle/state`.

Suspend/wake and display handoff use the stock hardware interfaces.

## Autoboot

Enable it through Startup settings. The launcher backs up the prior regular `/mnt/mmc/dmenu.bin` override, if any, into `/mnt/mmc/.dsstyle-v1-boot/`, then installs its small boot shim. This location is on **SD1's ROM partition**, even when DS Style itself lives on SD2. Existing symlink overrides or incompatible stock menu styles are refused. Follow the displayed error instead of manually overwriting them.

Disabling Autoboot checks that the installed shim is still ours and restores the prior file or its prior absence. The backup is then archived under `.dsstyle-v1-restored-*`. A separate archive is created every time Autoboot is successfully disabled, so repeated toggling leaves multiple folders in SD1's root. These archives are retained as recovery records; they are no longer used by DS Style and are safe to delete after the successful restore. Keep the active `.dsstyle-v1-boot` folder and `dmenu.bin` while Autoboot is enabled. **System → Stock OS** returns to stock without disabling the next boot's autoboot setting.

## Recovering access

- To bypass automatic game launching, hold physical **START** during startup. This reaches DS Style instead of the last game.
- To skip the frontend completely, shut down, connect the installation card to a PC and create an empty file `Roms/APPS/DSStyle/DISABLED` with no extension. Reinsert the card and boot. The shim skips this installation and opens stock.
- Keep DS Style's files present. Open it manually from stock Apps and switch Startup → Autoboot off before uninstalling.
- If the frontend itself cannot run, download [DS Style Disable.sh](../tools/developer-apps/DS%20Style%20Disable.sh), place it beside `DS Style.sh` in `Roms/APPS` and run it from stock Apps. It invokes the existing boot manager's restore operation. Remove the recovery helper afterwards.

Do not manually remove or replace the backup when the boot manager reports an unexpected override: another tool may have changed it. Preserve the files and report the exact message.

## Compatibility limits

Stock and stock-mod firmware layouts can change. Unknown emulator layouts fail with an explanation rather than silently selecting a guessed core. Return to Stock OS to use an unsupported route.
