// =============================================================================
// Luigi's Mansion - Memory global
// Alias to gcrecomp's global instance so recompiled code and launcher share
// the same emulated RAM.
// =============================================================================

#include "lm/runtime.h"

namespace lm {

Memory& g_mem = gcrecomp::g_mem;

} // namespace lm
