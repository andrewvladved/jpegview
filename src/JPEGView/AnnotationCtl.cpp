#include "AnnotationCtl.h"
#include "AnnotationGeometry.h"

CAnnotationCtl::CAnnotationCtl(IAnnotationHost* pHost) {
	m_pHost = pHost;
	m_eTool = ATOOL_None;
	m_bRectangleFilled = false;
	m_bDrawing = false;
	m_bPendingText = false;
	m_ptPendingText = CPoint(0, 0);
	m_ptRectAnchor.x = m_ptRectAnchor.y = 0.0f;
	m_ptLastStrokeEnd.x = m_ptLastStrokeEnd.y = 0.0f;
	m_bHasLastStrokeEnd = false;
	m_color = RGB(255, 0, 0);
	m_nAlpha = 180;
	m_nPenWidthScreen = 4;
	m_nFontSizeScreen = 24;
}

void CAnnotationCtl::SetTool(EAnnotationTool eTool) {
	if (eTool == ATOOL_Rectangle && m_eTool == ATOOL_Rectangle) {
		// Pressing the rectangle tool again switches between outline and fill. The panel
		// framework routes left clicks only, so there is no right click to spend on this.
		m_bRectangleFilled = !m_bRectangleFilled;
		return;
	}
	if (eTool != ATOOL_Rectangle) {
		m_bRectangleFilled = false;
	}
	if (eTool != ATOOL_Text) {
		m_bPendingText = false;
	}
	if (eTool != ATOOL_Freehand) {
		m_bHasLastStrokeEnd = false;
	}
	m_eTool = eTool;
	m_bDrawing = false;
}

void CAnnotationCtl::SetStyle(COLORREF color, int nAlpha, int nPenWidthScreen, int nFontSizeScreen) {
	m_color = color;
	m_nAlpha = max(0, min(255, nAlpha));
	m_nPenWidthScreen = max(1, nPenWidthScreen);
	m_nFontSizeScreen = max(4, nFontSizeScreen);
}

CPointF CAnnotationCtl::ToImage(int nX, int nY) {
	CPointF pt = AnnotationGeometry::ScreenToImage(CPoint(nX, nY),
		m_pHost->GetImageOrigin(), m_pHost->GetRealizedZoom());
	return AnnotationGeometry::ClampToImage(pt, m_pHost->GetImageSize());
}

void CAnnotationCtl::StartStyle(CAnnotation& annotation, EAnnotationType eType) {
	float fZoom = m_pHost->GetRealizedZoom();
	if (fZoom <= 0.0f) {
		fZoom = 1.0f;
	}
	annotation = CAnnotation();
	annotation.eType = eType;
	annotation.color = m_color;
	annotation.nAlpha = m_nAlpha;
	// The user picks sizes in screen pixels; they are stored in image pixels so a stroke
	// keeps its apparent thickness while drawing and scales with the image when saved.
	annotation.fPenWidth = m_nPenWidthScreen / fZoom;
	annotation.fFontHeight = m_nFontSizeScreen / fZoom;
	annotation.bFilled = m_bRectangleFilled;
}

// A freehand stroke only ever grows, and the part already on screen is unchanged, so
// repainting its whole bounding box on every mouse move erases and redraws the entire
// line each time - which is what made it flicker. Only the newest segment is dirty.
void CAnnotationCtl::InvalidateLastSegment() {
	size_t nCount = m_pending.points.size();
	if (!m_bDrawing || nCount < 2) {
		InvalidatePending();
		return;
	}
	CAnnotation segment;
	segment.eType = AT_Freehand;
	segment.fPenWidth = m_pending.fPenWidth;
	segment.points.push_back(m_pending.points[nCount - 2]);
	segment.points.push_back(m_pending.points[nCount - 1]);
	m_pHost->InvalidateScreenRect(AnnotationGeometry::BoundingBoxOnScreen(segment,
		m_pHost->GetImageOrigin(), m_pHost->GetRealizedZoom()));
}

void CAnnotationCtl::InvalidatePending() {
	if (!m_bDrawing) {
		return;
	}
	m_pHost->InvalidateScreenRect(AnnotationGeometry::BoundingBoxOnScreen(m_pending,
		m_pHost->GetImageOrigin(), m_pHost->GetRealizedZoom()));
}

