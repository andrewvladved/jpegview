#include "TestFramework.h"
#include "AnnotationGeometry.h"

TEST(ScreenToImageAtZoomOneIsATranslation) {
	CPointF pt = AnnotationGeometry::ScreenToImage(CPoint(150, 120), CPoint(100, 100), 1.0f);
	CHECK_NEAR(pt.x, 50.0, 0.001);
	CHECK_NEAR(pt.y, 20.0, 0.001);
}

TEST(ScreenToImageDividesByZoom) {
	CPointF pt = AnnotationGeometry::ScreenToImage(CPoint(300, 100), CPoint(100, 50), 2.0f);
	CHECK_NEAR(pt.x, 100.0, 0.001);
	CHECK_NEAR(pt.y, 25.0, 0.001);
}

TEST(ImageToScreenIsTheInverseOfScreenToImage) {
	CPoint ptOrigin(37, 91);
	float fZoom = 0.634f;
	CPointF ptImage = AnnotationGeometry::ScreenToImage(CPoint(512, 333), ptOrigin, fZoom);
	CPointF ptBack = AnnotationGeometry::ImageToScreen(ptImage, ptOrigin, fZoom);
	CHECK_NEAR(ptBack.x, 512.0, 0.01);
	CHECK_NEAR(ptBack.y, 333.0, 0.01);
}

// Review Focus 3: with the image fitted to screen there are black borders around it.
// A stroke dragged into them must clamp, or saving would write outside the pixel buffer.
TEST(ClampToImageKeepsPointsInsideBounds) {
	CSize sizeImage(800, 600);
	CPointF ptIn = { -30.0f, -5.0f };
	CPointF ptLow = AnnotationGeometry::ClampToImage(ptIn, sizeImage);
	CHECK_NEAR(ptLow.x, 0.0, 0.001);
	CHECK_NEAR(ptLow.y, 0.0, 0.001);
	CPointF ptOut = { 9999.0f, 700.0f };
	CPointF ptHigh = AnnotationGeometry::ClampToImage(ptOut, sizeImage);
	CHECK_NEAR(ptHigh.x, 799.0, 0.001);
	CHECK_NEAR(ptHigh.y, 599.0, 0.001);
}

TEST(ClampToImageLeavesInteriorPointsAlone) {
	CPointF ptIn = { 400.0f, 300.0f };
	CPointF pt = AnnotationGeometry::ClampToImage(ptIn, CSize(800, 600));
	CHECK_NEAR(pt.x, 400.0, 0.001);
	CHECK_NEAR(pt.y, 300.0, 0.001);
}

// Review Focus 2: dragging a rectangle right-to-left or bottom-to-top gives a negative
// width or height; it must normalise rather than render inverted or vanish.
TEST(NormalizeShapeOrdersTheCornersOfARectangle) {
	CAnnotation a;
	a.eType = AT_Rectangle;
	CPointF p0 = { 300.0f, 250.0f };
	CPointF p1 = { 100.0f, 50.0f };
	a.points.push_back(p0);
	a.points.push_back(p1);
	AnnotationGeometry::NormalizeShape(a);
	CHECK_NEAR(a.points[0].x, 100.0, 0.001);
	CHECK_NEAR(a.points[0].y, 50.0, 0.001);
	CHECK_NEAR(a.points[1].x, 300.0, 0.001);
	CHECK_NEAR(a.points[1].y, 250.0, 0.001);
}

TEST(NormalizeShapeLeavesAlreadyOrderedCornersAlone) {
	CAnnotation a;
	a.eType = AT_Rectangle;
	CPointF p0 = { 10.0f, 20.0f };
	CPointF p1 = { 30.0f, 40.0f };
	a.points.push_back(p0);
	a.points.push_back(p1);
	AnnotationGeometry::NormalizeShape(a);
	CHECK_NEAR(a.points[0].x, 10.0, 0.001);
	CHECK_NEAR(a.points[1].y, 40.0, 0.001);
}

TEST(BoundingBoxCoversTheStrokePlusPenWidth) {
	CAnnotation a;
	a.eType = AT_Freehand;
	a.fPenWidth = 10.0f;
	CPointF p0 = { 100.0f, 100.0f };
	CPointF p1 = { 200.0f, 150.0f };
	a.points.push_back(p0);
	a.points.push_back(p1);
	CRect rect = AnnotationGeometry::BoundingBoxOnScreen(a, CPoint(0, 0), 1.0f);
	CHECK(rect.left <= 95);
	CHECK(rect.top <= 95);
	CHECK(rect.right >= 205);
	CHECK(rect.bottom >= 155);
}

TEST(NormalizeShapeOrdersTheCornersOfAnEllipseAndATriangle) {
	EAnnotationType types[2] = { AT_Ellipse, AT_Triangle };
	for (int i = 0; i < 2; i++) {
		CAnnotation a;
		a.eType = types[i];
		CPointF p0 = { 300.0f, 250.0f };
		CPointF p1 = { 100.0f, 50.0f };
		a.points.push_back(p0);
		a.points.push_back(p1);
		AnnotationGeometry::NormalizeShape(a);
		CHECK_NEAR(a.points[0].x, 100.0, 0.001);
		CHECK_NEAR(a.points[0].y, 50.0, 0.001);
		CHECK_NEAR(a.points[1].x, 300.0, 0.001);
		CHECK_NEAR(a.points[1].y, 250.0, 0.001);
	}
}

TEST(NormalizeShapeLeavesAFreehandStrokeAlone) {
	CAnnotation a;
	a.eType = AT_Freehand;
	CPointF p0 = { 300.0f, 250.0f };
	CPointF p1 = { 100.0f, 50.0f };
	a.points.push_back(p0);
	a.points.push_back(p1);
	AnnotationGeometry::NormalizeShape(a);
	CHECK_NEAR(a.points[0].x, 300.0, 0.001);
	CHECK_NEAR(a.points[1].x, 100.0, 0.001);
}

// The arrow head sticks out past the last point by several times the pen width, so the
// area that has to be repainted is bigger than for a plain stroke.
TEST(BoundingBoxOfAnArrowStrokeIsWiderThanOfAPlainOne) {
	CAnnotation plain;
	plain.eType = AT_Freehand;
	plain.fPenWidth = 10.0f;
	CPointF p0 = { 100.0f, 100.0f };
	CPointF p1 = { 200.0f, 150.0f };
	plain.points.push_back(p0);
	plain.points.push_back(p1);
	CAnnotation arrow = plain;
	arrow.bArrowHead = true;
	CRect rcPlain = AnnotationGeometry::BoundingBoxOnScreen(plain, CPoint(0, 0), 1.0f);
	CRect rcArrow = AnnotationGeometry::BoundingBoxOnScreen(arrow, CPoint(0, 0), 1.0f);
	CHECK(rcArrow.left < rcPlain.left);
	CHECK(rcArrow.top < rcPlain.top);
	CHECK(rcArrow.right > rcPlain.right);
	CHECK(rcArrow.bottom > rcPlain.bottom);
}
