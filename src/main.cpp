// =============================================================================
// Luigi's Mansion Static Recompilation - Main Launcher
//
// "Don't be scared. It's just a mansion..."
//
// This is the entry point for the recompiled game.
// It initializes all subsystems, loads the game data, and enters the
// main loop. Luigi's ghost-hunting adventure, running natively on
// Windows 11 — no emulator in sight.
// =============================================================================

#include "lm/runtime.h"
#include "lm/gx/gx.h"
#include "lm/audio/audio.h"
#include "lm/input/input.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <atomic>
#include <cmath>

// Auto-generated function registration (from recompiler output)
extern void register_recompiled_functions(lm::FuncTable& table);

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

using namespace lm;

static const uint32_t WINDOW_WIDTH  = 1280;
static const uint32_t WINDOW_HEIGHT = 720;

static std::atomic<bool> g_game_running{false};

// =============================================================================
// Luigi's Mansion DOL Info (GLME01 — NTSC-U)
//
// Entry point:    0x80003100
// DOL size:       3.62 MB (3,799,584 bytes)
// Text sections:  2 (0x80003100, 0x80005760)
// Data sections:  8
// BSS:            0x803968E0 (size 0x00142F8D)
//
// TODO: Fill in SDA bases from symbol map (Yasiki decomp)
// SDA base (r13):  TODO — extract from DOL or symbol map
// SDA2 base (r2):  TODO — extract from DOL or symbol map
//
// Decomp reference: https://github.com/Moddimation/Yasiki (100% complete!)
// =============================================================================

static const uint32_t GAME_ENTRY_POINT = 0x80003100;

// TODO: Set these from the Yasiki symbol map once available
static const uint32_t SDA_BASE  = 0x00000000; // r13 — to be determined
static const uint32_t SDA2_BASE = 0x00000000; // r2  — to be determined

int main(int argc, char* argv[]) {
    printf("==========================================================\n");
    printf("  Luigi's Mansion - Static Recompilation\n");
    printf("  Game ID: GLME01 (NTSC-U)\n");
    printf("  Decomp:  Yasiki (100%% complete)\n");
    printf("==========================================================\n\n");

    // ---- Initialize runtime ------------------------------------------------
    if (!runtime_init()) {
        fprintf(stderr, "[Error] Failed to initialize runtime!\n");
        return 1;
    }

    // ---- Set SDA bases (r2/r13) -------------------------------------------
    if (SDA_BASE != 0)  g_ctx.r[13] = SDA_BASE;
    if (SDA2_BASE != 0) g_ctx.r[2]  = SDA2_BASE;

    // ---- Mount disc image --------------------------------------------------
    const char* iso_path = "Luigi's Mansion.iso";
    if (argc > 1) {
        iso_path = argv[1];
    }

    if (!lm::mount_disc_image(iso_path, &g_mem)) {
        fprintf(stderr, "[Error] Failed to mount disc image: %s\n", iso_path);
        fprintf(stderr, "  Place your GLME01 ISO in the project directory,\n");
        fprintf(stderr, "  or pass the path as a command-line argument.\n");
        return 1;
    }
    printf("[Main] Disc image mounted: %s\n\n", iso_path);

    // ---- Register recompiled functions -------------------------------------
    // TODO: Uncomment once recompiler has generated the function table
    // register_recompiled_functions(g_func_table);

    // ---- Load DOL into emulated memory ------------------------------------
    // TODO: Load main.dol sections into g_mem at their virtual addresses
    //       The DOL parser from tools/recompiler/dol_parser.cpp can be used here

    // ---- Enter game --------------------------------------------------------
    printf("[Main] Entry point: 0x%08X\n", GAME_ENTRY_POINT);
    printf("[Main] TODO: Call entry point once recompiled code is generated\n");
    printf("[Main] TODO: Set up game loop\n\n");

    // TODO: The actual game entry sequence will look like:
    //
    // // Run static constructors
    // g_func_table.call(__init_user_addr, &g_ctx, &g_mem);
    //
    // // Call main()
    // g_func_table.call(GAME_ENTRY_POINT, &g_ctx, &g_mem);
    //
    // // Main game loop
    // g_game_running = true;
    // while (g_game_running) {
    //     // Per-frame dispatch
    //     g_func_table.call(game_loop_addr, &g_ctx, &g_mem);
    // }

    // ---- Cleanup -----------------------------------------------------------
    runtime_shutdown();

    printf("[Main] Luigi's Mansion exited cleanly.\n");
    return 0;
}
