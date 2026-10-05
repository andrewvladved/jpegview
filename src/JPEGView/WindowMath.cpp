#include "WindowMath.h"

namespace WindowMath {

	SPlacement MaximizedPlacement(const RECT& rcMonitor, const RECT& rcWork) {
		SPlacement placement;
		placement.nX = rcWork.left - rcMonitor.left;
		placement.nY = rcWork.top - rcMonitor.top;
		placement.nWidth = rcWork.right - rcWork.left;
		placement.nHeight = rcWork.bottom - rcWork.top;
		return placement;
	}

	RECT ClampToWorkArea(const RECT& rcWindow, const RECT& rcWork) {
		return rcWindow;
	}
}
