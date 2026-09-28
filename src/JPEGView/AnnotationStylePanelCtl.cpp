#include "StdAfx.h"
#include "resource.h"
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
	m_dOpacity = sp.AnnotationOpacity();
	m_dWidth = sp.AnnotationPenWidth();

	m_pPanel = m_pStylePanel = new CAnnotationStylePanel(pMainDlg->GetHWND(), this, pNavPanel,
		pMainDlg->GetAnnotationCtl(), &m_dOpacity, &m_dWidth);

	for (int i = 0; i < CAnnotationStylePanel::NUM_SWATCHES; i++) {
		CButtonCtrl* pBtn = m_pStylePanel->GetSwatch(i);
		if (pBtn != NULL) {
			pBtn->SetButtonPressedHandler(&OnSwatchPressed, this, i);
		}
	}
	CSliderDouble* pOpacity = m_pStylePanel->GetSliderOpacity();
	if (pOpacity != NULL) {
		pOpacity->SetIntegerValue(true);
		pOpacity->SetDirectValueEntry(true);
	}
	CSliderDouble* pWidth = m_pStylePanel->GetSliderWidth();
	if (pWidth != NULL) {
		pWidth->SetIntegerValue(true);
		pWidth->SetDirectValueEntry(true);
	}
	CButtonCtrl* pOther = m_pStylePanel->GetBtnOtherColor();
	if (pOther != NULL) {
		pOther->SetButtonPressedHandler(&OnOtherColorPressed, this);
	}
	CButtonCtrl* pBackColor = m_pStylePanel->GetBtnBackColor();
	if (pBackColor != NULL) {
		pBackColor->SetButtonPressedHandler(&OnBackColorPressed, this);
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
		UpdateBackColorButton();
		m_pStylePanel->RequestRepositioning();
	}
	InvalidateMainDlg();
}

// The strip stays open while the user switches tools from the panel, the menu or a
// shortcut, so it has to follow along instead of showing the numbers of the tool it
// happened to be opened with - which is how a thin line for the shapes used to end up
// as the font size.
void CAnnotationStylePanelCtl::OnToolChanged() {
	if (!m_bVisible) {
		return;
	}
	LoadFromControl();
	RelabelWidthSlider();
	UpdateBackColorButton();
	m_pStylePanel->RequestRepositioning();
	InvalidateMainDlg();
}

// The opacity and the width belong to the selected tool; the annotation controller keeps
// one pair per tool and hands out the pair of whichever tool is active.
void CAnnotationStylePanelCtl::LoadFromControl() {
	CAnnotationCtl* pCtl = m_pMainDlg->GetAnnotationCtl();
	if (pCtl == NULL) {
		return;
	}
	m_color = pCtl->GetColor();
	m_dOpacity = pCtl->GetAlpha() * 100.0 / 255.0;
	m_dWidth = pCtl->GetWidthScreen();
}