bool CAnnotationCtl::OnLButtonDown(int nX, int nY) {
	switch (m_eTool) {
		case ATOOL_None:
			return false;
		case ATOOL_Text:
			m_bPendingText = true;
			m_ptPendingText = CPoint(nX, nY);
			return true;
		case ATOOL_Freehand:
			StartStyle(m_pending, AT_Freehand);
			m_pending.points.push_back(ToImage(nX, nY));
			m_bDrawing = true;
			return true;
		case ATOOL_Rectangle:
			StartStyle(m_pending, AT_Rectangle);
			m_ptRectAnchor = ToImage(nX, nY);
			m_pending.points.push_back(m_ptRectAnchor);
			m_pending.points.push_back(m_ptRectAnchor);
			m_bDrawing = true;
			return true;
	}
	return false;
}

bool CAnnotationCtl::OnLButtonDownShift(int nX, int nY) {
	if (m_eTool != ATOOL_Freehand || !m_bHasLastStrokeEnd) {
		// Nothing to anchor to, so behave like a plain click.
		return OnLButtonDown(nX, nY);
	}
	CAnnotation line;
	StartStyle(line, AT_Freehand);
	line.points.push_back(m_ptLastStrokeEnd);
	CPointF ptEnd = ToImage(nX, nY);
	line.points.push_back(ptEnd);
	m_model.Add(line); // a zero length line is dropped by the model
	m_ptLastStrokeEnd = ptEnd;
	m_pHost->InvalidateScreenRect(AnnotationGeometry::BoundingBoxOnScreen(line,
		m_pHost->GetImageOrigin(), m_pHost->GetRealizedZoom()));
	return true;
}

bool CAnnotationCtl::OnMouseMove(int nX, int nY) {
	if (!m_bDrawing) {
		return false;
	}
	if (m_pending.eType == AT_Freehand) {
		m_pending.points.push_back(ToImage(nX, nY));
		InvalidateLastSegment(); // only the piece just added is dirty
		return true;
	} else if (m_pending.eType == AT_Rectangle) {
		InvalidatePending(); // clear where the rectangle was before it is resized
		// Rebuild from the anchor rather than moving points[1], because normalising may
		// already have swapped the two corners on an earlier move.
		m_pending.points[0] = m_ptRectAnchor;
		m_pending.points[1] = ToImage(nX, nY);
		// GDI+ draws nothing for a negative width or height, so without this a drag up
		// or left would show no preview at all until the button came up.
		AnnotationGeometry::NormalizeRectangle(m_pending);
	}
	InvalidatePending(); // a rectangle is redrawn whole, so its whole area is dirty
	return true;
}

bool CAnnotationCtl::OnLButtonUp(int nX, int nY) {
	if (!m_bDrawing) {
		return false;
	}
	if (m_pending.eType == AT_Rectangle) {
		m_pending.points[0] = m_ptRectAnchor;
		m_pending.points[1] = ToImage(nX, nY);
		AnnotationGeometry::NormalizeRectangle(m_pending);
	}
	InvalidatePending();
	m_bDrawing = false;
	if (m_pending.eType == AT_Freehand && m_pending.points.size() >= 2) {
		m_ptLastStrokeEnd = m_pending.points.back();
		m_bHasLastStrokeEnd = true;
	}
	// A click without a drag leaves a one-point stroke or a zero-size rectangle;
	// CAnnotationModel::Add drops both, so neither marks the image dirty.
	m_model.Add(m_pending);
	return true;
}

void CAnnotationCtl::CommitText(LPCTSTR sText) {
	if (!m_bPendingText) {
		return;
	}
	m_bPendingText = false;
	CAnnotation annotation;
	StartStyle(annotation, AT_Text);
	annotation.points.push_back(ToImage(m_ptPendingText.x, m_ptPendingText.y));
	annotation.sText = (sText == NULL) ? _T("") : sText;
	m_model.Add(annotation); // empty text is dropped by the model
}

void CAnnotationCtl::Undo() {
	m_model.Undo();
}

void CAnnotationCtl::Redo() {
	m_model.Redo();
}

void CAnnotationCtl::Clear() {
	m_model.Clear();
	m_bHasLastStrokeEnd = false;
}
