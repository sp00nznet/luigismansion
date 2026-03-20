#pragma once
// =============================================================================
// Luigi's Mansion Static Recompilation - Hardware Constants
//
// Thin wrapper that re-exports gcrecomp's hardware constants under lm::hw.
// All definitions live in the gcrecomp library (lib/gcrecomp).
// =============================================================================

#include "gcrecomp/hw/gc_hw.h"

namespace lm {
    namespace hw = gcrecomp::hw;
}
