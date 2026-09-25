#include "AnnotationRenderer.h"

Gdiplus::PointF CAnnotationRenderer::Transform(const CPointF& pt, float fScale, const Gdiplus::PointF& ptOrigin) {
	return Gdiplus::PointF(pt.x * fScale + ptOrigin.X, pt.y * fScale + ptOrigin.Y);
}

Gdiplus::Color CAnnotationRenderer::ToColor(const CAnnotation& annotation) {
	return Gdiplus::Color((BYTE)annotation.nAlpha, GetRValue(annotation.color),
		GetGValue(annotation.color), GetBValue(annotation.color));
}

void CAnnotationRenderer::Render(Gdiplus::Graphics& g, const std::vector<CAnnotation>& annotations,
		float fScale, const Gdiplus::PointF& ptOrigin) {
	if (annotations.empty()) {
		return;
	}
	Gdiplus::SmoothingMode oldSmoothing = g.GetSmoothingMode();
	Gdiplus::TextRenderingHint oldHint = g.GetTextRenderingHint();
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
	for (size_t i = 0; i < annotations.size(); i++) {
		RenderOne(g, annotations[i], fScale, ptOrigin);
	}
	g.SetTextRenderingHint(oldHint);
	g.SetSmoothingMode(oldSmoothing);
}

void CAnnotationRenderer::RenderOne(Gdiplus::Graphics& g, const CAnnotation& annotation,
		float fScale, const Gdiplus::PointF& ptOrigin) {
	// Fully transparent draws nothing at all, rather than a faint mark.
	if (annotation.nAlpha <= 0 || annotation.points.empty()) {
		return;
	}
	Gdiplus::Color color = ToColor(annotation);

	switch (annotation.eType) {
		case AT_Freehand: {
			if (annotation.points.size() < 2) {
				return;
			}
			Gdiplus::Pen pen(color, max(1.0f, annotation.fPenWidth * fScale));
			pen.SetStartCap(Gdiplus::LineCapRound);
			pen.SetEndCap(Gdiplus::LineCapRound);
			pen.SetLineJoin(Gdiplus::LineJoinRound);
			std::vector<Gdiplus::PointF> pts;
			pts.reserve(annotation.points.size());
			for (size_t i = 0; i < annotation.points.size(); i++) {
				pts.push_back(Transform(annotation.points[i], fScale, ptOrigin));
			}
			// One DrawLines call, so that where a translucent stroke overlaps itself the
			// colour is composited once instead of darkening at every segment joint.
			g.DrawLines(&pen, &pts[0], (INT)pts.size());
			break;
		}
		case AT_Rectangle: {
			if (annotation.points.size() < 2) {
				return;
			}
			Gdiplus::PointF ptTopLeft = Transform(annotation.points[0], fScale, ptOrigin);
			Gdiplus::PointF ptBottomRight = Transform(annotation.points[1], fScale, ptOrigin);
			Gdiplus::RectF rect(ptTopLeft.X, ptTopLeft.Y,
				ptBottomRight.X - ptTopLeft.X, ptBottomRight.Y - ptTopLeft.Y);
			if (annotation.bFilled) {
				Gdiplus::SolidBrush brush(color);
				g.FillRectangle(&brush, rect);
			} else {
				Gdiplus::Pen pen(color, max(1.0f, annotation.fPenWidth * fScale));
				g.DrawRectangle(&pen, rect);
			}
			break;
		}
		case AT_Text: {
			if (annotation.sText.IsEmpty()) {
				return;
			}
			Gdiplus::PointF ptAnchor = Transform(annotation.points[0], fScale, ptOrigin);
			Gdiplus::FontFamily fontFamily(L"Segoe UI");
			Gdiplus::Font font(&fontFamily, max(1.0f, annotation.fFontHeight * fScale),
				Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
			Gdiplus::SolidBrush brush(color);
			g.DrawString(annotation.sText, -1, &font, ptAnchor, &brush);
			break;
		}
	}
}
