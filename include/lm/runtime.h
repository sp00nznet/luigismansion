#pragma once
// =============================================================================
// Luigi's Mansion Static Recompilation - Runtime
//
// Game-specific runtime layer built on top of gcrecomp's generic runtime.
// Re-exports PPCContext, Memory, FuncTable from gcrecomp, and adds
// Luigi's Mansion-specific initialization and OS HLE.
// =============================================================================

#include "gcrecomp/runtime.h"
#include "gcrecomp/yaz0.h"
#include "gcrecomp/rarc.h"

namespace lm {

// Re-export core types from gcrecomp
using PPCContext     = gcrecomp::PPCContext;
using Memory         = gcrecomp::Memory;
using FuncTable      = gcrecomp::FuncTable;
using RecompiledFunc = gcrecomp::RecompiledFunc;
using GameConfig     = gcrecomp::GameConfig;

// ---- Global runtime state (aliases to gcrecomp globals) --------------------
// Recompiled code uses gcrecomp::g_ctx/g_mem/g_func_table directly.
// These references ensure the launcher and OS HLE operate on the same state.
extern PPCContext& g_ctx;
extern Memory&     g_mem;
extern FuncTable&  g_func_table;

bool runtime_init();
void runtime_shutdown();

// ---- Luigi's Mansion OS HLE ------------------------------------------------
void init_low_memory(Memory* mem, const GameConfig& config);
void register_os_functions();
RecompiledFunc lookup_os_func(const char* name);
void set_game_root(const std::string& path);

// ---- Disc image support (delegates to gcrecomp) ----------------------------
inline bool mount_disc_image(const char* p, Memory* m) { return gcrecomp::mount_disc_image(p, m); }
inline size_t disc_read(uint32_t off, void* dst, size_t len) { return gcrecomp::disc_read(off, dst, len); }
inline bool is_disc_mounted() { return gcrecomp::is_disc_mounted(); }

} // namespace lm
