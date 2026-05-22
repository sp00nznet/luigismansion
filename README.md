# Luigi's Mansion - Static Recompilation

**A static recompilation of a GameCube game to native PC.**

This project takes *Luigi's Mansion* (GameCube, 2001) and statically recompiles
its PowerPC 750CXe (Gekko) machine code into native x86-64 C++ that runs
directly on Windows. No emulator. No compatibility layer. Just the mansion,
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
into compilable C++ source code. Each PowerPC instruction becomes a C++ expression:

```cpp
// Original GameCube PowerPC:
//   addi r3, r4, 0x20
//   lwz  r5, 0x10(r6)
//   bl   some_function

// Recompiled native C++:
ctx->r[3] = (int32_t)ctx->r[4] + 0x20;
ctx->r[5] = MEM_READ32(ctx->r[6] + 0x10);
some_function(ctx, mem);
```

The generated code compiles with any modern C++20 compiler (MSVC, Clang, GCC)
and runs at full native speed with all compiler optimizations applied.

## Architecture

```
                    +-----------------+
                    | Luigi's Mansion |
                    |      DOL        |
                    |  (PowerPC 750)  |
                    +--------+--------+
                             |
                    +--------v---------+
                    | gcrecomp         |
                    |   PPC Disasm  -> |
                    |   CFG Builder -> |
                    |   C++ Code Gen   |
                    +--------+---------+
                             |
                    +--------v---------+
                    | Recompiled .cpp  |
                    |  (in namespace   |
                    |    recomp::)     |
                    +--------+---------+
                             |
              +--------------+----------------+
              |              |                |
     +--------v------+  +----v-----+  +-------v-------+
     | gcrecomp_gx   |  | gcrecomp_|  | gcrecomp_     |
     | (D3D11)       |  |  audio   |  |   input       |
     | TEV -> HLSL   |  | (XAudio2)|  | (XInput +     |
     | Texture decode|  |          |  |  keyboard)    |
     +---------------+  +----------+  +---------------+
```