void CAnnotationStylePanelCtl::UpdateBackColorButton() {
	CButtonCtrl* pBackColor = m_pStylePanel->GetBtnBackColor();
	CAnnotationCtl* pCtl = m_pMainDlg->GetAnnotationCtl();
	if (pBackColor == NULL || pCtl == NULL) {
		return;
	}
	pBackColor->SetShow(pCtl->GetTool() == ATOOL_Text && pCtl->IsTextBackground(), false);
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

void CAnnotationStylePanelCtl::ApplyStyle(bool bPersist) {
	CAnnotationCtl* pCtl = m_pMainDlg->GetAnnotationCtl();
	if (pCtl == NULL) {
		return;
	}
	bool bText = pCtl->GetTool() == ATOOL_Text;
	int nWidth = max(bText ? 4 : 1, min(bText ? 400 : 100, (int)(m_dWidth + 0.5)));
	int nOpacityPercent = max(0, min(100, (int)(m_dOpacity + 0.5)));
	pCtl->SetStyleForCurrentTool(m_color, nOpacityPercent * 255 / 100, nWidth);
	if (bPersist) {
		// Writing the INI on every mouse-move would be hundreds of file writes per drag.
		// The file holds one starting value per kind, which every tool picks up at the
		// next start; the per-tool split only lives for the session.
		CSettingsProvider::This().SaveAnnotationStyle(m_color, nOpacityPercent,
			pCtl->GetPenWidthScreen(), pCtl->GetFontSizeScreen(), pCtl->GetTextBackColor());
	}
	InvalidateMainDlg();
}

void CAnnotationStylePanelCtl::OnSwatchPressed(void* pContext, int nParameter, CButtonCtrl& sender) {
	CAnnotationStylePanelCtl* pThis = (CAnnotationStylePanelCtl*)pContext;
	pThis->m_color = CAnnotationStylePanel::SwatchColor(nParameter);
	pThis->ApplyStyle(true);
}

void CAnnotationStylePanelCtl::OnOtherColorPressed(void* pContext, int nParameter, CButtonCtrl& sender) {
	CAnnotationStylePanelCtl* pThis = (CAnnotationStylePanelCtl*)pContext;
	COLORREF color = pThis->m_color;
	if (PickColor(pThis->m_pMainDlg, color)) {
		pThis->m_color = color;
		pThis->ApplyStyle(true);
	}
}

void CAnnotationStylePanelCtl::OnBackColorPressed(void* pContext, int nParameter, CButtonCtrl& sender) {
	CAnnotationStylePanelCtl* pThis = (CAnnotationStylePanelCtl*)pContext;
	CAnnotationCtl* pCtl = pThis->m_pMainDlg->GetAnnotationCtl();
	if (pCtl == NULL) {
		return;
	}
	COLORREF color = pCtl->GetTextBackColor();
	if (PickColor(pThis->m_pMainDlg, color)) {
		pCtl->SetTextBackColor(color);
		pThis->ApplyStyle(true);
	}
}

bool CAnnotationStylePanelCtl::PickColor(CMainDlg* pMainDlg, COLORREF& color) {
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
	cc.hwndOwner = pMainDlg->GetHWND();
	cc.lpCustColors = customColors;
	cc.rgbResult = color;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;
	if (!::ChooseColor(&cc)) {
		return false;
	}
	color = cc.rgbResult;
	return true;
}

bool CAnnotationStylePanelCtl::OnMouseLButton(EMouseEvent eMouseEvent, int nX, int nY) {
	bool bConsumed = CPanelController::OnMouseLButton(eMouseEvent, nX, nY);
	if (bConsumed && eMouseEvent == MouseEvent_BtnUp) {
		ApplyStyle(true); // a slider was released, or a swatch clicked
		CheckForValueEntry();
	}
	return bConsumed;
}

bool CAnnotationStylePanelCtl::OnMouseMove(int nX, int nY) {
	bool bConsumed = CPanelController::OnMouseMove(nX, nY);
	if (bConsumed && m_pStylePanel->MouseCursorCaptured()) {
		ApplyStyle(false); // live feedback while dragging, without touching disk
	}
	return bConsumed;
}

// A click on the number of either slider asks for an edit box over that number.
void CAnnotationStylePanelCtl::CheckForValueEntry() {
	CSliderDouble* pOpacity = m_pStylePanel->GetSliderOpacity();
	CSliderDouble* pWidth = m_pStylePanel->GetSliderWidth();
	if (pOpacity != NULL && pOpacity->TakeValueEntryRequest()) {
		m_pMainDlg->StartAnnotationValueEdit(ENTRY_OPACITY, pOpacity->GetNumberRect(),
			(int)(m_dOpacity + 0.5), (int)pOpacity->GetMin(), (int)pOpacity->GetMax());
		return;
	}
	if (pWidth != NULL && pWidth->TakeValueEntryRequest()) {
		m_pMainDlg->StartAnnotationValueEdit(ENTRY_WIDTH, pWidth->GetNumberRect(),
			(int)(m_dWidth + 0.5), (int)pWidth->GetMin(), (int)pWidth->GetMax());
	}
}

void CAnnotationStylePanelCtl::SetValueFromEntry(int nWhich, int nValue) {
	if (nWhich == ENTRY_OPACITY) {
		m_dOpacity = nValue;
	} else if (nWhich == ENTRY_WIDTH) {
		m_dWidth = nValue;
	} else {
		return;
	}
	ApplyStyle(true);
}
