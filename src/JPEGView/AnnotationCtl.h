#pragma once

#include "AnnotationModel.h"

// What the annotation controller needs from the window showing the image.
// CMainDlg implements this; the unit tests supply a fake.
class IAnnotationHost {
public:
	virtual ~IAnnotationHost() {}
	virtual CPoint GetImageOrigin() = 0;   // screen position of the image's top left corner
	virtual float GetRealizedZoom() = 0;   // image pixels to screen pixels factor
	virtual CSize GetImageSize() = 0;      // size of the image as currently displayed
	// Named InvalidateScreenRect, not InvalidateRect: CMainDlg implements this interface
	// and already inherits CWindow::InvalidateRect, which would collide.
	virtual void InvalidateScreenRect(const CRect& rect) = 0;
};

enum EAnnotationTool {
	ATOOL_None,
	ATOOL_Freehand,
	ATOOL_Text,
	ATOOL_Shape
};

// The shape tool cycles through these in order.
enum EAnnotationShape {
	SHAPE_Rectangle,
	SHAPE_Ellipse,
	SHAPE_Triangle
};

// Turns mouse input into annotations. Owns the model.
class CAnnotationCtl {
public:
	CAnnotationCtl(IAnnotationHost* pHost);

	// Selecting the tool that is already selected cycles it: the shape tool steps
	// rectangle -> ellipse -> triangle -> rectangle, the freehand tool turns its arrow
	// head on and off, the text tool turns the filled backing behind the glyphs on and
	// off. Leaving annotation mode is done with SetTool(ATOOL_None).
	void SetTool(EAnnotationTool eTool);
	EAnnotationTool GetTool() const { return m_eTool; }
	EAnnotationShape GetShape() const { return m_eShape; }
	bool IsFreehandArrow() const { return m_bFreehandArrow; }
	bool IsTextBackground() const { return m_bTextBackground; }
	COLORREF GetTextBackColor() const { return m_textBackColor; }
	void SetTextBackColor(COLORREF color) { m_textBackColor = color; }

	// Fill has a button of its own, so unlike the shape it is a style setting: it keeps
	// its value while the user switches tools, the way the colour does.
	void ToggleFill() { m_bFill = !m_bFill; }
	bool IsFillEnabled() const { return m_bFill; }
	bool IsAnnotating() const { return m_eTool != ATOOL_None; }
	bool HasUnsavedAnnotations() const { return m_model.IsDirty(); }

	// Return true when the event was consumed and must not reach panning, cropping or zoom.
	bool OnLButtonDown(int nX, int nY);
	// Freehand with Shift held: draws a straight segment from where the last stroke
	// ended to this point, the way a paint program's brush does, and leaves the new
	// end point as the anchor for the next one. With no anchor yet it starts an
	// ordinary stroke instead.
	bool OnLButtonDownShift(int nX, int nY);
	bool OnMouseMove(int nX, int nY);
	bool OnLButtonUp(int nX, int nY);

	// Text tool: OnLButtonDown records where the caret goes; the dialog creates the edit
	// control there and calls CommitText when the user confirms. Empty text adds nothing.
	bool HasPendingText() const { return m_bPendingText; }
	CPoint GetPendingTextPosition() const { return m_ptPendingText; }
	void CancelText() { m_bPendingText = false; }
	void CommitText(LPCTSTR sText);

	void Undo();
	void Redo();
	void Clear();
	void MarkSaved() { m_model.MarkClean(); }

	CAnnotationModel& Model() { return m_model; }
	const CAnnotationModel& Model() const { return m_model; }

	// The annotation being drawn right now, or NULL. Painted on top of the committed ones.
	const CAnnotation* PendingAnnotation() const { return m_bDrawing ? &m_pending : NULL; }

	// The colour is one choice for the whole feature, but the opacity and the width
	// belong to the tool: picking a two pixel line for the shapes must not shrink the
	// text to two pixels as well. The text tool's width is its font size.
	void InitStyle(COLORREF color, int nAlpha, int nPenWidthScreen, int nFontSizeScreen);
	void SetStyleForCurrentTool(COLORREF color, int nAlpha, int nWidthScreen);
	COLORREF GetColor() const { return m_color; }
	int GetAlpha() const { return m_nAlpha[StyleSlot()]; }
	int GetWidthScreen() const { return m_nWidthScreen[StyleSlot()]; }
	int GetFontSizeScreen() const { return m_nWidthScreen[SLOT_Text]; }
	int GetPenWidthScreen() const { return m_nWidthScreen[SLOT_Freehand]; }

private:
	// One set of numbers per tool. ATOOL_None borrows the freehand set, so the style
	// strip still shows something sensible when no tool is selected.
	enum { SLOT_Freehand = 0, SLOT_Text = 1, SLOT_Shape = 2, NUM_STYLE_SLOTS = 3 };
	int StyleSlot() const;

	CPointF ToImage(int nX, int nY);
	EAnnotationType CurrentShapeType() const;
	void StartStyle(CAnnotation& annotation, EAnnotationType eType);
	void InvalidatePending();
	void InvalidateLastSegment();

	IAnnotationHost* m_pHost;
	CAnnotationModel m_model;
	EAnnotationTool m_eTool;
	EAnnotationShape m_eShape;
	bool m_bFill;
	bool m_bFreehandArrow;
	bool m_bTextBackground;
	bool m_bDrawing;
	CAnnotation m_pending;
	// The corner the shape drag started from. Kept apart from m_pending.points,
	// which gets reordered on every move so the live preview has a positive size.
	CPointF m_ptShapeAnchor;
	CPointF m_ptLastStrokeEnd;   // anchor for the next Shift+click line
	bool m_bHasLastStrokeEnd;
	bool m_bPendingText;
	CPoint m_ptPendingText;

	COLORREF m_color;          // shared by every tool
	COLORREF m_textBackColor;  // shared, used only by the text tool
	int m_nAlpha[NUM_STYLE_SLOTS];
	int m_nWidthScreen[NUM_STYLE_SLOTS];
};
