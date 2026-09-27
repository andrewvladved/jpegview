#include "FaceMath.h"

namespace FaceMath {

int PickFace(const std::vector<SFace>& faces, bool bSmallest) {
	int nBest = -1;
	double dBestArea = 0;
	for (int i = 0; i < (int)faces.size(); i++) {
		double dArea = faces[i].dWidth * faces[i].dHeight;
		if (nBest < 0 || (bSmallest ? dArea < dBestArea : dArea > dBestArea)) {
			nBest = i;
			dBestArea = dArea;
		}
	}
	return nBest;
}

static double Limit(double dOffset, int nImage, double dZoom, int nWindow) {
	double dMax = (nImage * dZoom - nWindow) / 2;
	if (dMax <= 0) {
		return 0;
	}
	return (dOffset < -dMax) ? -dMax : (dOffset > dMax) ? dMax : dOffset;
}

SLetterbox Letterbox(int nImageWidth, int nImageHeight, int nSquare) {
	SLetterbox letterbox = { 1.0, 0, 0, 0, 0 };
	return letterbox;
}

std::vector<SScoredFace> DecodeDetections(const float* pOutput, int nAnchors, const SLetterbox& letterbox, double dMinScore) {
	return std::vector<SScoredFace>();
}

std::vector<SScoredFace> SuppressOverlaps(std::vector<SScoredFace> faces, double dMaxIoU) {
	return faces;
}

std::vector<SFace> SelectFaces(const std::vector<SScoredFace>& faces, double dThreshold, double dFallback) {
	return std::vector<SFace>();
}

std::vector<SFace> MergeFaces(const std::vector<SFace>& first, const std::vector<SFace>& second) {
	return first;
}

SOffset LimitOffset(SOffset offset, SIZE imageSize, double dZoom, SIZE windowSize) {
	SOffset limited = { Limit(offset.dX, imageSize.cx, dZoom, windowSize.cx), Limit(offset.dY, imageSize.cy, dZoom, windowSize.cy) };
	return limited;
}

SOffset CenterOffset(double dPointX, double dPointY, SIZE imageSize, double dZoom, SIZE windowSize) {
	SOffset offset = { (imageSize.cx / 2.0 - dPointX) * dZoom, (imageSize.cy / 2.0 - dPointY) * dZoom };
	return LimitOffset(offset, imageSize, dZoom, windowSize);
}

SOffset AnchorOffset(double dPointX, double dPointY, SIZE imageSize, double dStartZoom, SOffset startOffset,
	double dZoom, SIZE windowSize) {
	// The point is on screen at window/2 + offset + (point - image/2) * zoom; holding that
	// constant while the zoom changes moves the offset by (image/2 - point) * zoom change.
	SOffset offset = { startOffset.dX + (imageSize.cx / 2.0 - dPointX) * (dZoom - dStartZoom),
		startOffset.dY + (imageSize.cy / 2.0 - dPointY) * (dZoom - dStartZoom) };
	return LimitOffset(offset, imageSize, dZoom, windowSize);
}

}
