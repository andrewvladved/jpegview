#include "TestFramework.h"
#include "AnnotationRenderer.h"

// GDI+ renders into an in-memory bitmap without needing a window, so the renderer
// can be checked by reading the pixels it produced.

static Gdiplus::Color RenderAndSample(const CAnnotation& annotation, float fScale, int x, int y) {
	Gdiplus::Bitmap bitmap(200, 200, PixelFormat32bppARGB);
	Gdiplus::Graphics g(&bitmap);
	g.Clear(Gdiplus::Color(255, 255, 255, 255));
	std::vector<CAnnotation> annotations;
	annotations.push_back(annotation);
	CAnnotationRenderer::Render(g, annotations, fScale, Gdiplus::PointF(0.0f, 0.0f));
	Gdiplus::Color color;
	bitmap.GetPixel(x, y, &color);
	return color;
}

static CAnnotation MakeFilledRect(int nAlpha) {
	CAnnotation a;
	a.eType = AT_Rectangle;
	a.bFilled = true;
	a.color = RGB(255, 0, 0);
	a.nAlpha = nAlpha;
	a.fPenWidth = 2.0f;
	CPointF p0 = { 50.0f, 50.0f };
	CPointF p1 = { 150.0f, 150.0f };
	a.points.push_back(p0);
	a.points.push_back(p1);
	return a;
}

TEST(OpaqueFilledRectanglePaintsItsColourInside) {
	Gdiplus::Color color = RenderAndSample(MakeFilledRect(255), 1.0f, 100, 100);
	CHECK(color.GetR() == 255);
	CHECK(color.GetG() == 0);
	CHECK(color.GetB() == 0);
}

TEST(FilledRectangleLeavesTheOutsideUntouched) {
	Gdiplus::Color color = RenderAndSample(MakeFilledRect(255), 1.0f, 10, 10);
	CHECK(color.GetR() == 255);
	CHECK(color.GetG() == 255);
	CHECK(color.GetB() == 255);
}

TEST(HalfTransparentFillBlendsWithTheBackground) {
	// Red at alpha 128 over white gives roughly (255, 127, 127).
	Gdiplus::Color color = RenderAndSample(MakeFilledRect(128), 1.0f, 100, 100);
	CHECK(color.GetR() > 240);
	CHECK(color.GetG() > 110 && color.GetG() < 145);
	CHECK(color.GetB() > 110 && color.GetB() < 145);
}

TEST(OutlineRectangleLeavesItsInteriorUntouched) {
	CAnnotation a = MakeFilledRect(255);
	a.bFilled = false;
	Gdiplus::Color color = RenderAndSample(a, 1.0f, 100, 100);
	CHECK(color.GetR() == 255);
	CHECK(color.GetG() == 255);
	CHECK(color.GetB() == 255);
}

TEST(OutlineRectanglePaintsItsBorder) {
	CAnnotation a = MakeFilledRect(255);
	a.bFilled = false;
	a.fPenWidth = 6.0f;
	Gdiplus::Color color = RenderAndSample(a, 1.0f, 50, 100);
	CHECK(color.GetR() == 255);
	CHECK(color.GetG() < 60);
}

TEST(ScaleMovesGeometryProportionally) {
	// At scale 0.5 the rectangle spans image 50..150 -> screen 25..75, so (100,100) is outside.
	Gdiplus::Color inside = RenderAndSample(MakeFilledRect(255), 0.5f, 50, 50);
	CHECK(inside.GetG() == 0);
	Gdiplus::Color outside = RenderAndSample(MakeFilledRect(255), 0.5f, 100, 100);
	CHECK(outside.GetG() == 255);
}

TEST(FreehandStrokePaintsAlongItsPath) {
	CAnnotation a;
	a.eType = AT_Freehand;
	a.color = RGB(0, 0, 255);
	a.nAlpha = 255;
	a.fPenWidth = 8.0f;
	CPointF p0 = { 20.0f, 100.0f };
	CPointF p1 = { 180.0f, 100.0f };
	a.points.push_back(p0);
	a.points.push_back(p1);
	Gdiplus::Color color = RenderAndSample(a, 1.0f, 100, 100);
	CHECK(color.GetB() == 255);
	CHECK(color.GetR() < 60);
}

// Review Focus 5: opacity 0 must render nothing at all rather than a faint mark.
TEST(ZeroAlphaRendersNothing) {
	Gdiplus::Color color = RenderAndSample(MakeFilledRect(0), 1.0f, 100, 100);
	CHECK(color.GetR() == 255);
	CHECK(color.GetG() == 255);
	CHECK(color.GetB() == 255);
}

TEST(EmptyAnnotationListRendersNothing) {
	Gdiplus::Bitmap bitmap(200, 200, PixelFormat32bppARGB);
	Gdiplus::Graphics g(&bitmap);
	g.Clear(Gdiplus::Color(255, 255, 255, 255));
	std::vector<CAnnotation> annotations;
	CAnnotationRenderer::Render(g, annotations, 1.0f, Gdiplus::PointF(0.0f, 0.0f));
	Gdiplus::Color color;
	bitmap.GetPixel(100, 100, &color);
	CHECK(color.GetG() == 255);
}

