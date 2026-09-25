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
	ATOOL_Rectangle
};

// Turns mouse input into annotations. Owns the model.
class CAnnotationCtl {
public:
	CAnnotationCtl(IAnnotationHost* pHost);

	// Selecting the tool that is already selected toggles rectangle fill; for the other
	// tools it does nothing. Leaving annotation mode is done with SetTool(ATOOL_None).
	void SetTool(EAnnotationTool eTool);
	EAnnotationTool GetTool() const { return m_eTool; }
	bool IsRectangleFilled() const { return m_bRectangleFilled; }
	bool IsAnnotating() const { return m_eTool != ATOOL_None; }
	bool HasUnsavedAnnotations() const { return m_model.IsDirty(); }

	// Return true when the event was consumed and must not reach panning, cropping or zoom.
	bool OnLButtonDown(int nX, int nY);
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

	void SetStyle(COLORREF color, int nAlpha, int nPenWidthScreen, int nFontSizeScreen);
	COLORREF GetColor() const { return m_color; }
	int GetAlpha() const { return m_nAlpha; }
	int GetPenWidthScreen() const { return m_nPenWidthScreen; }
	int GetFontSizeScreen() const { return m_nFontSizeScreen; }

private:
	CPointF ToImage(int nX, int nY);
	void StartStyle(CAnnotation& annotation, EAnnotationType eType);
	void InvalidatePending();

	IAnnotationHost* m_pHost;
	CAnnotationModel m_model;
	EAnnotationTool m_eTool;
	bool m_bRectangleFilled;
	bool m_bDrawing;
	CAnnotation m_pending;
	bool m_bPendingText;
	CPoint m_ptPendingText;

	COLORREF m_color;
	int m_nAlpha;
	int m_nPenWidthScreen;
	int m_nFontSizeScreen;
};
