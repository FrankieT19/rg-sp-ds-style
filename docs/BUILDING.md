# Building and testing

The checked build uses **Zig 0.14.1**, Python 3 and Windows PowerShell. Download Zig from [ziglang.org](https://ziglang.org/download/). No emulator source or firmware image is needed.

From the repository root:

```powershell
./tools/build.ps1 -Zig 'C:/path/to/zig.exe'
python tools/package.py
```

The build cross-compiles the launcher and stock helpers into `device/Roms/APPS/DSStyle/bin`. It also builds a Windows preview and host tests in `preview`. The device build is static; a few stock display/brightness interfaces need 32-bit helpers while the frontend is AArch64. The ZIP is written to `dist/DS-Style-RG-SP-v1.0.zip`. Compiler caches, preview binaries, tests and personal state are not packaged.

Edit `README.md` for user instructions. Packaging regenerates both copies of `README.txt` from it, omitting repository-only building and contributing guidance; do not edit those copies separately. To refresh them without packaging, run `python tools/sync_readme.py`.

## Checks

With Git for Windows installed (its `sh.exe` supplies the shell used by integration fixtures):

```powershell
python tests/test_v1.py --shell 'C:/Program Files/Git/bin/sh.exe' --preview "$PWD/preview/dsstyle-preview.exe"
python tools/check_defaults.py --zig 'C:/path/to/zig.exe'
```

The tests cover browsing, settings, stock handoff and rendering. C assertion tests are compiled with `-UNDEBUG`. Use the matching Python harness for tests which need fixtures. Tests use sample files, not playable commercial games.

To view the interface without a handheld:

```powershell
./preview/dsstyle-preview.exe --base ./device/Roms/APPS/DSStyle --demo
```

Host checks do not replace device testing of audio, sleep, external controllers, HDMI or unfamiliar stock firmware. For translations, edit `source/locale.json` and run `python tools/generate_locale.py`; the generated header is committed so a normal build needs no code-generation dependency.
