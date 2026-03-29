// =============================================================================
// Luigi's Mansion Static Recompilation - Main Launcher
//
// "Don't be scared. It's just a mansion..."
//
// This is the entry point for the recompiled game.
// It initializes all subsystems, loads the DOL into emulated memory,
// imports the symbol map, binds OS functions, and enters the game loop.
// =============================================================================

#include "lm/runtime.h"
#include "lm/gx/gx.h"
#include "lm/audio/audio.h"
#include "lm/input/input.h"
#include "gcrecomp/dol.h"
#include "gcrecomp/symbol_map.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <sstream>
#include <atomic>

// Auto-generated function registration (from recompiler output)
// This is defined in recompiled/recomp_funcs.cpp when generated.
#ifdef LM_HAS_RECOMPILED
extern void register_recompiled_functions(gcrecomp::FuncTable& table);
#endif

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
// SDA base (r13):  0x804D6F60  (.sdata + 0x7000)
// SDA2 base (r2):  0x804E0880  (.sdata2 + 0x7000)
//
// Decomp reference: https://github.com/Moddimation/Yasiki (100% complete)
// =============================================================================

static const uint32_t GAME_ENTRY_POINT = 0x80003100;

// SDA bases extracted from the Yasiki symbol map section layout:
//   .sdata  starts at 0x804CFF60 -> r13 = 0x804CFF60 + 0x7000 = 0x804D6F60
//   .sdata2 starts at 0x804D9880 -> r2  = 0x804D9880 + 0x7000 = 0x804E0880
static const uint32_t SDA_BASE  = 0x804D6F60; // r13
static const uint32_t SDA2_BASE = 0x804E0880; // r2

// =============================================================================
// DOL Loader — copies DOL sections into emulated memory
// =============================================================================

static bool load_dol_into_memory(gcrecomp::DOLFile& dol, Memory& mem) {
    printf("[DOL] Loading DOL into emulated memory...\n");

    for (const auto& sec : dol.sections) {
        if (sec.size == 0) continue;

        uint8_t* dst = mem.translate(sec.address);
        if (!dst) {
            fprintf(stderr, "[DOL] ERROR: Cannot translate VA 0x%08X for section %s%d\n",
                    sec.address, sec.is_text ? "text" : "data", sec.index);
            return false;
        }

        std::memcpy(dst, sec.data.data(), sec.size);
        printf("[DOL]   %s%d: 0x%08X - 0x%08X (%u bytes)\n",
               sec.is_text ? "text" : "data", sec.index,
               sec.address, sec.address + sec.size, sec.size);
    }

    // Zero-fill BSS
    if (dol.bss_size > 0) {
        uint8_t* bss_dst = mem.translate(dol.bss_address);
        if (bss_dst) {
            std::memset(bss_dst, 0, dol.bss_size);
            printf("[DOL]   BSS:   0x%08X - 0x%08X (%u bytes, zeroed)\n",
                   dol.bss_address, dol.bss_address + dol.bss_size, dol.bss_size);
        }
    }

    printf("[DOL] Entry point: 0x%08X\n", dol.entry_point);
    return true;
}

// =============================================================================
// Yasiki Symbol Map Parser
//
// Format: name = .section:0xADDRESS; // type:function size:0xNN ...
// We parse function symbols and their sizes to populate gcrecomp::SymbolMap.
// =============================================================================

static bool load_yasiki_symbols(const char* path, gcrecomp::SymbolMap& map) {
    std::ifstream file(path);
    if (!file.is_open()) {
        fprintf(stderr, "[Symbols] Cannot open: %s\n", path);
        return false;
    }

    uint32_t func_count = 0;
    uint32_t data_count = 0;
    std::string line;

    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;

        // Parse: name = .section:0xADDRESS; // type:TYPE size:0xSIZE ...
        size_t eq = line.find(" = ");
        if (eq == std::string::npos) continue;

        std::string name = line.substr(0, eq);

        // Skip names starting with @ (internal labels like @etb_)
        if (!name.empty() && name[0] == '@') continue;

        // Extract address: find "0x" after ":"
        size_t colon = line.find(':', eq);
        if (colon == std::string::npos) continue;
        size_t addr_start = line.find("0x", colon);
        if (addr_start == std::string::npos) continue;

        uint32_t address = (uint32_t)std::strtoul(line.c_str() + addr_start, nullptr, 16);
        if (address == 0) continue;

        // Extract type from comment
        bool is_function = line.find("type:function") != std::string::npos;

        // Extract size
        uint32_t size = 0;
        size_t size_pos = line.find("size:0x");
        if (size_pos != std::string::npos) {
            size = (uint32_t)std::strtoul(line.c_str() + size_pos + 5, nullptr, 16);
        }

        gcrecomp::Symbol sym;
        sym.address = address;
        sym.size = size;
        sym.name = name;
        sym.is_function = is_function;
        map.symbols[address] = sym;

        if (is_function) func_count++;
        else data_count++;
    }

    printf("[Symbols] Loaded %u functions, %u data symbols from %s\n",
           func_count, data_count, path);
    return true;
}

// =============================================================================
// OS Function Binding — match symbol names to HLE replacements
// =============================================================================