The recompiler and runtime libraries live in
[gcrecomp](https://github.com/sp00nznet/gcrecomp), a generic GameCube
recompilation toolkit. This repo is the game-specific layer:
DOL loader, symbol-map import, OS HLE bindings, and the launcher executable.

## DOL Information

| Field             | Value |
|-------------------|-------|
| Game ID           | GLME01 (NTSC-U) |
| DOL Size          | 3.62 MB (3,799,584 bytes) |
| Entry Point       | 0x80003100 |
| Text Sections     | 2 (0x80003100, 0x80005760) |
| Data Sections     | 8 |
| BSS               | 0x803968E0 (size 0x142F8D) |
| SDA base (r13)    | 0x804D6F60 (.sdata + 0x7000) |
| SDA2 base (r2)    | 0x804E0880 (.sdata2 + 0x7000) |
| Decomp Project    | [Yasiki](https://github.com/Moddimation/Yasiki) (100%) |

After recompilation:

| Recompiler output | Value |
|-------------------|-------|
| Functions emitted | 20,457 |
| Source files      | 103 |
| Basic blocks      | 165,458 |
| Instructions      | 1,155,101 |

## Requirements

- A legally obtained copy of *Luigi's Mansion* (GLME01 — US version)
- Windows 10/11 with a DirectX 11 compatible GPU
- CMake 3.20+
- Visual Studio 2022 (or another C++20 compiler that MSBuild supports)
- A clone of [gcrecomp](https://github.com/sp00nznet/gcrecomp) symlinked at `lib/gcrecomp`

## Building

```bash
# Symlink the gcrecomp submodule (one-time)
# (Windows: use `mklink /J lib\gcrecomp <path-to-gcrecomp>` in an admin cmd.)
ln -s /path/to/gcrecomp lib/gcrecomp

# Configure
cmake -B build -G "Visual Studio 17 2022" -A x64

# Build the recompiler and runtime libs
cmake --build build --config Release --target gcrecomp_recompiler

# Recompile the game (needs main.dol + symbols.csv in the repo root)
./build/lib/gcrecomp/Release/gcrecomp_recompiler.exe \
    main.dol \
    --csv symbols.csv \
    --output recompiled/ \
    --project LuigisMansion \
    --funcs-per-file 200 \
    --namespace recomp \
    --trace

# Reconfigure so CMake picks up any newly-generated recomp_NNNN.cpp files,
# then build the launcher
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target lm_launcher

# Run (place your GLME01 ISO + main.dol in the project root)
./build/Release/lm_launcher.exe
# Or specify the ISO path explicitly:
./build/Release/lm_launcher.exe /path/to/game.iso
```

The `--namespace recomp` flag wraps all recompiled functions in a C++ namespace
so the game's own `main` / `memset` / `memcpy` / `exit` etc. don't collide with
the C standard library or the launcher's `main`.

The `--trace` flag emits a `TRACE_ENTER(addr)` call at every recompiled function
entry, which records the call in gcrecomp's lock-free `TraceRing`. The launcher
turns this on before invoking the entry point so any boot wedge can be traced
back to the last function that was called.

## Project Status

### Working

- [x] CMake + MSVC 2022 build pipeline
- [x] Runtime layer (`PPCContext`, `Memory`, `FuncTable`) via gcrecomp
- [x] OS HLE: 70+ function replacements (timing, interrupts, arena, heap, cache, debug, CRT, math, DVD, threading, mutex, message queues)
- [x] Video Interface HLE (`VIInit`, `VIConfigure`, `VIWaitForRetrace`, …)
- [x] Memory Card HLE (`CARD` API — full file I/O backed by host filesystem)
- [x] Init stubs (`OSInit`, `DVDInit`, `PSInit`, `AIInit`, `DSPInit`, `__OSInitSystemCall`)
- [x] Disc image mounting and raw read API
- [x] Low-memory init via `GameConfig` (game ID, RAM size, arena, console type)
- [x] DOL inspector tool (`lm_dol_info`)
- [x] Yasiki symbol-map import (10,987 functions) with SDA base extraction
- [x] DOL loader — copies text/data/BSS sections into emulated memory
- [x] OS function binding — matches symbol names to HLE replacements
- [x] Game loop and entry-point wiring (conditional on recompiled code)
- [x] **Static recompilation pass — 20,457 functions across 103 source files**
- [x] **Namespace isolation (`recomp::`) for the recompiled translation units**
- [x] Shared globals: `lm::g_ctx`/`g_mem`/`g_func_table` alias `gcrecomp::` globals
- [x] **Function-entry trace ring (`TraceRing`) with 10-second watchdog dump in the launcher**

### Boot trace (current state)

1. ✅ Runtime / OS / disc / DOL / symbols all initialize cleanly.
2. ✅ `__init_user @ 0x801E1A20` runs static constructors (20+ ctors execute).
3. ✅ Entry point `__start @ 0x80003100` is called.
4. ✅ `__OSInitAudioSystem @ 0x801DAF70` completes — DSP mailbox poll passes via state-machine HLE in `gcrecomp::Memory`.
5. ✅ OS thread system bring-up (`OSSetCurrentContext`, scheduler init).
6. ✅ `OSEnableInterrupts @ 0x801DDB24` is called — the game is now "running".
7. ✅ **Periodic VI vblank interrupts now fire** — gcrecomp's `InterruptScheduler` (cooperative, hooks into `Memory::translate`) invokes `ExternalInterruptHandler @ 0x801DE2E4` at ~60 Hz with the full `PPCContext` snapshotted around the call.
8. ❌ Wedges polling `RunQueueBits` (0x804D95F8, `.sbss`). The dispatcher fires but the **OS interrupt handler table is never initialized** — `MEM[0x804D8540]` (the table-base pointer at SDA+5728) stays `0x00000000` for the entire run, so the dispatcher's `bclrl` to the registered handler goes nowhere and no thread is ever marked runnable.

The next investigation is **why the handler table is uninitialized**. Either
`OSInit` doesn't run on this boot path (`__start → __init_hardware → ??`),
or it runs but uses a different table location, or some piece of the OS init
sequence lives in code the recompiler missed. The fallback option is to
skip the OS dispatcher entirely and synthesize the VI handler's effect from
the launcher — write `RunQueueBits |= 1` directly to unstick the idle poll.

### Critical architecture note

Recompiled `bl X` instructions emit a **direct C++ call** to `recomp::X(ctx, mem)`.
They do *not* consult `g_func_table`. The function table is only used by
`CALL_INDIRECT` (bctrl, blrl, mtctr+bctr, mtlr+bclrl) and by the launcher's
top-level entries. So `bind_os_functions` only intercepts indirect calls —
a direct `bl OSWhatever` from one recompiled function to another runs the
recompiled body, never the HLE. To intercept a function called via direct
`bl`, you have to either edit the recompiled source, or HLE at a higher
level (intercept the caller), or change the recompiler's `bl` emission to
conditionally consult the func table for a known list of names.

### Recent recompiler / runtime improvements

These fixes landed in [gcrecomp](https://github.com/sp00nznet/gcrecomp) during
LM bring-up; they apply generically to any GameCube binary recompiled with the
toolkit:

- **Jump-table detection** — `bctr` switch-table dispatches now resolve to real
  case targets instead of falling back to the full block-address list.
- **Symbol-map-aware call sites** — `bl` emits the actual symbol-map name
  (`fn_XXXX` or the named function) instead of always hardcoding
  `func_%08X`, which was producing thousands of link-time mismatches.
- **`--namespace <name>` flag** — wraps the entire recompiled output in a C++
  namespace so the game's own `main`/`memset`/`memcpy`/`exit`/`strlen` don't
  collide with the C stdlib or the host program's entry point.
- **CSV-to-CFG promotion** — every `is_function` entry in the symbol map is now
  auto-added as a CFG seed. This catches tiny vtable stubs (size 0x4, 0x8) that
  pure static analysis misses because no `bl` ever references them.
- **`bl`/`bla` fallback** — branches with link to an address the CFG doesn't
  know about (e.g. `bla 0x60`) now route through `CALL_INDIRECT` instead of
  emitting a dangling `func_00000060` reference.
- **DSP mailbox state-machine HLE** — reads of the DSP-to-CPU mailbox HIGH
  (0xCC005004) now simulate the "DSP wrote a message" handshake so OS boot
  proceeds past `__OSInitAudioSystem`.
- **TraceRing + `--trace` codegen** — lock-free 4096-entry function-entry ring
  that you can dump from anywhere. Pairs with the launcher's watchdog to give
  you a precise call-tail whenever the recompiled code wedges.
- **Cooperative interrupt scheduler** — `gcrecomp::InterruptScheduler` (in
  `gcrecomp/interrupts.h` + `src/runtime/interrupts.cpp`). Hooks into
  `Memory::translate`; once every `ops_per_frame` ticks, sets the PI cause
  + mask VI bit and calls the registered external dispatcher synchronously
  with the active `PPCContext`. Snapshots and restores the full register
  file around the dispatcher call so handler clobber doesn't leak into the
  interrupted code (the recompiled handler itself only saves r29-r31 — a
  real PPC exception entry would have saved everything else).

## Standing on the Shoulders of Giants

- [**Moddimation/Yasiki**](https://github.com/Moddimation/Yasiki) — Luigi's Mansion decompilation (100% complete), source of the full symbol map.
- [**N64Recomp**](https://github.com/N64Recomp/N64Recomp) — pioneered the static recompilation approach for N64 games.
- [**Dolphin**](https://github.com/dolphin-emu/dolphin) — the gold standard GameCube/Wii emulator, whose GX shader generation work informs the TEV implementation in gcrecomp.
- [**decomp-toolkit**](https://github.com/encounter/decomp-toolkit) — GameCube binary analysis tools.
- [**ExpansionPak/GCRecompiler**](https://github.com/ExpansionPak/GCRecompiler) — independent prior work on GC static recomp; the jump-table-detection heuristic in gcrecomp is modeled after theirs.

## Legal

This project contains no copyrighted game assets or code. You must provide your
own legally obtained copy of *Luigi's Mansion*.

---

*"Don't be scared. It's just a mansion."*
