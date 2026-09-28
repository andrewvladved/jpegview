#include "ScrollMath.h"
#include <math.h>

namespace ScrollMath {

void Reset(SState& state, int nMaxOffsetY) {
	state.ePhase = PHASE_HoldTop;
	state.dOffsetY = nMaxOffsetY; // a positive offset shows the top of the image
	state.nPhaseElapsedMs = 0;
	state.bAdvanceToNextImage = false;
	state.bMovingUp = false;
}

void StartMovingDown(SState& state) {
	state.ePhase = PHASE_Moving;
	state.bMovingUp = false;
	state.nPhaseElapsedMs = 0;
}

void StartMovingUp(SState& state) {
	state.ePhase = PHASE_Moving;
	state.bMovingUp = true;
	state.nPhaseElapsedMs = 0;
}

EAction ActionForStepDown(const SState& state) {
	return (state.ePhase == PHASE_HoldBottom) ? ACTION_NextImage : ACTION_MoveDown;
}

EAction ActionForStepUp(const SState& state) {
	return (state.ePhase == PHASE_HoldTop) ? ACTION_PreviousImage : ACTION_MoveUp;
}

double AccentSpeedFactor(double dOffsetY, int nMaxOffsetY) {
	if (nMaxOffsetY <= 0) {
		return 1.0;
	}
	// Half a cosine wave from the centre to the edge: flat at both ends, so the speed
	// neither jumps nor kinks anywhere along the way.
	const double dPi = 3.14159265358979323846;
	double dDistance = min(1.0, fabs(dOffsetY) / nMaxOffsetY);
	double dLift = (1.0 - cos(dPi * dDistance)) / 2.0;
	return ACCENT_CENTER_SPEED_FACTOR + (1.0 - ACCENT_CENTER_SPEED_FACTOR) * dLift;
}

// How far the image glides in nElapsedMs from dOffsetY. With the accent the speed depends
// on where the image is, so the time is cut into short steps that each use the speed of
// their own spot - otherwise a slow timer would carry the image past the centre at the
// speed of the edge.
static double GlideDistance(double dOffsetY, int nMaxOffsetY, double dSpeedPixelsPerSecond, int nElapsedMs,
	bool bAccentOnCenter, bool bMovingUp) {
	if (!bAccentOnCenter) {
		return dSpeedPixelsPerSecond * nElapsedMs / 1000.0;
	}
	const int nStepMs = 5;
	double dDistance = 0.0;
	for (int nDone = 0; nDone < nElapsedMs; nDone += nStepMs) {
		int nStep = min(nStepMs, nElapsedMs - nDone);
		double dHere = bMovingUp ? dOffsetY + dDistance : dOffsetY - dDistance;
		dDistance += dSpeedPixelsPerSecond * AccentSpeedFactor(dHere, nMaxOffsetY) * nStep / 1000.0;
	}
	return dDistance;
}

double ZoomSpeedUnitsPerSecond(double dPercentPerSecond) {
	return ZOOM_UNITS_PER_E * log(1.0 + dPercentPerSecond / 100.0);
}

int ZoomRunMaxOffset(double dPercentPerSecond, int nDurationMs) {
	if (nDurationMs <= 0) {
		return 0;
	}
	double dSpan = ZoomSpeedUnitsPerSecond(dPercentPerSecond) * nDurationMs / 1000.0;
	return (int)(dSpan / 2.0 + 0.5);
}

double ZoomRunZoomAt(double dOffset, int nMaxOffset, double dStartZoom, bool bOut) {
	double dTravelled = max(0.0, min(2.0 * nMaxOffset, nMaxOffset - dOffset));
	double dFactor = exp(dTravelled / ZOOM_UNITS_PER_E);
	return bOut ? dStartZoom / dFactor : dStartZoom * dFactor;
}

double ZoomRunLimitToFit(double dZoom, double dFitZoom, bool bOut) {
	return (bOut && dZoom < dFitZoom) ? dFitZoom : dZoom;
}

void Advance(SState& state, int nMaxOffsetY, double dSpeedPixelsPerSecond, int nHoldMs, int nElapsedMs,
	bool bAccentOnCenter) {
	switch (state.ePhase) {
		case PHASE_HoldTop:
			state.nPhaseElapsedMs += nElapsedMs;
			if (state.nPhaseElapsedMs >= nHoldMs) {
				if (nMaxOffsetY <= 0) {
					// Nothing to glide through, so this image is done after its hold.
					state.bAdvanceToNextImage = true;
				} else {
					StartMovingDown(state);
				}
			}
			break;
		case PHASE_Moving:
			// Distance is speed times time, so the result does not depend on how often
			// the timer happens to fire.
			if (state.bMovingUp) {
				state.dOffsetY += GlideDistance(state.dOffsetY, nMaxOffsetY, dSpeedPixelsPerSecond, nElapsedMs, bAccentOnCenter, true);
				if (state.dOffsetY >= nMaxOffsetY) {
					state.dOffsetY = nMaxOffsetY;
					state.ePhase = PHASE_HoldTop;
					state.bMovingUp = false;
					state.nPhaseElapsedMs = 0;
				}
			} else {
				state.dOffsetY -= GlideDistance(state.dOffsetY, nMaxOffsetY, dSpeedPixelsPerSecond, nElapsedMs, bAccentOnCenter, false);
				if (state.dOffsetY <= -nMaxOffsetY) {
					state.dOffsetY = -nMaxOffsetY;
					state.ePhase = PHASE_HoldBottom;
					state.nPhaseElapsedMs = 0;
				}
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
