#include "StdAfx.h"
#include "PanelMgr.h"
#include "PaintMemDCMgr.h"

CPanelMgr::CPanelMgr() {
	m_pCapturedPanelController = NULL;
	m_bHideNonModal = false;
}

CPanelMgr::~CPanelMgr() {
	std::list<CPanelController*>::iterator iter;
	for (iter = m_panelControllers.begin( ); iter != m_panelControllers.end( ); iter++ ) {
		delete (*iter);
	}
}

bool CPanelMgr::IsModalPanelShown() const {
	std::list<CPanelController*>::const_iterator iter;
	for (iter = m_panelControllers.begin( ); iter != m_panelControllers.end( ); iter++ ) {
		if ((*iter)->IsModal() && IsShown(*iter)) {
			return true;
		}
	}
	return false;
}

void CPanelMgr::CancelModalPanel() {
	std::list<CPanelController*>::const_iterator iter;
	for (iter = m_panelControllers.begin( ); iter != m_panelControllers.end( ); iter++ ) {
		if ((*iter)->IsModal() && IsShown(*iter)) {
			(*iter)->CancelModalPanel();
		}
	}
}

void CPanelMgr::PrepareMemDCMgr(CPaintMemDCMgr& memDCMgr, std::list<CRect>& listExcludedRects) {
	std::list<CPanelController*>::iterator iter;
	for (iter = m_panelControllers.begin( ); iter != m_panelControllers.end( ); iter++ ) {
		if (IsShown(*iter)) {
			CRect rectPanel = (*iter)->PanelRect();
			if (rectPanel.IntersectRect(rectPanel, &(memDCMgr.GetPaintDC().m_ps.rcPaint))) {
				listExcludedRects.push_back(memDCMgr.CreatePanelRegion((*iter)->GetPanel(), (*iter)->DimFactor(), (*iter)->BlendPanel()));
			}
		}
	}
}

void CPanelMgr::ExcludeVisiblePanels(CDC& dc) {
	std::list<CPanelController*>::iterator iter;
	for (iter = m_panelControllers.begin( ); iter != m_panelControllers.end( ); iter++ ) {
		if (IsShown(*iter)) {
			CRect rectPanel = (*iter)->PanelRect();
			dc.ExcludeClipRect(&rectPanel);
		}
	}
}

void CPanelMgr::AddPanelController(CPanelController* pPanelController) {
	m_panelControllers.push_back(pPanelController);
}

void CPanelMgr::AfterNewImageLoaded() {
	std::list<CPanelController*>::iterator iter;
	for (iter = m_panelControllers.begin( ); iter != m_panelControllers.end( ); iter++ ) {
		(*iter)->AfterNewImageLoaded();
	}
}

void CPanelMgr::AfterImageRenamed() {
	std::list<CPanelController*>::iterator iter;
	for (iter = m_panelControllers.begin( ); iter != m_panelControllers.end( ); iter++ ) {
		(*iter)->AfterImageRenamed();
	}
}

bool CPanelMgr::OnMouseLButton(EMouseEvent eMouseEvent, int nX, int nY) {
	if (m_pCapturedPanelController != NULL && IsShown(m_pCapturedPanelController)) {
		if (m_pCapturedPanelController->OnMouseLButton(eMouseEvent, nX, nY)) {
			return true;
		}
	}
	std::list<CPanelController*>::iterator iter;
	for (iter = m_panelControllers.begin( ); iter != m_panelControllers.end( ); iter++ ) {
		if (IsShown(*iter) && *iter != m_pCapturedPanelController) { // no mouse clicks for invisible panels
			if ((*iter)->OnMouseLButton(eMouseEvent, nX, nY)) {
				return true;
			}
		}
	}
	return false;
}

bool CPanelMgr::OnMouseMove(int nX, int nY) {
	if (m_pCapturedPanelController != NULL && IsLive(m_pCapturedPanelController)) {
		if (m_pCapturedPanelController->OnMouseMove(nX, nY)) {
			return true;
		}
	}
	std::list<CPanelController*>::iterator iter;
	for (iter = m_panelControllers.begin( ); iter != m_panelControllers.end( ); iter++ ) {
		if (IsLive(*iter) && *iter != m_pCapturedPanelController) { // must be routed to invisible but active panels, may get visible by mouse movements
			if ((*iter)->OnMouseMove(nX, nY)) {
				return true;
			}
		}
	}
	return false;
}

bool CPanelMgr::MouseCursorCaptured() {
	if (m_pCapturedPanelController != NULL && IsLive(m_pCapturedPanelController)) {
		if (m_pCapturedPanelController->MouseCursorCaptured()) {
			return true;
		}
	}
	std::list<CPanelController*>::iterator iter;
	for (iter = m_panelControllers.begin(); iter != m_panelControllers.end(); iter++) {
		if (IsLive(*iter) && *iter != m_pCapturedPanelController) { // must be routed to invisible but active panels, may get visible by mouse movements
			if ((*iter)->MouseCursorCaptured()) {
				return true;
			}
		}
	}
	return false;
}

bool CPanelMgr::OnKeyDown(unsigned int nVirtualKey, bool bShift, bool bAlt, bool bCtrl) {
	std::list<CPanelController*>::iterator iter;
	for (iter = m_panelControllers.begin( ); iter != m_panelControllers.end( ); iter++ ) {
		if (IsShown(*iter) && (*iter)->IsModal()) { // no keys for invisible panels
			if ((*iter)->OnKeyDown(nVirtualKey, bShift, bAlt, bCtrl)) {
				return true;
			}
		}
	}
	return false;
}

bool CPanelMgr::OnTimer(int nTimerId) {
	std::list<CPanelController*>::iterator iter;
	for (iter = m_panelControllers.begin( ); iter != m_panelControllers.end( ); iter++ ) {
		if (IsLive(*iter)) {
			if ((*iter)->OnTimer(nTimerId)) {
				return true;
			}
		}
	}
	return false;
}

void CPanelMgr::PaintPanels(CDC & dc, const CPoint& offset) {
	std::list<CPanelController*>::iterator iter;
	for (iter = m_panelControllers.begin( ); iter != m_panelControllers.end( ); iter++ ) {
		if (IsShown(*iter)) {
			(*iter)->OnPaintPanel(dc, offset);
		}
	}
}

void CPanelMgr::OnPrePaint(HDC hPaintDC) {
	std::list<CPanelController*>::iterator iter;
	for (iter = m_panelControllers.begin( ); iter != m_panelControllers.end( ); iter++ ) {
		if (IsShown(*iter)) {
			(*iter)->OnPrePaintMainDlg(hPaintDC);
		}
	}
}

void CPanelMgr::OnPostPaint(HDC hPaintDC) {
	std::list<CPanelController*>::iterator iter;
	for (iter = m_panelControllers.begin( ); iter != m_panelControllers.end( ); iter++ ) {
		if (IsShown(*iter)) {
			(*iter)->OnPostPaintMainDlg(hPaintDC);
		}
	}
}