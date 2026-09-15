# Pixel Transparency attribution

CPU adaptation informed by Matt Akins (mattakins), Pixel Transparency v2.2,
copyright 2025-2026:

- https://github.com/libretro/slang-shaders/blob/master/handheld/shaders/pixel_transparency/pixel_transparency.slang
- https://github.com/libretro/slang-shaders/blob/master/handheld/shaders/pixel_transparency/pt_base.inc

The launcher implements a static neutral backing, bright-pixel transparency,
paper noise, polarizer, highlight and a shadow offset of one physical output
pixel. Colour cast is reduced and shadow blur is disabled. It omits motion,
shimmer, GPU sampling compensation and final dithering. LCD grid is applied
first when both effects are enabled. Unchanged output and native artwork
are cached. This is a CPU visual adaptation, not a claim of pixel-identical
GPU shader output. No RetroArch preset or setting is installed or changed.
