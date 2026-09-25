#include "TestFramework.h"
#include "AnnotationCtl.h"

// The host interface is what makes the controller testable: instead of CMainDlg the
// tests supply this fake, so no window is ever created.
class CFakeHost : public IAnnotationHost {
public:
	CFakeHost() : m_ptOrigin(0, 0), m_fZoom(1.0f), m_sizeImage(800, 600), m_nInvalidateCount(0) {}
	virtual CPoint GetImageOrigin() { return m_ptOrigin; }
	virtual float GetRealizedZoom() { return m_fZoom; }
	virtual CSize GetImageSize() { return m_sizeImage; }
	virtual void InvalidateScreenRect(const CRect& rect) { m_nInvalidateCount++; }

	CPoint m_ptOrigin;
	float m_fZoom;
	CSize m_sizeImage;
	int m_nInvalidateCount;
};

TEST(NoToolMeansMouseIsNotConsumed) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	CHECK(!ctl.IsAnnotating());
	CHECK(!ctl.OnLButtonDown(100, 100));
	CHECK(!ctl.OnMouseMove(110, 110));
	CHECK(!ctl.OnLButtonUp(110, 110));
}

TEST(FreehandDragAddsOneStroke) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Freehand);
	CHECK(ctl.IsAnnotating());
	CHECK(ctl.OnLButtonDown(100, 100));
	CHECK(ctl.OnMouseMove(120, 110));
	CHECK(ctl.OnMouseMove(140, 130));
	CHECK(ctl.OnLButtonUp(140, 130));
	REQUIRE(ctl.Model().Count() == 1);
	CHECK(ctl.Model().Annotations()[0].eType == AT_Freehand);
	CHECK(ctl.Model().Annotations()[0].points.size() == 3);
}

TEST(FreehandClickWithoutDragAddsNothing) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Freehand);
	ctl.OnLButtonDown(100, 100);
	ctl.OnLButtonUp(100, 100);
	CHECK(ctl.Model().IsEmpty());
	CHECK(!ctl.HasUnsavedAnnotations());
}

TEST(FreehandStoresImageCoordinatesNotScreenCoordinates) {
	CFakeHost host;
	host.m_ptOrigin = CPoint(40, 20);
	host.m_fZoom = 2.0f;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Freehand);
	ctl.OnLButtonDown(240, 220);
	ctl.OnMouseMove(440, 420);
	ctl.OnLButtonUp(440, 420);
	REQUIRE(ctl.Model().Count() == 1);
	REQUIRE(ctl.Model().Annotations()[0].points.size() == 2);
	const CAnnotation& ann = ctl.Model().Annotations()[0];
	CHECK_NEAR(ann.points[0].x, 100.0, 0.01);
	CHECK_NEAR(ann.points[0].y, 100.0, 0.01);
	CHECK_NEAR(ann.points[1].x, 200.0, 0.01);
	CHECK_NEAR(ann.points[1].y, 200.0, 0.01);
}

TEST(PenWidthIsStoredInImagePixels) {
	CFakeHost host;
	host.m_fZoom = 4.0f;
	CAnnotationCtl ctl(&host);
	ctl.SetStyle(RGB(255, 0, 0), 200, 8, 24);
	ctl.SetTool(ATOOL_Freehand);
	ctl.OnLButtonDown(0, 0);
	ctl.OnMouseMove(40, 40);
	ctl.OnLButtonUp(40, 40);
	REQUIRE(ctl.Model().Count() == 1);
	// 8 screen pixels at zoom 4 is 2 image pixels
	CHECK_NEAR(ctl.Model().Annotations()[0].fPenWidth, 2.0, 0.01);
}

TEST(StrokeIsClampedToTheImage) {
	CFakeHost host;
	host.m_sizeImage = CSize(200, 200);
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Freehand);
	ctl.OnLButtonDown(-50, -50);
	ctl.OnMouseMove(5000, 5000);
	ctl.OnLButtonUp(5000, 5000);
	REQUIRE(ctl.Model().Count() == 1);
	REQUIRE(ctl.Model().Annotations()[0].points.size() == 2);
	const CAnnotation& ann = ctl.Model().Annotations()[0];
	CHECK(ann.points[0].x >= 0.0f && ann.points[0].y >= 0.0f);
	CHECK(ann.points[1].x <= 199.0f && ann.points[1].y <= 199.0f);
}

TEST(RectangleDragAddsANormalizedRectangle) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Rectangle);
	ctl.OnLButtonDown(300, 250);
	ctl.OnMouseMove(100, 50);
	ctl.OnLButtonUp(100, 50);
	REQUIRE(ctl.Model().Count() == 1);
	const CAnnotation& ann = ctl.Model().Annotations()[0];
	CHECK(ann.eType == AT_Rectangle);
	CHECK(ann.points[0].x < ann.points[1].x);
	CHECK(ann.points[0].y < ann.points[1].y);
}

