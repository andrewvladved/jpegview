#include "StdAfx.h"
#include "AnnotationStylePanel.h"
#include "AnnotationCtl.h"
#include "Helpers.h"
#include "SettingsProvider.h"
#include "NLS.h"

#define STYLE_PANEL_HEIGHT 32
#define STYLE_PANEL_BORDER 6
#define STYLE_PANEL_GAP 6
#define STYLE_PANEL_SWATCH 18
#define STYLE_PANEL_SLIDER 110

static const COLORREF s_swatches[CAnnotationStylePanel::NUM_SWATCHES] = {
	RGB(255, 0, 0),     // red
	RGB(255, 220, 0),   // yellow
	RGB(0, 200, 0),     // green
	RGB(0, 200, 255),   // cyan
	RGB(0, 80, 255),    // blue
	RGB(255, 0, 200),   // magenta
	RGB(255, 255, 255), // white
	RGB(0, 0, 0)        // black
};

// Stable addresses to hand to the swatch paint handlers as their paint context.
static const int s_swatchIndex[CAnnotationStylePanel::NUM_SWATCHES] = { 0, 1, 2, 3, 4, 5, 6, 7 };

COLORREF CAnnotationStylePanel::SwatchColor(int nIndex) {
	if (nIndex < 0 || nIndex >= NUM_SWATCHES) {
		return RGB(255, 0, 0);
	}
	return s_swatches[nIndex];
}

CAnnotationStylePanel::CAnnotationStylePanel(HWND hWnd, INotifiyMouseCapture* pNotifyMouseCapture,
		CPanel* pNavPanel, CAnnotationCtl* pAnnotationCtl, double* pdOpacity, double* pdWidth)
	: CPanel(hWnd, pNotifyMouseCapture, true, false) { // framed, and it swallows clicks
	m_pNavPanel = pNavPanel;
	m_pAnnotationCtl = pAnnotationCtl;
	m_clientRect = CRect(0, 0, 0, 0);
	m_fDPIScale *= CSettingsProvider::This().ScaleFactorNavPanel();
	m_nHeight = (int)(STYLE_PANEL_HEIGHT * m_fDPIScale);
	m_nBorder = (int)(STYLE_PANEL_BORDER * m_fDPIScale);
	m_nGap = (int)(STYLE_PANEL_GAP * m_fDPIScale);
	m_nSwatchSize = (int)(STYLE_PANEL_SWATCH * m_fDPIScale);
	m_nWidth = 0;

	for (int i = 0; i < NUM_SWATCHES; i++) {
		// A paint handler is given only its paint context, never the button's parameter,
		// so each swatch is painted with its own index as that context.
		AddUserPaintButton(ID_swatch0 + i, _T(""), &PaintSwatch, NULL, (void*)&s_swatchIndex[i], NULL, i);
	}
	AddButton(ID_btnOtherColor, CNLS::GetString(_T("Other colour...")));
	// Only meaningful for a text label with a filled backing, so the controller shows and
	// hides it with that mode. A hidden control is skipped by painting and hit testing.
	AddButton(ID_btnBackColor, CNLS::GetString(_T("Back colour...")));
	CButtonCtrl* pBackColor = GetBtnBackColor();
	if (pBackColor != NULL) {
		pBackColor->SetShow(false, false);
	}
	// The width slider is relabelled by the controller when the text tool is active,
	// because there it sets the font size instead of the line width.
	AddSlider(ID_slOpacity, CNLS::GetString(_T("Opacity")), pdOpacity, NULL,
		0.0, 100.0, 70.0, false, false, false, (int)(STYLE_PANEL_SLIDER * m_fDPIScale));
	AddSlider(ID_slWidth, CNLS::GetString(_T("Line width")), pdWidth, NULL,
		1.0, 60.0, 4.0, false, false, false, (int)(STYLE_PANEL_SLIDER * m_fDPIScale));
}

void CAnnotationStylePanel::PaintSwatch(void* pContext, const CRect& rect, CDC& dc) {
	if (pContext == NULL) {
		return;
	}
	int nIndex = *(const int*)pContext;
	CRect r = Helpers::InflateRect(rect, 0.12f);
	CBrush brush;
	brush.CreateSolidBrush(SwatchColor(nIndex));
	HBRUSH hOldBrush = dc.SelectBrush(brush);
	dc.Rectangle(r.left, r.top, r.right, r.bottom);
	dc.SelectBrush(hOldBrush);
}

