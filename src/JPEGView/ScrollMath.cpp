#include "ScrollMath.h"

namespace ScrollMath {

void Reset(SState& state, int nMaxOffsetY) {
	state.ePhase = PHASE_HoldTop;
	state.dOffsetY = 0;
	state.nPhaseElapsedMs = 0;
	state.bAdvanceToNextImage = false;
}

void Advance(SState& state, int nMaxOffsetY, double dSpeedPixelsPerSecond, int nHoldMs, int nElapsedMs) {
}

}
