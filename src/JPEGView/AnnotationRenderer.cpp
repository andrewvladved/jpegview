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
			float fWidth = max(1.0f, annotation.fPenWidth * fScale);
			Gdiplus::Pen pen(color, fWidth);
			pen.SetStartCap(Gdiplus::LineCapRound);
			pen.SetEndCap(Gdiplus::LineCapRound);
			pen.SetLineJoin(Gdiplus::LineJoinRound);
			std::vector<Gdiplus::PointF> pts;
			pts.reserve(annotation.points.size());
			for (size_t i = 0; i < annotation.points.size(); i++) {
				Gdiplus::PointF pt = Transform(annotation.points[i], fScale, ptOrigin);
				// Consecutive identical points carry no direction for the arrow head and
				// add nothing to the line, so they are dropped.
				if (pts.empty() || pts.back().X != pt.X || pts.back().Y != pt.Y) {
					pts.push_back(pt);
				}
			}
			if (pts.size() < 2) {
				return; // the whole stroke collapsed onto one point
			}
			// The arrow replaces the round end cap and GDI+ orients it along the last
			// segment by itself, which is why the duplicates above had to go.
			if (annotation.bArrowHead) {
				// AdjustableArrowCap takes the height (from base to tip) first and the
				// width across the base second, both in units of the pen width.
				// SetCustomEndCap copies the cap, so this local one may go out of scope.
				Gdiplus::AdjustableArrowCap arrowCap(ARROW_LENGTH_IN_PEN_WIDTHS,
					ARROW_LENGTH_IN_PEN_WIDTHS * 0.6f, TRUE);
				pen.SetCustomEndCap(&arrowCap);
			}
			// One DrawLines call, so that where a translucent stroke overlaps itself the
			// colour is composited once instead of darkening at every segment joint.
			g.DrawLines(&pen, &pts[0], (INT)pts.size());
			break;
		}
		case AT_Rectangle:
		case AT_Ellipse:
		case AT_Triangle: {
			if (annotation.points.size() < 2) {
				return;
			}
			Gdiplus::PointF ptTopLeft = Transform(annotation.points[0], fScale, ptOrigin);
			Gdiplus::PointF ptBottomRight = Transform(annotation.points[1], fScale, ptOrigin);
			Gdiplus::RectF rect(ptTopLeft.X, ptTopLeft.Y,
				ptBottomRight.X - ptTopLeft.X, ptBottomRight.Y - ptTopLeft.Y);
			Gdiplus::SolidBrush brush(color);
			Gdiplus::Pen pen(color, max(1.0f, annotation.fPenWidth * fScale));
			pen.SetLineJoin(Gdiplus::LineJoinMiter);
			if (annotation.eType == AT_Ellipse) {
				if (annotation.bFilled) {
					g.FillEllipse(&brush, rect);
				} else {
					g.DrawEllipse(&pen, rect);
				}
			} else if (annotation.eType == AT_Triangle) {
				// Apex at the top centre of the dragged box, base along its bottom edge.
				Gdiplus::PointF corners[3];
				corners[0] = Gdiplus::PointF(rect.X + rect.Width / 2, rect.Y);
				corners[1] = Gdiplus::PointF(rect.X + rect.Width, rect.Y + rect.Height);
				corners[2] = Gdiplus::PointF(rect.X, rect.Y + rect.Height);
				if (annotation.bFilled) {
					g.FillPolygon(&brush, corners, 3);
				} else {
					g.DrawPolygon(&pen, corners, 3);
				}
			} else {
				if (annotation.bFilled) {
					g.FillRectangle(&brush, rect);
				} else {
					g.DrawRectangle(&pen, rect);
				}
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
