#!/bin/sh
# Sourced only from our own launch.sh. Profile files are parsed as DATA, never sourced.
profile_get() {
    [ -f "$1" ] || return 0
    awk -v key="$2" 'index($0,"=") {k=substr($0,1,index($0,"=")-1);if(k==key){v=substr($0,index($0,"=")+1);sub(/\r$/, "",v);print v;exit}}' "$1"
}
profile_value() {
    value=$(profile_get "$PROFILE" "$1")
    [ -n "$value" ] || value=$(profile_get "$BASE/config/profiles/$SYS.conf" "$1")
    [ -n "$value" ] || value=$(profile_get "$BASE/config/launch.conf" "$1")
    printf '%s' "$value"
}
root_path() {
    case "$1" in /*) printf '%s%s' "$ROOT" "$1" ;; *) fail "Expected an absolute stock path: $1" ;; esac
}
prepare_profile() {
    # An exact-game profile overrides its system profile. A capture is never
    # generalized to other games because it may include per-game flags/paths.
    PROFILE=
    RELATIVE_ROM=${ROM#"$ROOT"}
    if [ -f "$BASE/config/game-profiles.map" ]; then
        NAME=$(DS_PROFILE_ROM="$RELATIVE_ROM" awk -F '|' '$1==ENVIRON["DS_PROFILE_ROM"] {sub(/\r$/,"",$2);print $2;exit}' "$BASE/config/game-profiles.map")
        case "$NAME" in '') ;; *[!a-zA-Z0-9_.-]*|.*) fail 'Invalid game profile name.' ;; *) PROFILE="$BASE/config/profiles/$NAME"; [ -f "$PROFILE" ] || fail 'Game profile is missing.' ;; esac
    fi
    CORE=$(profile_value core)
    if [ -z "$CORE" ]; then
        CORE=$(awk -F '|' -v s="$SYS" '$1==s {sub(/\r$/, "", $2); print $2; exit}' "$BASE/config/systems.map")
    fi
    # User-selected GBA core affects the system default, not captured game profiles.
    if [ "$SYS" = GBA ] && [ -z "$PROFILE" ] && [ -f "$BASE/state/gba-core.txt" ]; then
        case "$(cat "$BASE/state/gba-core.txt")" in mgba) CORE=mgba_libretro.so ;; gpsp) CORE=gpsp_libretro.so ;; *) fail 'Invalid GBA core preference.' ;; esac
    fi
    [ "$MODE" != retroarch ] || CORE=menu
    [ -n "$CORE" ] || fail "No launch mapping for $SYS. Use the stock menu."
    case "$CORE" in *[!a-zA-Z0-9_.-]*|.*) fail 'Invalid core filename.' ;; esac
    RA_WORK=$(profile_value workdir);RA_WORK=${RA_WORK:-/mnt/vendor/deep/retro}
    RA_WORK=$(root_path "$RA_WORK") || exit 20
    EXE="$ROOT/mnt/vendor/deep/retro/retroarch"
    CORE_PATH="$ROOT/mnt/vendor/deep/retro/cores/$CORE"
    CFG=$(profile_value retroarch_config);CFG=${CFG:-/.config/retroarch/retroarch.cfg}
    CFG=$(root_path "$CFG") || exit 20
    PROFILE_HOME=$(profile_value home);PROFILE_HOME=${PROFILE_HOME:-inherit}
    PROFILE_XDG=$(profile_value xdg_config_home);PROFILE_XDG=${PROFILE_XDG:-inherit}
    PROFILE_LD=$(profile_value ld_library_path);PROFILE_LD=${PROFILE_LD:-inherit}
    PROFILE_VIDEO=$(profile_value sdl_video);PROFILE_VIDEO=${PROFILE_VIDEO:-inherit}
    PROFILE_AUDIO=$(profile_value sdl_audio);PROFILE_AUDIO=${PROFILE_AUDIO:-inherit}
    ROUTE=$(profile_value route);ROUTE=${ROUTE:-direct}
    case "$ROUTE" in direct|stock-mod) ;; *) fail 'Unknown stock launch route.' ;; esac
    WRAPPER="$ROOT/mnt/mod/ctrl/RA_launch.sh"
    if [ "$ROUTE" = stock-mod ] && [ ! -x "$WRAPPER" ]; then ROUTE=direct-fallback;fi
    APPEND=$(profile_value appendconfig)
    SAVE=$(profile_value savefile);STATE=$(profile_value savestate)
    if [ "$ROUTE" = stock-mod ] && { [ -n "$APPEND" ] || [ -n "$SAVE" ] || [ -n "$STATE" ]; }; then
        fail 'Explicit append/save/state arguments require route=direct in the profile.'
    fi
    VERBOSE=$(profile_value verbose)
    [ -x "$EXE" ] || fail 'Stock RetroArch executable was not found.'
    [ "$MODE" = retroarch ] || [ -f "$CORE_PATH" ] || fail "The selected stock core is missing: $CORE"
    # The observed Stock Mod wrapper may create/update its system config itself.
    [ "$ROUTE" = stock-mod ] || [ -f "$CFG" ] || fail "Stock RetroArch config missing: $CFG"
    [ -d "$RA_WORK" ] || fail 'Stock working directory is missing.'
    # No copied RetroArch config, no private HOME, no automatic save relocation.
    # Only a specifically configured/captured profile may provide explicit paths.
    if [ -n "$APPEND" ]; then
        case "$APPEND" in *'|'*) fail 'Use a single persistent append config; combined captures need review.' ;; esac
        APPEND=$(root_path "$APPEND") || exit 20
        [ -f "$APPEND" ] || fail 'Stock append config is unavailable. Recapture or use stock.'
    fi
    if [ -n "$SAVE" ]; then SAVE=$(root_path "$SAVE") || exit 20;[ -d "$(dirname -- "$SAVE")" ] || fail 'Configured save parent is missing.';fi
    if [ -n "$STATE" ]; then STATE=$(root_path "$STATE") || exit 20;[ -d "$(dirname -- "$STATE")" ] || fail 'Configured state parent is missing.';fi
    case "$PROFILE_HOME" in inherit|unset) ;; /*) PROFILE_HOME=$(root_path "$PROFILE_HOME") ;; *) fail 'HOME must be inherit, unset or absolute.' ;; esac
    case "$PROFILE_XDG" in inherit|unset) ;; /*) PROFILE_XDG=$(root_path "$PROFILE_XDG") ;; *) fail 'XDG_CONFIG_HOME must be inherit, unset or absolute.' ;; esac
}
run_profile() (
    cd "$RA_WORK" || exit 20
    case "$PROFILE_HOME" in inherit) ;; unset) unset HOME ;; *) HOME=$PROFILE_HOME;export HOME ;; esac
    case "$PROFILE_XDG" in inherit) ;; unset) unset XDG_CONFIG_HOME ;; *) XDG_CONFIG_HOME=$PROFILE_XDG;export XDG_CONFIG_HOME ;; esac
    case "$PROFILE_LD" in inherit) ;; unset) unset LD_LIBRARY_PATH ;; *) LD_LIBRARY_PATH=$PROFILE_LD;export LD_LIBRARY_PATH ;; esac
    case "$PROFILE_VIDEO" in inherit) ;; unset) unset SDL_VIDEODRIVER ;; *) SDL_VIDEODRIVER=$PROFILE_VIDEO;export SDL_VIDEODRIVER ;; esac
    case "$PROFILE_AUDIO" in inherit) ;; unset) unset SDL_AUDIODRIVER ;; *) SDL_AUDIODRIVER=$PROFILE_AUDIO;export SDL_AUDIODRIVER ;; esac
    if [ "$ROUTE" = stock-mod ] && [ "$MODE" != retroarch ]; then
        "$WRAPPER" "$CORE" "$ROM"
        exit $?
    fi
    set -- -c "$CFG" -L "$CORE_PATH"
    [ "$VERBOSE" != true ] || set -- "$@" --verbose
    [ -z "$APPEND" ] || set -- "$@" --appendconfig "$APPEND"
    [ -z "$SAVE" ] || set -- "$@" --save "$SAVE"
    [ -z "$STATE" ] || set -- "$@" --savestate "$STATE"
    [ "$MODE" != retroarch ] || { "$EXE" -c "$CFG" --menu;exit $?; }
    "$EXE" "$@" "$ROM"
)
