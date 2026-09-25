#include "StdAfx.h"
#include "TestFramework.h"
#include "AnnotationModel.h"

static CAnnotation MakeStroke(int nPoints) {
	CAnnotation a;
	a.eType = AT_Freehand;
	a.color = RGB(255, 0, 0);
	a.nAlpha = 180;
	a.fPenWidth = 4.0f;
	a.bFilled = false;
	for (int i = 0; i < nPoints; i++) {
		CPointF pt = { (float)i, (float)i };
		a.points.push_back(pt);
	}
	return a;
}

TEST(NewModelIsEmptyAndClean) {
	CAnnotationModel model;
	CHECK(model.IsEmpty());
	CHECK(model.Count() == 0);
	CHECK(!model.IsDirty());
}

TEST(AddMakesModelDirty) {
	CAnnotationModel model;
	model.Add(MakeStroke(5));
	CHECK(model.Count() == 1);
	CHECK(model.IsDirty());
}

TEST(UndoRemovesLastAnnotation) {
	CAnnotationModel model;
	model.Add(MakeStroke(5));
	model.Add(MakeStroke(7));
	CHECK(model.Undo());
	CHECK(model.Count() == 1);
	CHECK(model.Annotations()[0].points.size() == 5);
}

TEST(UndoOnEmptyModelReturnsFalse) {
	CAnnotationModel model;
	CHECK(!model.Undo());
}

TEST(RedoRestoresUndoneAnnotation) {
	CAnnotationModel model;
	model.Add(MakeStroke(5));
	model.Undo();
	CHECK(model.Redo());
	CHECK(model.Count() == 1);
	CHECK(model.Annotations()[0].points.size() == 5);
}

TEST(RedoOnEmptyStackReturnsFalse) {
	CAnnotationModel model;
	model.Add(MakeStroke(5));
	CHECK(!model.Redo());
}

TEST(AddClearsRedoStack) {
	CAnnotationModel model;
	model.Add(MakeStroke(5));
	model.Undo();
	model.Add(MakeStroke(9));
	CHECK(!model.Redo());
	CHECK(model.Count() == 1);
}

TEST(ClearRemovesEverythingIncludingRedo) {
	CAnnotationModel model;
	model.Add(MakeStroke(5));
	model.Add(MakeStroke(6));
	model.Undo();
	model.Clear();
	CHECK(model.IsEmpty());
	CHECK(!model.Redo());
}

TEST(MarkCleanClearsDirtyButKeepsAnnotations) {
	CAnnotationModel model;
	model.Add(MakeStroke(5));
	model.MarkClean();
	CHECK(!model.IsDirty());
	CHECK(model.Count() == 1);
}

TEST(UndoBackToStartLeavesModelClean) {
	CAnnotationModel model;
	model.Add(MakeStroke(5));
	model.Undo();
	CHECK(!model.IsDirty());
}

// Review Focus 1: a click without a drag produces a single point and must be rejected,
// otherwise an invisible annotation makes the image dirty and triggers the save prompt.
TEST(SinglePointStrokeIsRejected) {
	CAnnotationModel model;
	model.Add(MakeStroke(1));
	CHECK(model.IsEmpty());
	CHECK(!model.IsDirty());
}

TEST(EmptyPointListIsRejected) {
	CAnnotationModel model;
	model.Add(MakeStroke(0));
	CHECK(model.IsEmpty());
}
