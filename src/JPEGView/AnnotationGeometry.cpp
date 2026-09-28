#include "AnnotationGeometry.h"
#include <math.h>

namespace AnnotationGeometry {

CPointF ScreenToImage(CPoint ptScreen, CPoint ptImageOrigin, float fZoom) {
	CPointF pt = { (ptScreen.x - ptImageOrigin.x) / fZoom, (ptScreen.y - ptImageOrigin.y) / fZoom };
	return pt;
}

CPointF ImageToScreen(CPointF ptImage, CPoint ptImageOrigin, float fZoom) {
	CPointF pt = { ptImage.x * fZoom + ptImageOrigin.x, ptImage.y * fZoom + ptImageOrigin.y };
	return pt;
}

CPointF ClampToImage(CPointF pt, CSize sizeImage) {
	float fMaxX = (float)max(0, sizeImage.cx - 1);
	float fMaxY = (float)max(0, sizeImage.cy - 1);
	CPointF ptClamped = { min(max(0.0f, pt.x), fMaxX), min(max(0.0f, pt.y), fMaxY) };
	return ptClamped;
}

void NormalizeShape(CAnnotation& annotation) {
	if (!IsTwoCornerShape(annotation.eType) || annotation.points.size() < 2) {
		return;
	}
	float fLeft = min(annotation.points[0].x, annotation.points[1].x);
	float fTop = min(annotation.points[0].y, annotation.points[1].y);
	float fRight = max(annotation.points[0].x, annotation.points[1].x);
	float fBottom = max(annotation.points[0].y, annotation.points[1].y);
	annotation.points[0].x = fLeft;
	annotation.points[0].y = fTop;
	annotation.points[1].x = fRight;
	annotation.points[1].y = fBottom;
}

CRect BoundingBoxOnScreen(const CAnnotation& annotation, CPoint ptImageOrigin, float fZoom) {
	if (annotation.points.empty()) {
		return CRect(0, 0, 0, 0);
	}
	float fLeft = annotation.points[0].x, fRight = annotation.points[0].x;
	float fTop = annotation.points[0].y, fBottom = annotation.points[0].y;
	for (size_t i = 1; i < annotation.points.size(); i++) {
		fLeft = min(fLeft, annotation.points[i].x);
		fRight = max(fRight, annotation.points[i].x);
		fTop = min(fTop, annotation.points[i].y);
		fBottom = max(fBottom, annotation.points[i].y);
	}
	// Text grows right and down from its anchor; its width is not known here, so use a
	// generous multiple of the font height. Over-invalidating only costs a repaint.
	if (annotation.eType == AT_Text) {
		fRight += annotation.fFontHeight * 40.0f;
		fBottom += annotation.fFontHeight * 1.5f;
	}
	// The arrow head reaches several pen widths past the end of the line, so the area
	// that has to be repainted while it is drawn is correspondingly larger.
	float fMargin = annotation.fPenWidth * (annotation.bArrowHead ? ARROW_LENGTH_IN_PEN_WIDTHS : 1.0f);
	fMargin = max(fMargin, 1.0f);
	CPointF ptTopLeftImage = { fLeft - fMargin, fTop - fMargin };
	CPointF ptBottomRightImage = { fRight + fMargin, fBottom + fMargin };
	CPointF ptTopLeft = ImageToScreen(ptTopLeftImage, ptImageOrigin, fZoom);
	CPointF ptBottomRight = ImageToScreen(ptBottomRightImage, ptImageOrigin, fZoom);
	return CRect((int)floor(ptTopLeft.x), (int)floor(ptTopLeft.y),
		(int)ceil(ptBottomRight.x) + 1, (int)ceil(ptBottomRight.y) + 1);
}

}
