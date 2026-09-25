#include "AnnotationCtl.h"
#include "AnnotationGeometry.h"

CAnnotationCtl::CAnnotationCtl(IAnnotationHost* pHost) {
	m_pHost = pHost;
	m_eTool = ATOOL_None;
	m_eShape = SHAPE_Rectangle;
	m_bFill = false;
	m_bFreehandArrow = false;
	m_bTextBackground = false;
	m_bDrawing = false;
	m_bPendingText = false;
	m_ptPendingText = CPoint(0, 0);
	m_ptShapeAnchor.x = m_ptShapeAnchor.y = 0.0f;
	m_ptLastStrokeEnd.x = m_ptLastStrokeEnd.y = 0.0f;
	m_bHasLastStrokeEnd = false;
	m_color = RGB(255, 0, 0);
	m_textBackColor = RGB(0, 0, 0);
	InitStyle(m_color, 180, 4, 24);
}

int CAnnotationCtl::StyleSlot() const {
	switch (m_eTool) {
		case ATOOL_Text: return SLOT_Text;
		case ATOOL_Shape: return SLOT_Shape;
		default: return SLOT_Freehand; // also where ATOOL_None looks
	}
}

void CAnnotationCtl::SetTool(EAnnotationTool eTool) {
	if (eTool == ATOOL_Shape && m_eTool == ATOOL_Shape) {
		// Pressing the shape tool again steps to the next shape. The panel framework
		// routes left clicks only, so there is no right click to spend on this.
		m_eShape = (m_eShape == SHAPE_Rectangle) ? SHAPE_Ellipse :
			(m_eShape == SHAPE_Ellipse) ? SHAPE_Triangle : SHAPE_Rectangle;
		return;
	}
	if (eTool == ATOOL_Freehand && m_eTool == ATOOL_Freehand) {
		// Likewise, pressing freehand again puts an arrow head on the end of the line.
		m_bFreehandArrow = !m_bFreehandArrow;
		return;
	}
	if (eTool == ATOOL_Text && m_eTool == ATOOL_Text) {
		// And pressing text again puts a filled backing behind the glyphs, which is what
		// makes a label readable over a busy picture.
		m_bTextBackground = !m_bTextBackground;
		return;
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

EAnnotationType CAnnotationCtl::CurrentShapeType() const {
	switch (m_eShape) {
		case SHAPE_Ellipse: return AT_Ellipse;
		case SHAPE_Triangle: return AT_Triangle;
		default: return AT_Rectangle;
	}
}

void CAnnotationCtl::InitStyle(COLORREF color, int nAlpha, int nPenWidthScreen, int nFontSizeScreen) {
	m_color = color;
	for (int i = 0; i < NUM_STYLE_SLOTS; i++) {
		m_nAlpha[i] = max(0, min(255, nAlpha));
		m_nWidthScreen[i] = (i == SLOT_Text) ? max(4, nFontSizeScreen) : max(1, nPenWidthScreen);
	}
}

void CAnnotationCtl::SetStyleForCurrentTool(COLORREF color, int nAlpha, int nWidthScreen) {
	int nSlot = StyleSlot();
	m_color = color;
	m_nAlpha[nSlot] = max(0, min(255, nAlpha));
	m_nWidthScreen[nSlot] = max((nSlot == SLOT_Text) ? 4 : 1, nWidthScreen);
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
	// The element takes the numbers of the tool drawing it, not of whichever tool the
	// style strip last showed.
	int nSlot = (eType == AT_Text) ? SLOT_Text : (IsTwoCornerShape(eType) ? SLOT_Shape : SLOT_Freehand);
	annotation.nAlpha = m_nAlpha[nSlot];
	// The user picks sizes in screen pixels; they are stored in image pixels so a stroke
	// keeps its apparent thickness while drawing and scales with the image when saved.
	annotation.fPenWidth = m_nWidthScreen[nSlot] / fZoom;
	annotation.fFontHeight = m_nWidthScreen[SLOT_Text] / fZoom;
	// Fill belongs to the shapes and the arrow head to the freehand line; neither must
	// leak onto the other kind of element.
	annotation.bFilled = m_bFill && IsTwoCornerShape(eType);
	annotation.bArrowHead = m_bFreehandArrow && eType == AT_Freehand;
	annotation.bBackground = m_bTextBackground && eType == AT_Text;
	annotation.backColor = m_textBackColor;
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
	// The arrow head sits at the end of the segment and moves with it, so its area has
	// to be part of what is repainted, both where it is now and where it just was.
	segment.bArrowHead = m_pending.bArrowHead;
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
		case ATOOL_Shape:
			StartStyle(m_pending, CurrentShapeType());
			m_ptShapeAnchor = ToImage(nX, nY);
			m_pending.points.push_back(m_ptShapeAnchor);
			m_pending.points.push_back(m_ptShapeAnchor);
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
	} else if (IsTwoCornerShape(m_pending.eType)) {
		InvalidatePending(); // clear where the shape was before it is resized
		// Rebuild from the anchor rather than moving points[1], because normalising may
		// already have swapped the two corners on an earlier move.
		m_pending.points[0] = m_ptShapeAnchor;
		m_pending.points[1] = ToImage(nX, nY);
		// GDI+ draws nothing for a negative width or height, so without this a drag up
		// or left would show no preview at all until the button came up.
		AnnotationGeometry::NormalizeShape(m_pending);
	}
	InvalidatePending(); // a shape is redrawn whole, so its whole area is dirty
	return true;
}

bool CAnnotationCtl::OnLButtonUp(int nX, int nY) {
	if (!m_bDrawing) {
		return false;
	}
	if (IsTwoCornerShape(m_pending.eType)) {
		m_pending.points[0] = m_ptShapeAnchor;
		m_pending.points[1] = ToImage(nX, nY);
		AnnotationGeometry::NormalizeShape(m_pending);
	}
	InvalidatePending();
	m_bDrawing = false;
	if (m_pending.eType == AT_Freehand && m_pending.points.size() >= 2) {
		m_ptLastStrokeEnd = m_pending.points.back();
		m_bHasLastStrokeEnd = true;
	}
	// A click without a drag leaves a one-point stroke or a zero-size shape;
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
