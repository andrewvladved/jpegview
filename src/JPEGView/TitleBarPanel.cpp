#include "StdAfx.h"
#include "TitleBarPanel.h"
#include "WndButtonPanel.h"
#include "Helpers.h"
#include "SettingsProvider.h"
#include "NLS.h"

#define TITLEBAR_PANEL_HEIGHT 24
#define TITLEBAR_BORDER 1
#define TITLEBAR_TEXT_MARGIN 6

/////////////////////////////////////////////////////////////////////////////////////////////
// CTitleBarPanel
/////////////////////////////////////////////////////////////////////////////////////////////

CTitleBarPanel::CTitleBarPanel(HWND hWnd, INotifiyMouseCapture* pNotifyMouseCapture)
	: CPanel(hWnd, pNotifyMouseCapture, false, true) { // not framed, clicks not consumed by the panel itself
	m_clientRect = CRect(0, 0, 0, 0);
	m_fDPIScale *= CSettingsProvider::This().ScaleFactorNavPanel();
	m_nButtonAreaWidth = 0;
	m_nHeight = (int)(TITLEBAR_PANEL_HEIGHT*m_fDPIScale);

	AddText(ID_txtFilePath, _T(""), false);

	CButtonCtrl* pMinimizeBtn = AddUserPaintButton(ID_btnMinimize, CNLS::GetString(_T("Minimize")), &PaintMinimizeBtn, NULL, this);
	pMinimizeBtn->SetExtendedActiveArea(CRect(0, -2, 0, 0));
	CButtonCtrl* pMaximizeBtn = AddUserPaintButton(ID_btnMaximize, CNLS::GetString(_T("Maximize/Restore")), &PaintMaximizeBtn, NULL, this);
	pMaximizeBtn->SetExtendedActiveArea(CRect(0, -2, 0, 0));
	CButtonCtrl* pCloseBtn = AddUserPaintButton(ID_btnClose, CNLS::GetString(_T("Close")), &PaintCloseBtn, NULL, this);
	pCloseBtn->SetExtendedActiveArea(CRect(0, -2, 2, 0)); // extend to the right screen border
}

int CTitleBarPanel::NumberOfButtons() {
	return (int)m_controls.size() - 1; // all controls except the file path text
}

CRect CTitleBarPanel::PanelRect() {
	::GetClientRect(m_hWnd, &m_clientRect);
	return CRect(CPoint(m_clientRect.left, m_clientRect.top), CSize(m_clientRect.Width(), m_nHeight));
}

CRect CTitleBarPanel::ButtonAreaRect() {
	CRect panelRect = PanelRect();
	if (m_nButtonAreaWidth == 0) {
		int nButtonSize = m_nHeight - 4 * TITLEBAR_BORDER;
		int nNumButtons = NumberOfButtons();
		m_nButtonAreaWidth = 4 * TITLEBAR_BORDER + (nNumButtons - 1) * TITLEBAR_BORDER * 3 + nNumButtons * nButtonSize;
	}
	return CRect(CPoint(panelRect.right - m_nButtonAreaWidth, panelRect.top), CSize(m_nButtonAreaWidth, m_nHeight));
}

void CTitleBarPanel::SetFilePath(LPCTSTR sFilePath) {
	GetTextFilePath()->SetText((sFilePath == NULL) ? _T("") : sFilePath);
}

void CTitleBarPanel::RequestRepositioning() {
	RepositionAll();
}

void CTitleBarPanel::RepositionAll() {
	CRect panelRect = PanelRect();
	CRect buttonAreaRect = ButtonAreaRect();

	int nButtonSize = m_nHeight - 4 * TITLEBAR_BORDER;
	int nStartX = buttonAreaRect.left + TITLEBAR_BORDER * 2;
	int nStartY = TITLEBAR_BORDER * 2;
	ControlsIterator iter;
	for (iter = m_controls.begin( ); iter != m_controls.end( ); iter++ ) {
		CButtonCtrl* pButton = dynamic_cast<CButtonCtrl*>(iter->second);
		if (pButton != NULL) {
			pButton->SetPosition(CRect(CPoint(nStartX, nStartY), CSize(nButtonSize, nButtonSize)));
			nStartX += nButtonSize + TITLEBAR_BORDER * 3;
		}
	}

	// the file path text gets all the space left of the buttons
	int nTextMargin = (int)(TITLEBAR_TEXT_MARGIN*m_fDPIScale);
	int nTextWidth = max(0, buttonAreaRect.left - panelRect.left - 2 * nTextMargin);
	GetTextFilePath()->SetPosition(CRect(CPoint(panelRect.left + nTextMargin, nStartY), CSize(nTextWidth, nButtonSize)));
}

void CTitleBarPanel::PaintMinimizeBtn(void* pContext, const CRect& rect, CDC& dc) {
	CWndButtonPanel::PaintMinimizeBtn(pContext, rect, dc);
}

void CTitleBarPanel::PaintMaximizeBtn(void* pContext, const CRect& rect, CDC& dc) {
	CTitleBarPanel* pPanel = (CTitleBarPanel*)pContext;
	if (pPanel != NULL && ::IsZoomed(pPanel->GetHWND())) {
		// window is maximized - show the 'restore down' symbol
		CWndButtonPanel::PaintRestoreBtn(pContext, rect, dc);
	} else {
		// window is not maximized - show a plain rectangle as 'maximize' symbol
		CRect r = Helpers::InflateRect(rect, 0.25f);
		dc.Rectangle(r.left, r.top, r.right, r.bottom);
		CPoint p1(r.left + 1, r.top + 1);
		dc.MoveTo(p1);
		CPoint p2(r.right - 1, r.top + 1);
		dc.LineTo(p2);
	}
}

void CTitleBarPanel::PaintCloseBtn(void* pContext, const CRect& rect, CDC& dc) {
	CWndButtonPanel::PaintCloseBtn(pContext, rect, dc);
}
