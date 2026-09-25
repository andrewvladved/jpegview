#include "StdAfx.h"
#include "AnnotationStylePanelCtl.h"
#include "AnnotationStylePanel.h"
#include "AnnotationCtl.h"
#include "MainDlg.h"
#include "SettingsProvider.h"
#include "NLS.h"

CAnnotationStylePanelCtl::CAnnotationStylePanelCtl(CMainDlg* pMainDlg, CPanel* pNavPanel)
		: CPanelController(pMainDlg, false) {
	CSettingsProvider& sp = CSettingsProvider::This();
	m_bVisible = false;
	m_color = sp.AnnotationColor();
	m_nPenWidth = sp.AnnotationPenWidth();
	m_nFontSize = sp.AnnotationFontSize();
	m_dOpacity = sp.AnnotationOpacity();
	m_dWidth = m_nPenWidth;

	m_pPanel = m_pStylePanel = new CAnnotationStylePanel(pMainDlg->GetHWND(), this, pNavPanel,
		pMainDlg->GetAnnotationCtl(), &m_dOpacity, &m_dWidth);

	for (int i = 0; i < CAnnotationStylePanel::NUM_SWATCHES; i++) {
		CButtonCtrl* pBtn = m_pStylePanel->GetSwatch(i);
		if (pBtn != NULL) {
			pBtn->SetButtonPressedHandler(&OnSwatchPressed, this, i);
		}
	}
	CButtonCtrl* pOther = m_pStylePanel->GetBtnOtherColor();
	if (pOther != NULL) {
		pOther->SetButtonPressedHandler(&OnOtherColorPressed, this);
	}
}

CAnnotationStylePanelCtl::~CAnnotationStylePanelCtl() {
	// m_pPanel is deleted by CPanelMgr, which owns every panel.
}

void CAnnotationStylePanelCtl::SetVisible(bool bVisible) {
	if (m_bVisible == bVisible) {
		return;
	}
	m_bVisible = bVisible;
	if (bVisible) {
		LoadFromControl();
		RelabelWidthSlider();
		m_pStylePanel->RequestRepositioning();
	}
	InvalidateMainDlg();
}

// The width slider drives the font size while the text tool is active and the line width
// otherwise, so the two values are kept separately and swapped in when the strip opens.
void CAnnotationStylePanelCtl::LoadFromControl() {
	CAnnotationCtl* pCtl = m_pMainDlg->GetAnnotationCtl();
	if (pCtl == NULL) {
		return;
	}
	m_color = pCtl->GetColor();
	m_dOpacity = pCtl->GetAlpha() * 100.0 / 255.0;
	m_nPenWidth = pCtl->GetPenWidthScreen();
	m_nFontSize = pCtl->GetFontSizeScreen();
	m_dWidth = (pCtl->GetTool() == ATOOL_Text) ? m_nFontSize : m_nPenWidth;
}

void CAnnotationStylePanelCtl::RelabelWidthSlider() {
	CSliderDouble* pWidth = m_pStylePanel->GetSliderWidth();
	CAnnotationCtl* pCtl = m_pMainDlg->GetAnnotationCtl();
	if (pWidth == NULL || pCtl == NULL) {
		return;
	}
	pWidth->SetName((pCtl->GetTool() == ATOOL_Text)
		? CNLS::GetString(_T("Font size")) : CNLS::GetString(_T("Line width")));
}

void CAnnotationStylePanelCtl::ApplyStyle() {
	CAnnotationCtl* pCtl = m_pMainDlg->GetAnnotationCtl();
	if (pCtl == NULL) {
		return;
	}
	if (pCtl->GetTool() == ATOOL_Text) {
		m_nFontSize = max(4, min(400, (int)(m_dWidth + 0.5)));
	} else {
		m_nPenWidth = max(1, min(100, (int)(m_dWidth + 0.5)));
	}
	int nOpacityPercent = max(0, min(100, (int)(m_dOpacity + 0.5)));
	pCtl->SetStyle(m_color, nOpacityPercent * 255 / 100, m_nPenWidth, m_nFontSize);
	CSettingsProvider::This().SaveAnnotationStyle(m_color, nOpacityPercent, m_nPenWidth, m_nFontSize);
	InvalidateMainDlg();
}

void CAnnotationStylePanelCtl::OnSwatchPressed(void* pContext, int nParameter) {
	CAnnotationStylePanelCtl* pThis = (CAnnotationStylePanelCtl*)pContext;
	pThis->m_color = CAnnotationStylePanel::SwatchColor(nParameter);
	pThis->ApplyStyle();
}

void CAnnotationStylePanelCtl::OnOtherColorPressed(void* pContext, int nParameter) {
	CAnnotationStylePanelCtl* pThis = (CAnnotationStylePanelCtl*)pContext;
	// Custom colours persist for the session, the way the system dialog expects.
	static COLORREF customColors[16] = {
		RGB(255,255,255), RGB(255,255,255), RGB(255,255,255), RGB(255,255,255),
		RGB(255,255,255), RGB(255,255,255), RGB(255,255,255), RGB(255,255,255),
		RGB(255,255,255), RGB(255,255,255), RGB(255,255,255), RGB(255,255,255),
		RGB(255,255,255), RGB(255,255,255), RGB(255,255,255), RGB(255,255,255)
	};
	CHOOSECOLOR cc;
	memset(&cc, 0, sizeof(cc));
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = pThis->m_pMainDlg->GetHWND();
	cc.lpCustColors = customColors;
	cc.rgbResult = pThis->m_color;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;
	if (::ChooseColor(&cc)) {
		pThis->m_color = cc.rgbResult;
		pThis->ApplyStyle();
	}
}

bool CAnnotationStylePanelCtl::OnMouseLButton(EMouseEvent eMouseEvent, int nX, int nY) {
	bool bConsumed = CPanelController::OnMouseLButton(eMouseEvent, nX, nY);
	if (bConsumed && eMouseEvent == MouseEvent_BtnUp) {
		ApplyStyle(); // a slider was released, or a swatch clicked
	}
	return bConsumed;
}

bool CAnnotationStylePanelCtl::OnMouseMove(int nX, int nY) {
	bool bConsumed = CPanelController::OnMouseMove(nX, nY);
	if (bConsumed && m_pStylePanel->MouseCursorCaptured()) {
		ApplyStyle(); // dragging a slider, so the change is visible while dragging
	}
	return bConsumed;
}
