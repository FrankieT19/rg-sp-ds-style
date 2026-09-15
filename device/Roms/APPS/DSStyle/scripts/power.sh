#!/bin/sh
case "${1:-}" in reboot) ACTION=reboot ;; shutdown) ACTION=poweroff ;; *) exit 2 ;; esac
if [ -n "${DS_STYLE_TEST_ROOT:-}" ]; then printf '%s\n' "$ACTION";exit 0;fi
sync
if command -v systemctl >/dev/null 2>&1; then exec systemctl "$ACTION";fi
exec "/sbin/$ACTION"
