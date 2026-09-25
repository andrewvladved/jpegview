#include "StdAfx.h"
#include "resource.h"
#include "MainDlg.h"
#include "TitleBarPanelCtl.h"
#include "TitleBarPanel.h"

CTitleBarPanelCtl::CTitleBarPanelCtl(CMainDlg* pMainDlg) : CPanelController(pMainDlg, false) {
	m_pPanel = m_pTitleBarPanel = new CTitleBarPanel(pMainDlg->GetHWND(), this);
	m_pTitleBarPanel->GetBtnMinimize()->SetButtonPressedHandler(&(CMainDlg::OnExecuteCommand), pMainDlg, IDM_MINIMIZE);
	m_pTitleBarPanel->GetBtnMaximize()->SetButtonPressedHandler(&(CMainDlg::OnExecuteCommand), pMainDlg, IDM_MAXIMIZE_RESTORE);
	m_pTitleBarPanel->GetBtnClose()->SetButtonPressedHandler(&(CMainDlg::OnExecuteCommand), pMainDlg, IDM_EXIT);
}

CTitleBarPanelCtl::~CTitleBarPanelCtl() {
	delete m_pTitleBarPanel;
	m_pTitleBarPanel = NULL;
}

bool CTitleBarPanelCtl::IsVisible() {
	// the title bar replaces the window caption, thus it makes no sense in full screen mode
	return m_pMainDlg->IsTransparentTitleBar() && !m_pMainDlg->IsFullScreenMode();
}

void CTitleBarPanelCtl::AfterNewImageLoaded() {
	UpdateFilePath();
}

void CTitleBarPanelCtl::AfterImageRenamed() {
	UpdateFilePath();
}

void CTitleBarPanelCtl::UpdateFilePath() {
	m_pTitleBarPanel->SetFilePath(m_pMainDlg->CurrentFileName(false));
	if (IsVisible()) {
		m_pMainDlg->InvalidateRect(m_pTitleBarPanel->PanelRect(), FALSE);
	}
}

int CTitleBarPanelCtl::TitleBarHeight() {
	return m_pTitleBarPanel->TitleBarHeight();
}

bool CTitleBarPanelCtl::IsPointInDragArea(CPoint pt) {
	if (!IsVisible()) {
		return false;
	}
	return m_pTitleBarPanel->PanelRect().PtInRect(pt) && !m_pTitleBarPanel->ButtonAreaRect().PtInRect(pt);
}

bool CTitleBarPanelCtl::IsPointInButtonArea(CPoint pt) {
	if (!IsVisible()) {
		return false;
	}
	return m_pTitleBarPanel->ButtonAreaRect().PtInRect(pt);
}
