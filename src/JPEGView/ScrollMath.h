// The scrolling cycle: hold at the top, glide down, hold at the bottom, next image
/////////////////////////////////////////////////////////////////////////////

#pragma once

// Self-sufficient on purpose: this is compiled into the unit test project as well as
// into JPEGView, so it must not depend on the application's StdAfx.h (which pulls in WTL).
#include <windows.h>

// Scroll mode shows a folder the way a slideshow does, but each image is scaled to fill
// the window (cropping what does not fit), parked at its top edge, held there, glided down
// to its bottom edge and held again before the next image is loaded.
//
// Offsets are the ones CMainDlg pans with: measured from the centre, so +nMaxOffsetY shows
// the top of the image and -nMaxOffsetY the bottom. An image that is not taller than the
// window has nMaxOffsetY == 0 and nothing to glide through.
namespace ScrollMath {

	enum EPhase {
		PHASE_HoldTop,
		PHASE_Moving,
		PHASE_HoldBottom
	};

	struct SState {
		EPhase ePhase;
		double dOffsetY;
		int nPhaseElapsedMs;
		bool bAdvanceToNextImage;
		bool bMovingUp;
	};

	// Parks a freshly loaded image at its top edge.
	void Reset(SState& state, int nMaxOffsetY);

	// Starts gliding at once, in either direction, whatever the cycle was doing. This is
	// what the next and previous image commands do while scroll mode runs: rather than
	// waiting out the hold, the image starts moving straight away.
	void StartMovingDown(SState& state);
	void StartMovingUp(SState& state);

	// Moves the cycle on by nElapsedMs. dSpeedPixelsPerSecond is measured on screen, and
	// nHoldMs is how long to stand still at each end.
	void Advance(SState& state, int nMaxOffsetY, double dSpeedPixelsPerSecond, int nHoldMs, int nElapsedMs);
}
