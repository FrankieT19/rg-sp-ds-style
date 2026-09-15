#!/bin/sh
BASE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd) || exit 1
ROOT=${DS_STYLE_TEST_ROOT:-}
case "${1:-}" in
    boot) "$BASE/bin/dsstyle-stock-state" "$BASE" restore ;;
    enter) : ;;
    leave) "$BASE/bin/dsstyle-stock-state" "$BASE" save ;;
    *) exit 2 ;;
esac
if [ -x "$ROOT/mnt/vendor/ctrl/cpu_setting.sh" ]; then
    if [ "${1:-}" = leave ]; then "$ROOT/mnt/vendor/ctrl/cpu_setting.sh" deflt
    else "$ROOT/mnt/vendor/ctrl/cpu_setting.sh" full; fi
fi
if [ "${1:-}" = leave ]; then LED=led_launch.cfg; else LED=led_exit.cfg; fi
if [ -f "$ROOT/mnt/mod/ctrl/configs/$LED" ] && [ -e "$ROOT/sys/class/power_supply/axp2202-battery/work_led" ]; then
    cat "$ROOT/mnt/mod/ctrl/configs/$LED" > "$ROOT/sys/class/power_supply/axp2202-battery/work_led"
fi
exit 0
