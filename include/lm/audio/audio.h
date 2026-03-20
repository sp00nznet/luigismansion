#pragma once
// =============================================================================
// Luigi's Mansion Static Recompilation - Audio System
//
// Thin wrapper that re-exports gcrecomp's audio API under the lm:: namespace.
// All audio implementation lives in the gcrecomp library (lib/gcrecomp).
// =============================================================================

#include "gcrecomp/audio/audio.h"

namespace lm {
    namespace audio = gcrecomp::audio;
}
