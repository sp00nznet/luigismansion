#pragma once
// =============================================================================
// Luigi's Mansion Static Recompilation - GX Graphics API
//
// Thin wrapper that re-exports gcrecomp's GX API under the lm:: namespace.
// All GX implementation lives in the gcrecomp library (lib/gcrecomp).
// =============================================================================

#include "gcrecomp/gx/gx.h"

namespace lm {
    namespace gx = gcrecomp::gx;
}
