param([Parameter(Mandatory=$true)][string]$Zig)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$cacheRoot = Join-Path $projectRoot '.build-cache'
$env:ZIG_GLOBAL_CACHE_DIR = Join-Path $cacheRoot 'global'
$env:ZIG_LOCAL_CACHE_DIR = Join-Path $cacheRoot 'local'
$src = Join-Path $projectRoot 'source/dsstyle.c'
$deviceBin = Join-Path $projectRoot 'device/Roms/APPS/DSStyle/bin/dsstyle'
New-Item -ItemType Directory -Force (Split-Path $deviceBin), (Join-Path $projectRoot 'preview') | Out-Null
& $Zig cc -target aarch64-linux-musl -O2 -static -Wall -Wextra $src -o $deviceBin -lm
if ($LASTEXITCODE) { throw 'Device build failed' }
& $Zig cc -target arm-linux-musleabihf -O2 -static -Wall -Wextra (Join-Path $projectRoot 'source/brightness.c') -o (Join-Path $projectRoot 'device/Roms/APPS/DSStyle/bin/dsstyle-brightness')
if ($LASTEXITCODE) { throw 'Brightness helper build failed' }
& $Zig cc -target aarch64-linux-musl -O2 -static -Wall -Wextra (Join-Path $projectRoot 'source/volume.c') -o (Join-Path $projectRoot 'device/Roms/APPS/DSStyle/bin/dsstyle-volume')
if ($LASTEXITCODE) { throw 'Volume helper build failed' }
& $Zig cc -target aarch64-linux-musl -O2 -static (Join-Path $projectRoot 'source/audio_stream.c') -lm -o (Join-Path $projectRoot 'device/Roms/APPS/DSStyle/bin/dsstyle-audio')
if ($LASTEXITCODE) { throw 'Stock audio worker build failed' }
& $Zig cc -target aarch64-linux-musl -O2 -static (Join-Path $projectRoot 'source/stock_state.c') -o (Join-Path $projectRoot 'device/Roms/APPS/DSStyle/bin/dsstyle-stock-state')
if ($LASTEXITCODE) { throw 'Stock state helper build failed' }
& $Zig cc -target aarch64-linux-musl -O2 -static -Wall -Wextra (Join-Path $projectRoot 'source/stock_ra.c') -o (Join-Path $projectRoot 'device/Roms/APPS/DSStyle/bin/dsstyle-stock-ra')
if ($LASTEXITCODE) { throw 'Stock RA reader build failed' }
& $Zig cc -target x86_64-windows-gnu -O2 (Join-Path $projectRoot 'source/stock_ra.c') -o (Join-Path $projectRoot 'preview/stock-ra-test.exe')
if ($LASTEXITCODE) { throw 'Stock RA host build failed' }
& $Zig cc -target aarch64-linux-musl -O2 -static (Join-Path $projectRoot 'source/stock_room.c') -o (Join-Path $projectRoot 'device/Roms/APPS/DSStyle/bin/dsstyle-stock-room')
if ($LASTEXITCODE) { throw 'Stock Game Rooms reader build failed' }
& $Zig cc -target x86_64-windows-gnu -O2 (Join-Path $projectRoot 'source/stock_state.c') -o (Join-Path $projectRoot 'preview/stock-state-test.exe')
if ($LASTEXITCODE) { throw 'Stock settings host build failed' }
& $Zig cc -target x86_64-windows-gnu -O2 (Join-Path $projectRoot 'source/stock_room.c') -o (Join-Path $projectRoot 'preview/stock-room-test.exe')
if ($LASTEXITCODE) { throw 'Stock Game Rooms host build failed' }
& $Zig cc -target x86_64-windows-gnu -O2 -Wall -Wextra $src -o (Join-Path $projectRoot 'preview/dsstyle-preview.exe') -lgdi32 -luser32
if ($LASTEXITCODE) { throw 'Preview build failed' }
& $Zig cc -target x86_64-windows-gnu -O2 -DAUDIO_OFFLINE (Join-Path $projectRoot 'source/audio_stock.c') -o (Join-Path $projectRoot 'preview/audio-offline-test.exe')
if ($LASTEXITCODE) { throw 'Offline audio test build failed' }
$vendor=Join-Path $projectRoot 'source/vendor/tinyalsa'
& $Zig cc -target x86_64-windows-gnu -O2 -DAUDIO_OFFLINE "-I$vendor/include" (Join-Path $projectRoot 'source/audio_stream.c') -lm -o (Join-Path $projectRoot 'preview/audio-stream-test.exe')
if ($LASTEXITCODE) { throw 'Audio stream host build failed' }
Get-FileHash $deviceBin, (Join-Path $projectRoot 'preview/dsstyle-preview.exe') -Algorithm SHA256

& $Zig cc -target x86_64-windows-gnu -O2 -UNDEBUG (Join-Path $projectRoot "tests/pt_cache_test.c") -o (Join-Path $projectRoot "preview/pt-cache-test.exe") -lgdi32 -luser32
if ($LASTEXITCODE) { throw "Pixel Transparency cache test build failed" }

& $Zig cc -target x86_64-windows-gnu -O2 (Join-Path $projectRoot "source/volume.c") -o (Join-Path $projectRoot "preview/volume-test.exe")
if ($LASTEXITCODE) { throw "Volume test build failed" }

& $Zig cc -target x86_64-windows-gnu -O2 -UNDEBUG (Join-Path $projectRoot "tests/mixer_guard_test.c") -o (Join-Path $projectRoot "preview/mixer-guard-test.exe")
if ($LASTEXITCODE) { throw "Mixer guard test build failed" }

& $Zig cc -target x86_64-windows-gnu -O2 -UNDEBUG (Join-Path $projectRoot "tests/handoff_repeat_test.c") -o (Join-Path $projectRoot "preview/handoff-repeat-test.exe")
if ($LASTEXITCODE) { throw "Handoff/repeat test build failed" }

& $Zig cc -target arm-linux-musleabihf -O2 -static -Wall -Wextra (Join-Path $projectRoot "source/display_stock.c") -o (Join-Path $projectRoot "device/Roms/APPS/DSStyle/bin/dsstyle-display")
if ($LASTEXITCODE) { throw "Display bridge build failed" }
& $Zig cc -target x86_64-windows-gnu -O2 -UNDEBUG (Join-Path $projectRoot "tests/features_test.c") -o (Join-Path $projectRoot "preview/features-test.exe") -lgdi32 -luser32
if ($LASTEXITCODE) { throw "Feature tests build failed" }

& $Zig cc -target x86_64-windows-gnu -O2 -UNDEBUG (Join-Path $projectRoot "tests/v217_test.c") -o (Join-Path $projectRoot "preview/v217-test.exe") -lgdi32 -luser32
if ($LASTEXITCODE) { throw "v217 tests build failed" }