TEST(TextRendersSomethingAtItsAnchor) {
	CAnnotation a;
	a.eType = AT_Text;
	a.color = RGB(0, 0, 0);
	a.nAlpha = 255;
	a.fFontHeight = 40.0f;
	CPointF anchor = { 20.0f, 60.0f };
	a.points.push_back(anchor);
	a.sText = _T("IIII");

	Gdiplus::Bitmap bitmap(200, 200, PixelFormat32bppARGB);
	Gdiplus::Graphics g(&bitmap);
	g.Clear(Gdiplus::Color(255, 255, 255, 255));
	std::vector<CAnnotation> annotations;
	annotations.push_back(a);
	CAnnotationRenderer::Render(g, annotations, 1.0f, Gdiplus::PointF(0.0f, 0.0f));

	// Rather than guess which pixel a glyph covers, count the non-white ones in the
	// band the text must occupy. Zero of them means nothing was drawn.
	int nDark = 0;
	for (int y = 60; y < 110; y++) {
		for (int x = 20; x < 180; x++) {
			Gdiplus::Color c;
			bitmap.GetPixel(x, y, &c);
			if (c.GetR() < 200) nDark++;
		}
	}
	CHECK(nDark > 20);
}

static CAnnotation MakeShape(EAnnotationType eType, bool bFilled) {
	CAnnotation a;
	a.eType = eType;
	a.bFilled = bFilled;
	a.color = RGB(255, 0, 0);
	a.nAlpha = 255;
	a.fPenWidth = 2.0f;
	CPointF p0 = { 50.0f, 50.0f };
	CPointF p1 = { 150.0f, 150.0f };
	a.points.push_back(p0);
	a.points.push_back(p1);
	return a;
}

TEST(FilledEllipsePaintsItsCentre) {
	Gdiplus::Color color = RenderAndSample(MakeShape(AT_Ellipse, true), 1.0f, 100, 100);
	CHECK(color.GetR() == 255);
	CHECK(color.GetG() == 0);
	CHECK(color.GetB() == 0);
}

// The point just inside the top left of the bounding box is outside the ellipse itself.
// This is what tells an ellipse apart from a rectangle with the same corners.
TEST(FilledEllipseLeavesTheCornerOfItsBoundingBoxUntouched) {
	Gdiplus::Color color = RenderAndSample(MakeShape(AT_Ellipse, true), 1.0f, 56, 56);
	CHECK(color.GetG() > 200);
	CHECK(color.GetB() > 200);
}

TEST(OutlineEllipseLeavesItsInteriorUntouched) {
	Gdiplus::Color color = RenderAndSample(MakeShape(AT_Ellipse, false), 1.0f, 100, 100);
	CHECK(color.GetG() > 200);
	CHECK(color.GetB() > 200);
}

TEST(OutlineEllipsePaintsItsLeftmostPoint) {
	// Halfway down the bounding box the outline crosses its left edge.
	Gdiplus::Color color = RenderAndSample(MakeShape(AT_Ellipse, false), 1.0f, 50, 100);
	CHECK(color.GetG() < 200);
}

// Apex at the top centre, base along the bottom edge: the bottom centre is inside and
// the top left corner of the bounding box is outside.
TEST(FilledTrianglePaintsItsBottomCentre) {
	Gdiplus::Color color = RenderAndSample(MakeShape(AT_Triangle, true), 1.0f, 100, 140);
	CHECK(color.GetR() == 255);
	CHECK(color.GetG() == 0);
	CHECK(color.GetB() == 0);
}

TEST(FilledTriangleLeavesTheTopLeftOfItsBoundingBoxUntouched) {
	Gdiplus::Color color = RenderAndSample(MakeShape(AT_Triangle, true), 1.0f, 56, 56);
	CHECK(color.GetG() > 200);
	CHECK(color.GetB() > 200);
}

TEST(OutlineTriangleLeavesItsInteriorUntouched) {
	Gdiplus::Color color = RenderAndSample(MakeShape(AT_Triangle, false), 1.0f, 100, 120);
	CHECK(color.GetG() > 200);
	CHECK(color.GetB() > 200);
}

TEST(OutlineTrianglePaintsItsBaseline) {
	Gdiplus::Color color = RenderAndSample(MakeShape(AT_Triangle, false), 1.0f, 100, 150);
	CHECK(color.GetG() < 200);
}

static CAnnotation MakeHorizontalStroke(bool bArrow) {
	CAnnotation a;
	a.eType = AT_Freehand;
	a.bArrowHead = bArrow;
	a.color = RGB(255, 0, 0);
	a.nAlpha = 255;
	a.fPenWidth = 6.0f;
	CPointF p0 = { 40.0f, 100.0f };
	CPointF p1 = { 120.0f, 100.0f };
	a.points.push_back(p0);
	a.points.push_back(p1);
	return a;
}

// The arrow head is wider than the line, so a point well off the line near its end is
// painted only when the head is there.
TEST(ArrowHeadPaintsBesideTheEndOfTheStroke) {
	Gdiplus::Color plain = RenderAndSample(MakeHorizontalStroke(false), 1.0f, 108, 112);
	CHECK(plain.GetG() > 200);
	Gdiplus::Color arrow = RenderAndSample(MakeHorizontalStroke(true), 1.0f, 108, 112);
	CHECK(arrow.GetG() < 200);
}

TEST(AStrokeWithoutTheArrowFlagIsUnchanged) {
	Gdiplus::Color color = RenderAndSample(MakeHorizontalStroke(false), 1.0f, 80, 100);
	CHECK(color.GetR() == 255);
	CHECK(color.GetG() == 0);
	CHECK(color.GetB() == 0);
}

// A stroke whose last points coincide has no direction for the head; it must still draw
// the line rather than disappear.
TEST(ArrowStrokeWithRepeatedEndPointStillDrawsTheLine) {
	CAnnotation a = MakeHorizontalStroke(true);
	a.points.push_back(a.points.back());
	a.points.push_back(a.points.back());
	Gdiplus::Color color = RenderAndSample(a, 1.0f, 80, 100);
	CHECK(color.GetR() == 255);
	CHECK(color.GetG() == 0);
}