static uint32_t bind_os_functions(const gcrecomp::SymbolMap& map) {
    uint32_t bound = 0;

    for (const auto& [addr, sym] : map.symbols) {
        if (!sym.is_function) continue;

        RecompiledFunc hle = lookup_os_func(sym.name.c_str());
        if (hle) {
            g_func_table.register_func(addr, hle);
            bound++;
        }
    }

    printf("[Bind] Bound %u OS/library functions to HLE replacements\n", bound);
    return bound;
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[]) {
    printf("==========================================================\n");
    printf("  Luigi's Mansion - Static Recompilation\n");
    printf("  Game ID: GLME01 (NTSC-U)\n");
    printf("  Decomp:  Yasiki (100%% complete)\n");
    printf("==========================================================\n\n");

    // ---- Parse command-line ---------------------------------------------------
    const char* iso_path    = "Luigi's Mansion.iso";
    const char* dol_path    = nullptr;
    const char* symbol_path = "symbols.txt";

    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        if (std::strstr(arg, ".iso") || std::strstr(arg, ".gcm")) {
            iso_path = arg;
        } else if (std::strstr(arg, ".dol")) {
            dol_path = arg;
        } else if (std::strstr(arg, "symbol")) {
            symbol_path = arg;
        }
    }

    // ---- Initialize runtime ---------------------------------------------------
    if (!runtime_init()) {
        fprintf(stderr, "[Error] Failed to initialize runtime!\n");
        return 1;
    }

    // ---- Set SDA bases (r2/r13) ----------------------------------------------
    g_ctx.r[13] = SDA_BASE;
    g_ctx.r[2]  = SDA2_BASE;
    printf("[Main] SDA bases: r13=0x%08X  r2=0x%08X\n", SDA_BASE, SDA2_BASE);

    // ---- Mount disc image -----------------------------------------------------
    if (!lm::mount_disc_image(iso_path, &g_mem)) {
        fprintf(stderr, "[Error] Failed to mount disc image: %s\n", iso_path);
        fprintf(stderr, "  Place your GLME01 ISO in the project directory,\n");
        fprintf(stderr, "  or pass the path as a command-line argument.\n");
        // Continue anyway — we can still run if DOL is provided separately
    } else {
        printf("[Main] Disc image mounted: %s\n", iso_path);
    }

    // ---- Load DOL into emulated memory ----------------------------------------
    gcrecomp::DOLFile dol;
    bool dol_loaded = false;

    if (dol_path) {
        // Load from explicit path
        if (dol.load(dol_path)) {
            dol_loaded = load_dol_into_memory(dol, g_mem);
        } else {
            fprintf(stderr, "[Error] Failed to parse DOL: %s\n", dol_path);
        }
    } else {
        // Try extracting from mounted disc (main.dol is at disc offset 0x2440)
        // For now, try loading from filesystem
        const char* fallback_paths[] = { "main.dol", "sys/main.dol", nullptr };
        for (int i = 0; fallback_paths[i]; i++) {
            if (dol.load(fallback_paths[i])) {
                printf("[DOL] Loaded from: %s\n", fallback_paths[i]);
                dol_loaded = load_dol_into_memory(dol, g_mem);
                break;
            }
        }
    }

    if (!dol_loaded) {
        fprintf(stderr, "[Warning] No DOL loaded. Recompiled code will run without memory image.\n");
        fprintf(stderr, "  Provide main.dol via command line or place it in the project directory.\n\n");
    }

    // ---- Import symbol map ----------------------------------------------------
    gcrecomp::SymbolMap symbols;
    if (load_yasiki_symbols(symbol_path, symbols)) {
        bind_os_functions(symbols);
    } else {
        fprintf(stderr, "[Warning] No symbol map loaded. OS function binding skipped.\n");
        fprintf(stderr, "  Place symbols.txt in the project directory or provide path via command line.\n\n");
    }

    // ---- Register recompiled functions ----------------------------------------
#ifdef LM_HAS_RECOMPILED
    register_recompiled_functions(g_func_table);
    printf("[Main] Recompiled functions registered.\n");
#else
    printf("[Main] No recompiled code linked (LM_HAS_RECOMPILED not defined).\n");
    printf("[Main] Run gcrecomp_recompiler on main.dol to generate recompiled/ sources.\n\n");
#endif

    // ---- Enter game -----------------------------------------------------------
    printf("[Main] Entry point: 0x%08X\n", GAME_ENTRY_POINT);

#ifdef LM_HAS_RECOMPILED
    // Look up key entry points from symbol map
    uint32_t init_user_addr = 0;
    for (const auto& [addr, sym] : symbols.symbols) {
        if (sym.name == "__init_user") init_user_addr = addr;
    }

    // Run static constructors (__init_user calls .ctors)
    if (init_user_addr) {
        printf("[Main] Running static constructors (__init_user @ 0x%08X)...\n", init_user_addr);
        g_func_table.call(init_user_addr, &g_ctx, &g_mem);
    }

    // Call entry point (__start -> __init_hardware -> main)
    printf("[Main] Calling entry point...\n");
    g_func_table.call(GAME_ENTRY_POINT, &g_ctx, &g_mem);

    // If the game's main() returns, we're done
    printf("[Main] Game entry point returned.\n");
#else
    printf("[Main] Skipping game execution (no recompiled code).\n");
    printf("[Main] Next steps:\n");
    printf("[Main]   1. Run: gcrecomp_recompiler main.dol --symbols symbols.txt --output recompiled/\n");
    printf("[Main]   2. Rebuild with recompiled code\n");
    printf("[Main]   3. Run again!\n\n");
#endif

    // ---- Cleanup --------------------------------------------------------------
    runtime_shutdown();

    printf("[Main] Luigi's Mansion exited cleanly.\n");
    return 0;
}
