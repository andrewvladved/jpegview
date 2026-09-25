#include "AnnotationRenderer.h"

// RED STUB - compiles and draws nothing. Replaced in the GREEN pass.

Gdiplus::PointF CAnnotationRenderer::Transform(const CPointF& pt, float fScale, const Gdiplus::PointF& ptOrigin) {
	return Gdiplus::PointF(0.0f, 0.0f);
}

Gdiplus::Color CAnnotationRenderer::ToColor(const CAnnotation& annotation) {
	return Gdiplus::Color(0, 0, 0, 0);
}

void CAnnotationRenderer::Render(Gdiplus::Graphics& g, const std::vector<CAnnotation>& annotations,
		float fScale, const Gdiplus::PointF& ptOrigin) {
}

void CAnnotationRenderer::RenderOne(Gdiplus::Graphics& g, const CAnnotation& annotation,
		float fScale, const Gdiplus::PointF& ptOrigin) {
}
