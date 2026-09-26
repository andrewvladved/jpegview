#include "ScrollMath.h"

namespace ScrollMath {

void Reset(SState& state, int nMaxOffsetY) {
	state.ePhase = PHASE_HoldTop;
	state.dOffsetY = nMaxOffsetY; // a positive offset shows the top of the image
	state.nPhaseElapsedMs = 0;
	state.bAdvanceToNextImage = false;
}

void Advance(SState& state, int nMaxOffsetY, double dSpeedPixelsPerSecond, int nHoldMs, int nElapsedMs) {
	switch (state.ePhase) {
		case PHASE_HoldTop:
			state.nPhaseElapsedMs += nElapsedMs;
			if (state.nPhaseElapsedMs >= nHoldMs) {
				if (nMaxOffsetY <= 0) {
					// Nothing to glide through, so this image is done after its hold.
					state.bAdvanceToNextImage = true;
				} else {
					state.ePhase = PHASE_Moving;
					state.nPhaseElapsedMs = 0;
				}
			}
			break;
		case PHASE_Moving:
			// Distance is speed times time, so the result does not depend on how often
			// the timer happens to fire.
			state.dOffsetY -= dSpeedPixelsPerSecond * nElapsedMs / 1000.0;
			if (state.dOffsetY <= -nMaxOffsetY) {
				state.dOffsetY = -nMaxOffsetY;
				state.ePhase = PHASE_HoldBottom;
				state.nPhaseElapsedMs = 0;
			}
			break;
		case PHASE_HoldBottom:
			state.nPhaseElapsedMs += nElapsedMs;
			if (state.nPhaseElapsedMs >= nHoldMs) {
				state.bAdvanceToNextImage = true;
			}
			break;
	}
}

}
