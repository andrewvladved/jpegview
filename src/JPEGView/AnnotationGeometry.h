#pragma once

#include "AnnotationTypes.h"

// Conversions between screen pixels and image pixels for annotations.
// ptImageOrigin is the screen position of the image's top left corner, i.e. what
// HelpersGUI::DrawDIB32bppWithBlackBorders returns; fZoom is the realized zoom.
namespace AnnotationGeometry {
	CPointF ScreenToImage(CPoint ptScreen, CPoint ptImageOrigin, float fZoom);
	CPointF ImageToScreen(CPointF ptImage, CPoint ptImageOrigin, float fZoom);

	// Clamps to [0, width-1] x [0, height-1] so a drag into the black borders
	// cannot produce coordinates outside the pixel buffer.
	CPointF ClampToImage(CPointF pt, CSize sizeImage);

	// Orders the two corners of a rectangle annotation so that points[0] is top left.
	void NormalizeRectangle(CAnnotation& annotation);

	// Screen rectangle the annotation paints into, inflated by the pen width, for Invalidate.
	CRect BoundingBoxOnScreen(const CAnnotation& annotation, CPoint ptImageOrigin, float fZoom);
}
