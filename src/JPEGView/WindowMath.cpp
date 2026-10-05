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
		// The size the window had is kept wherever it fits, so a window that merely hangs over
		// an edge slides back in rather than being resized.
		int nWidth = rcWindow.right - rcWindow.left;
		int nHeight = rcWindow.bottom - rcWindow.top;
		int nWorkWidth = rcWork.right - rcWork.left;
		int nWorkHeight = rcWork.bottom - rcWork.top;
		if (nWidth > nWorkWidth) {
			nWidth = nWorkWidth;
		}
		if (nHeight > nWorkHeight) {
			nHeight = nWorkHeight;
		}
		int nLeft = rcWindow.left;
		int nTop = rcWindow.top;
		if (nLeft + nWidth > rcWork.right) {
			nLeft = rcWork.right - nWidth;
		}
		if (nTop + nHeight > rcWork.bottom) {
			nTop = rcWork.bottom - nHeight;
		}
		if (nLeft < rcWork.left) {
			nLeft = rcWork.left;
		}
		if (nTop < rcWork.top) {
			nTop = rcWork.top;
		}
		RECT rcResult;
		rcResult.left = nLeft;
		rcResult.top = nTop;
		rcResult.right = nLeft + nWidth;
		rcResult.bottom = nTop + nHeight;
		return rcResult;
	}
}
