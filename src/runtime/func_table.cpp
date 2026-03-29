// =============================================================================
// Luigi's Mansion - Function Table and Runtime Init/Shutdown
//
// The FuncTable member functions are provided by gcrecomp_runtime.
// This file defines the global instance and Luigi's Mansion-specific
// initialization that wires up OS HLE and low-memory state.
// =============================================================================

#include "lm/runtime.h"
#include <cstdio>

namespace lm {

FuncTable g_func_table;

bool runtime_init() {
    printf("[Runtime] Initializing Luigi's Mansion runtime...\n");

    // Initialize memory (provided by gcrecomp)
    if (!g_mem.init()) return false;

    // Initialize CPU context
    g_ctx.reset();

    // Set initial stack pointer (top of main RAM - some space for OS)
    g_ctx.r[1] = Memory::MAIN_RAM_BASE + Memory::MAIN_RAM_SIZE - 0x100;

    // Luigi's Mansion game configuration (GLME01 — NTSC-U)
    GameConfig config;
    std::memcpy(config.game_id, "GLME", 4);
    std::memcpy(config.company_code, "01", 2);
    config.disc_number  = 0;
    config.disc_version = 0;
    config.ram_size     = 24 * 1024 * 1024;  // Standard GameCube: 24 MB
    config.console_type = 0x00000001;         // OS_CONSOLE_RETAIL
    config.arena_lo     = 0x80400000;
    config.arena_hi     = 0x81700000;

    // Initialize Dolphin OS low-memory state (clock speeds, arena, game ID, etc.)
    lm::init_low_memory(&g_mem, config);

    // Register OS function replacements (Luigi's Mansion-specific HLE)
    register_os_functions();

    printf("[Runtime] Ready. Stack at 0x%08X\n", g_ctx.r[1]);
    return true;
}

void runtime_shutdown() {
    g_mem.shutdown();
    printf("[Runtime] Shutdown complete.\n");
}

} // namespace lm
