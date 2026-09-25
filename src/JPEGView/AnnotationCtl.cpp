#include "AnnotationCtl.h"
#include "AnnotationGeometry.h"

// RED STUB - compiles, does nothing. Replaced in the GREEN pass.

CAnnotationCtl::CAnnotationCtl(IAnnotationHost* pHost) {
	m_pHost = pHost;
	m_eTool = ATOOL_None;
	m_bRectangleFilled = false;
	m_bDrawing = false;
	m_bPendingText = false;
	m_ptPendingText = CPoint(0, 0);
	m_color = RGB(255, 0, 0);
	m_nAlpha = 180;
	m_nPenWidthScreen = 4;
	m_nFontSizeScreen = 24;
}

void CAnnotationCtl::SetTool(EAnnotationTool eTool) {
}

void CAnnotationCtl::SetStyle(COLORREF color, int nAlpha, int nPenWidthScreen, int nFontSizeScreen) {
}

CPointF CAnnotationCtl::ToImage(int nX, int nY) {
	CPointF pt = { 0.0f, 0.0f };
	return pt;
}

void CAnnotationCtl::StartStyle(CAnnotation& annotation, EAnnotationType eType) {
}

void CAnnotationCtl::InvalidatePending() {
}

bool CAnnotationCtl::OnLButtonDown(int nX, int nY) {
	return false;
}

bool CAnnotationCtl::OnMouseMove(int nX, int nY) {
	return false;
}

bool CAnnotationCtl::OnLButtonUp(int nX, int nY) {
	return false;
}

void CAnnotationCtl::CommitText(LPCTSTR sText) {
}

void CAnnotationCtl::Undo() {
}

void CAnnotationCtl::Redo() {
}

void CAnnotationCtl::Clear() {
}
