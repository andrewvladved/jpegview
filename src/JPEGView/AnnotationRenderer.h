#pragma once

#include "AnnotationTypes.h"
#include <gdiplus.h>

// The only place annotations are turned into pixels. Called from exactly two places:
// CMainDlg::OnPaint, with the realized zoom as fScale and the image's top left corner
// on screen as ptOrigin; and CJPEGImage::ApplyAnnotationsToOriginalPixels, with
// fScale 1 and origin (0, 0), to burn them into the image being saved.
class CAnnotationRenderer {
public:
	static void Render(Gdiplus::Graphics& g, const std::vector<CAnnotation>& annotations,
		float fScale, const Gdiplus::PointF& ptOrigin);
	static void RenderOne(Gdiplus::Graphics& g, const CAnnotation& annotation,
		float fScale, const Gdiplus::PointF& ptOrigin);

private:
	static Gdiplus::PointF Transform(const CPointF& pt, float fScale, const Gdiplus::PointF& ptOrigin);
	static Gdiplus::Color ToColor(const CAnnotation& annotation);
};
