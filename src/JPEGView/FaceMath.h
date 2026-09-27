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

	// A face found by the neural detector, with how sure it is (0 to 1).
	struct SScoredFace {
		SFace face;
		double dScore;
	};

	// How an image is fitted into the square the neural detector looks at: scaled so its
	// long side fills the square, and centred with the rest padded.
	struct SLetterbox {
		double dScale;
		int nWidth, nHeight; // the scaled image inside the square
		int nPadX, nPadY; // where it starts in the square
	};

	SLetterbox Letterbox(int nImageWidth, int nImageHeight, int nSquare);

	// The detector's output as it comes: five rows of nAnchors values each - centre x,
	// centre y, width, height (all in the square) and score. Returns every box scoring at
	// least dMinScore, in image pixels.
	std::vector<SScoredFace> DecodeDetections(const float* pOutput, int nAnchors, const SLetterbox& letterbox, double dMinScore);

	// Of boxes overlapping by more than dMaxIoU (intersection over union) only the best
	// scoring one is kept. The result is sorted by score, best first.
	std::vector<SScoredFace> SuppressOverlaps(std::vector<SScoredFace> faces, double dMaxIoU);

	// Every face scoring at least dThreshold; when there is none, the best one scoring at
	// least dFallback, so a single less certain face still counts.
	std::vector<SFace> SelectFaces(const std::vector<SScoredFace>& faces, double dThreshold, double dFallback);

	// The first list, plus those of the second that are not already in it - a face counts
	// as the same when most of the smaller box lies inside the other.
	std::vector<SFace> MergeFaces(const std::vector<SFace>& first, const std::vector<SFace>& second);

	// The offset limited so the image at this zoom does not leave an edge of the window.
	SOffset LimitOffset(SOffset offset, SIZE imageSize, double dZoom, SIZE windowSize);

	// The offset that brings the image point to the centre of the window, as far as the
	// edges allow.
	SOffset CenterOffset(double dPointX, double dPointY, SIZE imageSize, double dZoom, SIZE windowSize);

	// On the way from one offset to another: 0 is the start, 1 (and beyond) the end, and
	// the move eases in and out so a face found late does not make the image jump.
	SOffset BlendOffset(SOffset from, SOffset to, double dProgress);

	// The offset that keeps the image point where it was on screen at the start zoom and
	// start offset, so the zoom grows or shrinks around it - as far as the edges allow.
	SOffset AnchorOffset(double dPointX, double dPointY, SIZE imageSize, double dStartZoom, SOffset startOffset,
		double dZoom, SIZE windowSize);
}
