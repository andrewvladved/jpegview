// Where a window without a title bar belongs when it is maximized
/////////////////////////////////////////////////////////////////////////////

#pragma once

// Self-sufficient on purpose: this is compiled into the unit test project as well as
// into JPEGView, so it must not depend on the application's StdAfx.h (which pulls in WTL).
#include <windows.h>

// Windows sizes a maximized window to the monitor's work area - the screen without the
// taskbar - but only for a window that has a title bar. With WS_CAPTION cleared, which is
// what the transparent title bar and the borderless window do, it uses the whole monitor
// instead and the window ends up over the taskbar. WM_GETMINMAXINFO is where that size is
// settled, and this works out what to answer with.
namespace WindowMath {

	// What WM_GETMINMAXINFO asks for: the position is relative to the monitor's top left
	// corner, not to the virtual desktop, which is what makes it work on a second monitor.
	struct SPlacement {
		int nX;
		int nY;
		int nWidth;
		int nHeight;
	};

	// Both rectangles come from MONITORINFO and are in screen coordinates.
	SPlacement MaximizedPlacement(const RECT& rcMonitor, const RECT& rcWork);
}
