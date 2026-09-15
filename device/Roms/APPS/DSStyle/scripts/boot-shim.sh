#!/bin/sh
# DS_STYLE_V1_BOOT_SHIM -- removable override, no firmware edits.
stock() { exec /mnt/vendor/bin/dmenu.bin; }
# A missing/disabled UI always falls back to the original vendor menu.
for BASE in /mnt/sdcard/Roms/APPS/DSStyle /mnt/mmc/Roms/APPS/DSStyle; do
    if [ -f "$BASE/bin/dsstyle" ] && [ -f "$BASE/scripts/run.sh" ] && [ ! -e "$BASE/DISABLED" ]; then
        /bin/sh "$BASE/scripts/run.sh" --boot
        stock
    fi
done
stock
