#pragma once
// =============================================================================
// Luigi's Mansion Static Recompilation - Input System
//
// Thin wrapper that re-exports gcrecomp's input API under the lm:: namespace.
// All input implementation lives in the gcrecomp library (lib/gcrecomp).
// =============================================================================

#include "gcrecomp/input/input.h"

namespace lm {
    namespace input = gcrecomp::input;
}