CRect CAnnotationStylePanel::PanelRect() {
	::GetClientRect(m_hWnd, &m_clientRect);
	if (m_nWidth == 0) {
		// Width is the sum of what the controls need; computed once they all exist.
		int nSwatches = NUM_SWATCHES * m_nSwatchSize + (NUM_SWATCHES - 1) * (m_nGap / 2);
		int nOther = 0;
		CButtonCtrl* pOther = GetBtnOtherColor();
		if (pOther != NULL) {
			nOther = pOther->GetMinSize().cx;
		}
		CButtonCtrl* pBackColor = GetBtnBackColor();
		if (pBackColor != NULL && pBackColor->IsShown()) {
			nOther += m_nGap + pBackColor->GetMinSize().cx;
		}
		int nSliders = 0;
		CSliderDouble* pOpacity = GetSliderOpacity();
		CSliderDouble* pWidth = GetSliderWidth();
		if (pOpacity != NULL) nSliders += pOpacity->GetMinSize().cx;
		if (pWidth != NULL) nSliders += pWidth->GetMinSize().cx;
		if (pOther != NULL && pOpacity != NULL && pWidth != NULL) {
			m_nWidth = 2 * m_nBorder + nSwatches + m_nGap + nOther + m_nGap + nSliders + m_nGap;
		} else {
			return CRect(0, 0, 0, 0); // still being filled with controls
		}
	}
	// Centred horizontally, sitting directly above the navigation panel.
	CRect navRect = (m_pNavPanel == NULL) ? CRect(0, m_clientRect.bottom, 0, m_clientRect.bottom)
		: m_pNavPanel->PanelRect();
	int nLeft = (m_clientRect.Width() - m_nWidth) / 2;
	int nTop = navRect.top - m_nHeight - m_nGap;
	return CRect(CPoint(nLeft, nTop), CSize(m_nWidth, m_nHeight));
}

void CAnnotationStylePanel::RequestRepositioning() {
	m_nWidth = 0;
	RepositionAll();
}

void CAnnotationStylePanel::RepositionAll() {
	CRect panelRect = PanelRect();
	if (panelRect.Width() == 0) {
		return; // controls are not all present yet
	}
	int nX = panelRect.left + m_nBorder;
	int nYCenter = panelRect.top + m_nHeight / 2;

	for (int i = 0; i < NUM_SWATCHES; i++) {
		CButtonCtrl* pBtn = GetSwatch(i);
		// CPanel::AddUserPaintButton inserts into m_controls only after the control's
		// constructor has run, and that constructor already asks for repositioning, so a
		// control can still be missing here.
		if (pBtn == NULL) {
			return;
		}
		pBtn->SetPosition(CRect(CPoint(nX, nYCenter - m_nSwatchSize / 2),
			CSize(m_nSwatchSize, m_nSwatchSize)));
		nX += m_nSwatchSize + m_nGap / 2;
	}
	nX += m_nGap / 2;

	CButtonCtrl* pOther = GetBtnOtherColor();
	if (pOther == NULL) {
		return;
	}
	CSize sizeOther = pOther->GetMinSize();
	pOther->SetPosition(CRect(CPoint(nX, nYCenter - sizeOther.cy / 2), sizeOther));
	nX += sizeOther.cx + m_nGap;

	CButtonCtrl* pBackColor = GetBtnBackColor();
	if (pBackColor == NULL) {
		return;
	}
	if (pBackColor->IsShown()) {
		CSize sizeBack = pBackColor->GetMinSize();
		pBackColor->SetPosition(CRect(CPoint(nX, nYCenter - sizeBack.cy / 2), sizeBack));
		nX += sizeBack.cx + m_nGap;
	}

	CSliderDouble* pOpacity = GetSliderOpacity();
	if (pOpacity == NULL) {
		return;
	}
	CSize sizeOpacity = pOpacity->GetMinSize();
	pOpacity->SetPosition(CRect(CPoint(nX, nYCenter - sizeOpacity.cy / 2), sizeOpacity));
	nX += sizeOpacity.cx + m_nGap;

	CSliderDouble* pWidth = GetSliderWidth();
	if (pWidth == NULL) {
		return;
	}
	CSize sizeWidth = pWidth->GetMinSize();
	pWidth->SetPosition(CRect(CPoint(nX, nYCenter - sizeWidth.cy / 2), sizeWidth));
}
