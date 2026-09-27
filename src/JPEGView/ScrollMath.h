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

	// Starts gliding at once, in either direction, whatever the cycle was doing.
	void StartMovingDown(SState& state);
	void StartMovingUp(SState& state);

	enum EAction {
		ACTION_MoveDown,
		ACTION_MoveUp,
		ACTION_NextImage,
		ACTION_PreviousImage
	};

	// What the down and up keys do while scroll mode runs. They set the image gliding
	// without waiting out the hold - except at the end they are already standing at,
	// where there is nothing left to glide through and they move on to another image.
	EAction ActionForStepDown(const SState& state);
	EAction ActionForStepUp(const SState& state);

	// Moves the cycle on by nElapsedMs. dSpeedPixelsPerSecond is measured on screen, and
	// nHoldMs is how long to stand still at each end.
	//
	// With bAccentOnCenter the glide eases off on its way to the centre of the image, down to
	// ACCENT_CENTER_SPEED_FACTOR of the speed there, and picks up again just as smoothly on
	// its way to the other edge - in both directions.
	void Advance(SState& state, int nMaxOffsetY, double dSpeedPixelsPerSecond, int nHoldMs, int nElapsedMs,
		bool bAccentOnCenter = false);

	// The share of the speed the image glides with at dOffsetY when the centre is accented:
	// ACCENT_CENTER_SPEED_FACTOR at the centre, 1 at either edge, and a smooth curve between
	// that neither jumps at the centre nor at the edges.
	const double ACCENT_CENTER_SPEED_FACTOR = 0.3;
	double AccentSpeedFactor(double dOffsetY, int nMaxOffsetY);

	// Zoom mode runs the very same cycle - hold, move, hold, next image - but what moves is
	// the zoom, from the image fitted to the window to the image filling it with crop (or the
	// other way round with bInverse). The offset of the cycle stands for the logarithm of the
	// zoom, ZOOM_UNITS_PER_E units to a factor of e, so a constant speed in these units is a
	// constant rate of magnification: every second multiplies the zoom by the same factor.
	// +nMaxOffset is where the zoom starts and -nMaxOffset where it ends.
	const double ZOOM_UNITS_PER_E = 10000.0;

	// Half the way from dFitZoom to dCropZoom in those units. Zero when the image has the
	// shape of the window, so the two zooms are one and there is nothing to zoom through.
	int ZoomMaxOffset(double dFitZoom, double dCropZoom);

	// The speed in those units for a zoom changing by dPercentPerSecond every second.
	double ZoomSpeedUnitsPerSecond(double dPercentPerSecond);

	// The zoom at dOffset of the cycle: dFitZoom at the start and dCropZoom at the end, or,
	// with bInverse, dCropZoom at the start and dFitZoom at the end.
	double ZoomAt(double dOffset, int nMaxOffset, double dFitZoom, double dCropZoom, bool bInverse);
}
