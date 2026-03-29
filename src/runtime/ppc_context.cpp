// =============================================================================
// Luigi's Mansion - PPCContext global
// Alias to gcrecomp's global instance so recompiled code and launcher share
// the same CPU state.
// =============================================================================

#include "lm/runtime.h"

namespace lm {

PPCContext& g_ctx = gcrecomp::g_ctx;

} // namespace lm
