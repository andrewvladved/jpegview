#include "AnnotationGeometry.h"

// RED STUB - compiles, returns wrong answers on purpose. Replaced in the GREEN pass.

namespace AnnotationGeometry {

CPointF ScreenToImage(CPoint ptScreen, CPoint ptImageOrigin, float fZoom) {
	CPointF pt = { 0.0f, 0.0f };
	return pt;
}

CPointF ImageToScreen(CPointF ptImage, CPoint ptImageOrigin, float fZoom) {
	CPointF pt = { 0.0f, 0.0f };
	return pt;
}

CPointF ClampToImage(CPointF pt, CSize sizeImage) {
	return pt;
}

void NormalizeRectangle(CAnnotation& annotation) {
}

CRect BoundingBoxOnScreen(const CAnnotation& annotation, CPoint ptImageOrigin, float fZoom) {
	return CRect(0, 0, 0, 0);
}

}
