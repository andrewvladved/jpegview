// Where zoom mode looks when it follows a face
/////////////////////////////////////////////////////////////////////////////

#pragma once

// Self-sufficient on purpose: this is compiled into the unit test project as well as
// into JPEGView, so it must not depend on the application's StdAfx.h (which pulls in WTL).
#include <windows.h>
#include <vector>

// Offsets are the ones CMainDlg pans with: measured from the centre, and a positive offset
// moves the image right and down. An image point x is on screen at
// window/2 + offset + (x - image/2) * zoom. Every offset returned here is already limited
// the way Helpers::LimitOffsets limits it, so the image never leaves an edge of the window.
namespace FaceMath {

	// A face box in original image pixels: top left corner and size.
	struct SFace {
		double dX, dY, dWidth, dHeight;
	};

	struct SOffset {
		double dX, dY;
	};

	// The index of the smallest (or else the largest) face by area, -1 when there is none.
	int PickFace(const std::vector<SFace>& faces, bool bSmallest);

	// The offset limited so the image at this zoom does not leave an edge of the window.
	SOffset LimitOffset(SOffset offset, SIZE imageSize, double dZoom, SIZE windowSize);

	// The offset that brings the image point to the centre of the window, as far as the
	// edges allow.
	SOffset CenterOffset(double dPointX, double dPointY, SIZE imageSize, double dZoom, SIZE windowSize);

	// The offset that keeps the image point where it was on screen at the start zoom and
	// start offset, so the zoom grows or shrinks around it - as far as the edges allow.
	SOffset AnchorOffset(double dPointX, double dPointY, SIZE imageSize, double dStartZoom, SOffset startOffset,
		double dZoom, SIZE windowSize);
}
