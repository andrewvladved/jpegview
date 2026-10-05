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

	// A remembered window rect brought back inside the work area: edges that stick out are
	// pulled in, and a rect larger than the work area is sized down to it. A rect that already
	// fits comes back unchanged. Without this, a rect saved while the window was covering the
	// taskbar - or saved on a larger screen - puts the window over the taskbar again on the
	// next start, and every restore from maximized goes back there.
	RECT ClampToWorkArea(const RECT& rcWindow, const RECT& rcWork);
}
