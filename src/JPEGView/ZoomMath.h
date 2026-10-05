// Zoom arithmetic shared by the zoom commands and the zoom read-out
/////////////////////////////////////////////////////////////////////////////

#pragma once

// Self-sufficient on purpose: this is compiled into the unit test project as well as
// into JPEGView, so it must not depend on the application's StdAfx.h (which pulls in WTL).
#include <windows.h>
#include <atlbase.h>
#include <atlstr.h>

// In relative zoom mode the image fitted to the window counts as 100%, and every zoom
// command is expressed against that anchor instead of against the image's own pixel size.
// Without the mode the anchor is 1.0, and every function here behaves as JPEGView always
// has, so the call sites need no branch.
namespace ZoomMath {

	// The factor one zoom step multiplies by. Without relative mode the step is tuned per
	// image so that zooming from fit-to-screen lands exactly on the image's own 100% after
	// a whole number of steps - which is why a big photo and a small picture magnify by
	// different amounts per step. In relative mode fit-to-screen is the anchor, so every
	// image gets the same step.
	double StepMultiplier(double dZoomToFit, bool bRelativeZoom);

	// Turns a factor the menu expresses against the anchor into an absolute zoom.
	double AbsoluteZoom(double dFactor, double dBase);

	// The text of the zoom read-out. With an anchor of 1.0 it is a single percentage, as
	// before. With any other anchor the value against the anchor comes first and the true
	// scale against the original pixels follows in brackets.
	CString FormatZoom(double dZoom, double dBase);
}
