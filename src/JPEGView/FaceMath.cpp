#include "FaceMath.h"
#include <algorithm>

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
	SLetterbox letterbox;
	letterbox.dScale = (double)nSquare / max(1, max(nImageWidth, nImageHeight));
	letterbox.nWidth = max(1, min(nSquare, (int)(nImageWidth * letterbox.dScale + 0.5)));
	letterbox.nHeight = max(1, min(nSquare, (int)(nImageHeight * letterbox.dScale + 0.5)));
	letterbox.nPadX = (nSquare - letterbox.nWidth) / 2;
	letterbox.nPadY = (nSquare - letterbox.nHeight) / 2;
	return letterbox;
}

std::vector<SScoredFace> DecodeDetections(const float* pOutput, int nAnchors, const SLetterbox& letterbox, double dMinScore) {
	std::vector<SScoredFace> faces;
	for (int i = 0; i < nAnchors; i++) {
		double dScore = pOutput[4 * nAnchors + i];
		if (dScore < dMinScore) {
			continue;
		}
		double dCenterX = pOutput[i], dCenterY = pOutput[nAnchors + i];
		double dWidth = pOutput[2 * nAnchors + i], dHeight = pOutput[3 * nAnchors + i];
		SScoredFace face;
		face.face.dX = (dCenterX - dWidth / 2 - letterbox.nPadX) / letterbox.dScale;
		face.face.dY = (dCenterY - dHeight / 2 - letterbox.nPadY) / letterbox.dScale;
		face.face.dWidth = dWidth / letterbox.dScale;
		face.face.dHeight = dHeight / letterbox.dScale;
		face.dScore = dScore;
		faces.push_back(face);
	}
	return faces;
}

static double Intersection(const SFace& a, const SFace& b) {
	double dWidth = min(a.dX + a.dWidth, b.dX + b.dWidth) - max(a.dX, b.dX);
	double dHeight = min(a.dY + a.dHeight, b.dY + b.dHeight) - max(a.dY, b.dY);
	return (dWidth > 0 && dHeight > 0) ? dWidth * dHeight : 0;
}

std::vector<SScoredFace> SuppressOverlaps(std::vector<SScoredFace> faces, double dMaxIoU) {
	std::stable_sort(faces.begin(), faces.end(),
		[](const SScoredFace& a, const SScoredFace& b) { return a.dScore > b.dScore; });
	std::vector<SScoredFace> kept;
	for (size_t i = 0; i < faces.size(); i++) {
		bool bOverlaps = false;
		for (size_t j = 0; j < kept.size() && !bOverlaps; j++) {
			double dCommon = Intersection(faces[i].face, kept[j].face);
			double dUnion = faces[i].face.dWidth * faces[i].face.dHeight + kept[j].face.dWidth * kept[j].face.dHeight - dCommon;
			bOverlaps = dUnion > 0 && dCommon / dUnion > dMaxIoU;
		}
		if (!bOverlaps) {
			kept.push_back(faces[i]);
		}
	}
	return kept;
}

std::vector<SFace> SelectFaces(const std::vector<SScoredFace>& faces, double dThreshold, double dFallback) {
	std::vector<SFace> selected;
	int nBest = -1;
	for (int i = 0; i < (int)faces.size(); i++) {
		if (faces[i].dScore >= dThreshold) {
			selected.push_back(faces[i].face);
		}
		if (nBest < 0 || faces[i].dScore > faces[nBest].dScore) {
			nBest = i;
		}
	}
	if (selected.empty() && nBest >= 0 && faces[nBest].dScore >= dFallback) {
		selected.push_back(faces[nBest].face);
	}
	return selected;
}

std::vector<SFace> MergeFaces(const std::vector<SFace>& first, const std::vector<SFace>& second) {
	std::vector<SFace> merged = first;
	for (size_t i = 0; i < second.size(); i++) {
		bool bKnown = false;
		for (size_t j = 0; j < first.size() && !bKnown; j++) {
			double dSmaller = min(second[i].dWidth * second[i].dHeight, first[j].dWidth * first[j].dHeight);
			bKnown = dSmaller > 0 && Intersection(second[i], first[j]) / dSmaller > 0.5;
		}
		if (!bKnown) {
			merged.push_back(second[i]);
		}
	}
	return merged;
}

SOffset LimitOffset(SOffset offset, SIZE imageSize, double dZoom, SIZE windowSize) {
	SOffset limited = { Limit(offset.dX, imageSize.cx, dZoom, windowSize.cx), Limit(offset.dY, imageSize.cy, dZoom, windowSize.cy) };
	return limited;
}

SOffset BlendOffset(SOffset from, SOffset to, double dProgress) {
	double t = (dProgress < 0) ? 0 : (dProgress > 1) ? 1 : dProgress;
	double dEase = t * t * (3 - 2 * t);
	SOffset offset = { from.dX + (to.dX - from.dX) * dEase, from.dY + (to.dY - from.dY) * dEase };
	return offset;
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