TEST(RepeatSelectionOfRectangleTogglesFill) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Rectangle);
	CHECK(!ctl.IsRectangleFilled());
	ctl.SetTool(ATOOL_Rectangle);
	CHECK(ctl.IsRectangleFilled());
	ctl.SetTool(ATOOL_Rectangle);
	CHECK(!ctl.IsRectangleFilled());
}

TEST(SwitchingAwayAndBackResetsFillToOutline) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Rectangle);
	ctl.SetTool(ATOOL_Rectangle);
	CHECK(ctl.IsRectangleFilled());
	ctl.SetTool(ATOOL_Freehand);
	ctl.SetTool(ATOOL_Rectangle);
	CHECK(!ctl.IsRectangleFilled());
}

TEST(FilledRectangleCarriesTheFillFlag) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Rectangle);
	ctl.SetTool(ATOOL_Rectangle); // second press switches to filled
	ctl.OnLButtonDown(10, 10);
	ctl.OnMouseMove(60, 60);
	ctl.OnLButtonUp(60, 60);
	REQUIRE(ctl.Model().Count() == 1);
	CHECK(ctl.Model().Annotations()[0].bFilled);
}

TEST(TextClickRecordsAPendingPositionAndConsumesTheClick) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Text);
	CHECK(ctl.OnLButtonDown(120, 140));
	CHECK(ctl.HasPendingText());
	CHECK(ctl.GetPendingTextPosition() == CPoint(120, 140));
	CHECK(ctl.Model().IsEmpty());
}

TEST(CommitTextAddsATextAnnotation) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetStyle(RGB(0, 255, 0), 255, 4, 30);
	ctl.SetTool(ATOOL_Text);
	ctl.OnLButtonDown(100, 100);
	ctl.CommitText(_T("hello"));
	REQUIRE(ctl.Model().Count() == 1);
	const CAnnotation& ann = ctl.Model().Annotations()[0];
	CHECK(ann.eType == AT_Text);
	CHECK(ann.sText == CString(_T("hello")));
	CHECK_NEAR(ann.fFontHeight, 30.0, 0.01);
}

// Review Focus 4: committing empty text must add nothing and leave the image clean.
TEST(CommitEmptyTextAddsNothing) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Text);
	ctl.OnLButtonDown(100, 100);
	ctl.CommitText(_T(""));
	CHECK(ctl.Model().IsEmpty());
	CHECK(!ctl.HasUnsavedAnnotations());
}

TEST(CancelTextDropsThePendingPosition) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Text);
	ctl.OnLButtonDown(100, 100);
	ctl.CancelText();
	CHECK(!ctl.HasPendingText());
	ctl.CommitText(_T("ignored"));
	CHECK(ctl.Model().IsEmpty());
}

TEST(SetToolNoneLeavesAnnotationMode) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Freehand);
	ctl.SetTool(ATOOL_None);
	CHECK(!ctl.IsAnnotating());
	CHECK(!ctl.OnLButtonDown(10, 10));
}

TEST(LeavingAnnotationModeKeepsTheAnnotations) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Freehand);
	ctl.OnLButtonDown(10, 10);
	ctl.OnMouseMove(50, 50);
	ctl.OnLButtonUp(50, 50);
	ctl.SetTool(ATOOL_None);
	CHECK(ctl.Model().Count() == 1);
	CHECK(ctl.HasUnsavedAnnotations());
}

TEST(MarkSavedClearsTheUnsavedFlagButKeepsAnnotations) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Rectangle);
	ctl.OnLButtonDown(10, 10);
	ctl.OnMouseMove(50, 50);
	ctl.OnLButtonUp(50, 50);
	ctl.MarkSaved();
	CHECK(!ctl.HasUnsavedAnnotations());
	CHECK(ctl.Model().Count() == 1);
}

TEST(DraggingInvalidatesOnlyWhileDrawing) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.OnMouseMove(50, 50);
	CHECK(host.m_nInvalidateCount == 0);
	ctl.SetTool(ATOOL_Freehand);
	ctl.OnLButtonDown(10, 10);
	ctl.OnMouseMove(50, 50);
	CHECK(host.m_nInvalidateCount > 0);
}

TEST(PendingAnnotationIsVisibleOnlyDuringTheDrag) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Freehand);
	CHECK(ctl.PendingAnnotation() == NULL);
	ctl.OnLButtonDown(10, 10);
	ctl.OnMouseMove(50, 50);
	CHECK(ctl.PendingAnnotation() != NULL);
	ctl.OnLButtonUp(50, 50);
	CHECK(ctl.PendingAnnotation() == NULL);
}

