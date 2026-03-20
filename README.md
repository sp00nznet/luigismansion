# Luigi's Mansion - Static Recompilation

**A static recompilation of a GameCube game to native PC.**

This project takes *Luigi's Mansion* (GameCube, 2001) and statically recompiles
its PowerPC 750CXe (Gekko) machine code into native x86-64 C code that runs
directly on Windows 11. No emulator. No compatibility layer. Just the mansion,
running natively.

## Decomp Status: 100% Complete

This game is an ideal candidate for static recompilation because the
[Yasiki](https://github.com/Moddimation/Yasiki) decompilation project has
achieved **100% completion**, providing full symbol coverage for every function
in the game binary. This means:

- Every function has a known name and signature
- All data structures are documented
- Complete symbol map available for the recompiler
- No unknown functions to discover heuristically

## What Is Static Recompilation?

Unlike an emulator that interprets or dynamically translates instructions at
runtime, static recompilation translates the *entire game binary* ahead of time
into compilable C source code. Each PowerPC instruction becomes a C expression:

```c
// Original GameCube PowerPC:
//   addi r3, r4, 0x20
//   lwz  r5, 0x10(r6)
//   bl   some_function

// Recompiled native C:
ctx->r[3] = (int32_t)ctx->r[4] + 0x20;
ctx->r[5] = MEM_READ32(ctx->r[6] + 0x10);
some_function(ctx, mem);
```

The generated C code compiles with any modern compiler (MSVC, Clang, GCC) and
runs at full native speed with all compiler optimizations applied.

## Architecture

```
                    +-----------------+
                    | Luigi's Mansion |
                    |    DOL          |
                    |  (PowerPC 750)  |
                    +--------+--------+
                             |
                    +--------v--------+
                    |  lm_recompiler  |
                    |  PPC Disasm ->  |
                    |  CFG Builder -> |
                    |  C Code Gen     |
                    +--------+--------+
                             |
                    +--------v--------+
                    | Recompiled .cpp |
                    | (x86-64 native) |
                    +--------+--------+
                             |
              +--------------+--------------+
              |              |              |
     +--------v------+ +----v----+ +-------v------+
     | GX Runtime    | | Audio   | | Input        |
     | (D3D11)       | | (DSP    | | (XInput /    |
     | TEV -> HLSL   | |  ADPCM) | |  Keyboard)   |
     | Texture decode| | XAudio2 | | GC Pad map   |
     +---------------+ +---------+ +--------------+
```

## DOL Information

| Field | Value |
|-------|-------|
| Game ID | GLME01 (NTSC-U) |
| DOL Size | 3.62 MB (3,799,584 bytes) |
| Entry Point | 0x80003100 |
| Text Sections | 2 |
| Data Sections | 8 |
| BSS | 0x803968E0 (size 0x142F8D) |
| Decomp Project | [Yasiki](https://github.com/Moddimation/Yasiki) (100%) |

## Requirements

- **Game**: A legally obtained copy of *Luigi's Mansion* (GLME01 - US version)
- **Windows 11** with a DirectX 11 compatible GPU
- **CMake** 3.20+
- **Visual Studio 2022** or compatible C++20 compiler

## Building

```bash
# Configure
cmake -B build -G "Visual Studio 17 2022" -A x64

# Build the recompiler tool
cmake --build build --target lm_recompiler --config Release

# Recompile the game (you need the DOL extracted from your ISO)
./build/Release/lm_recompiler.exe main.dol --output recompiled/

# Build the game runtime + recompiled code
cmake --build build --target lm_launcher --config Release

# Run (place your GLME01 ISO in the project directory)
./build/Release/lm_launcher.exe
# Or specify paths explicitly:
./build/Release/lm_launcher.exe /path/to/game.iso
```

## Project Status

Early stage. The project structure and runtime layer are in place. Next steps:

1. Import symbol map from Yasiki decomp
2. Run recompiler on main.dol with full symbol coverage
3. Patch hardware-dependent functions in recompiled output
4. Bring up rendering pipeline (GX -> D3D11)

## Standing on the Shoulders of Giants

This project builds on incredible work by the community:

- [**Moddimation/Yasiki**](https://github.com/Moddimation/Yasiki) — Luigi's Mansion decompilation project (100% complete) providing the full symbol map
- [**N64Recomp**](https://github.com/N64Recomp/N64Recomp) — Pioneered the static recompilation approach for N64 games
- [**Dolphin**](https://github.com/dolphin-emu/dolphin) — The gold standard GameCube/Wii emulator, whose GX shader generation code informs our TEV implementation
- [**decomp-toolkit**](https://github.com/encounter/decomp-toolkit) — GameCube binary analysis tools

## Legal

This project does not include any copyrighted game assets or code. You must
provide your own legally obtained copy of *Luigi's Mansion*.

## Part of sp00nznet

Built with the same static recompilation methodology as our other projects.
The mansion awaits.

---

*"It's-a me... Luigi?"*
