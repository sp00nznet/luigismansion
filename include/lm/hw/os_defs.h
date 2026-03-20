#pragma once
// =============================================================================
// Luigi's Mansion Static Recompilation - OS Definitions
//
// Thin wrapper that re-exports gcrecomp's OS definitions under lm::os.
// All definitions live in the gcrecomp library (lib/gcrecomp).
// =============================================================================

#include "gcrecomp/hw/os_defs.h"

namespace lm {
    namespace os = gcrecomp::os;
}