// Found in review: the pending rectangle was only normalised on button-up, so while
// dragging up-left GDI+ was handed a negative width and drew nothing at all - the user
// got no preview for that drag direction.
TEST(PendingRectangleIsNormalisedWhileDragging) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Rectangle);
	ctl.OnLButtonDown(300, 250);
	ctl.OnMouseMove(100, 50);
	const CAnnotation* pPending = ctl.PendingAnnotation();
	REQUIRE(pPending != NULL);
	REQUIRE(pPending->points.size() == 2);
	CHECK(pPending->points[0].x < pPending->points[1].x);
	CHECK(pPending->points[0].y < pPending->points[1].y);
}

// Found in review: the host origin is the screen position of image pixel (0,0), which is
// negative whenever the image is larger than the window. The controller must simply
// subtract it, with no assumption that it is positive.
TEST(NegativeHostOriginMapsToPositiveImageCoordinates) {
	CFakeHost host;
	host.m_ptOrigin = CPoint(-1500, -800); // image scrolled left and up, as when panned
	host.m_fZoom = 2.0f;
	host.m_sizeImage = CSize(4000, 4000);
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Freehand);
	ctl.OnLButtonDown(500, 400);
	ctl.OnMouseMove(700, 600);
	ctl.OnLButtonUp(700, 600);
	REQUIRE(ctl.Model().Count() == 1);
	REQUIRE(ctl.Model().Annotations()[0].points.size() == 2);
	const CAnnotation& ann = ctl.Model().Annotations()[0];
	CHECK_NEAR(ann.points[0].x, 1000.0, 0.01); // (500 + 1500) / 2
	CHECK_NEAR(ann.points[0].y, 600.0, 0.01);  // (400 + 800) / 2
	CHECK_NEAR(ann.points[1].x, 1100.0, 0.01);
	CHECK_NEAR(ann.points[1].y, 700.0, 0.01);
}

// Shift+click with the freehand tool draws a straight segment from where the last stroke
// ended to the click, the way Photoshop's brush does.
TEST(ShiftClickDrawsAStraightLineFromTheLastPoint) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Freehand);
	ctl.OnLButtonDown(100, 100);
	ctl.OnMouseMove(150, 100);
	ctl.OnLButtonUp(150, 100);
	REQUIRE(ctl.Model().Count() == 1);

	CHECK(ctl.OnLButtonDownShift(300, 260));
	REQUIRE(ctl.Model().Count() == 2);
	const CAnnotation& line = ctl.Model().Annotations()[1];
	CHECK(line.eType == AT_Freehand);
	REQUIRE(line.points.size() == 2);
	CHECK_NEAR(line.points[0].x, 150.0, 0.01); // where the previous stroke ended
	CHECK_NEAR(line.points[0].y, 100.0, 0.01);
	CHECK_NEAR(line.points[1].x, 300.0, 0.01);
	CHECK_NEAR(line.points[1].y, 260.0, 0.01);
}

TEST(ShiftClickWithNoPreviousPointStartsAnOrdinaryStroke) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Freehand);
	// Nothing drawn yet, so there is no anchor to draw a line from.
	CHECK(ctl.OnLButtonDownShift(200, 200));
	CHECK(ctl.Model().IsEmpty());
	ctl.OnMouseMove(260, 240);
	ctl.OnLButtonUp(260, 240);
	CHECK(ctl.Model().Count() == 1);
}

TEST(ShiftClickChainsFromTheEndOfThePreviousShiftLine) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Freehand);
	ctl.OnLButtonDown(10, 10);
	ctl.OnMouseMove(20, 20);
	ctl.OnLButtonUp(20, 20);
	ctl.OnLButtonDownShift(100, 20);
	ctl.OnLButtonDownShift(100, 90);
	REQUIRE(ctl.Model().Count() == 3);
	const CAnnotation& second = ctl.Model().Annotations()[2];
	REQUIRE(second.points.size() == 2);
	CHECK_NEAR(second.points[0].x, 100.0, 0.01); // end of the first shift line
	CHECK_NEAR(second.points[0].y, 20.0, 0.01);
	CHECK_NEAR(second.points[1].y, 90.0, 0.01);
}

TEST(ShiftClickIsIgnoredOutsideTheFreehandTool) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Rectangle);
	ctl.OnLButtonDown(10, 10);
	ctl.OnMouseMove(50, 50);
	ctl.OnLButtonUp(50, 50);
	size_t nBefore = ctl.Model().Count();
	// A rectangle still needs a drag, so a shift-click alone must add nothing.
	ctl.OnLButtonDownShift(200, 200);
	CHECK(ctl.Model().Count() == nBefore);
}
