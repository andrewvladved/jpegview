#pragma once

#include "Panel.h"

class CAnnotationCtl;

// Popup strip for choosing the annotation colour, opacity and width. Opened from the
// style button on the navigation panel and placed directly above it.
class CAnnotationStylePanel : public CPanel {
public:
	enum {
		ID_swatch0, ID_swatch1, ID_swatch2, ID_swatch3,
		ID_swatch4, ID_swatch5, ID_swatch6, ID_swatch7,
		ID_slOpacity,
		ID_slWidth,
		ID_btnOtherColor
	};

	static const int NUM_SWATCHES = 8;

	// pAnnotationCtl is read while painting to show which swatch is the current colour.
	CAnnotationStylePanel(HWND hWnd, INotifiyMouseCapture* pNotifyMouseCapture,
		CPanel* pNavPanel, CAnnotationCtl* pAnnotationCtl, double* pdOpacity, double* pdWidth);

	static COLORREF SwatchColor(int nIndex);

	CButtonCtrl* GetSwatch(int nIndex) { return GetControl<CButtonCtrl*>(ID_swatch0 + nIndex); }
	CButtonCtrl* GetBtnOtherColor() { return GetControl<CButtonCtrl*>(ID_btnOtherColor); }
	CSliderDouble* GetSliderOpacity() { return GetControl<CSliderDouble*>(ID_slOpacity); }
	CSliderDouble* GetSliderWidth() { return GetControl<CSliderDouble*>(ID_slWidth); }

	virtual CRect PanelRect();
	virtual void RequestRepositioning();

protected:
	virtual void RepositionAll();

private:
	static void PaintSwatch(void* pContext, const CRect& rect, CDC& dc);

	CPanel* m_pNavPanel;
	CAnnotationCtl* m_pAnnotationCtl;
	CRect m_clientRect;
	int m_nWidth, m_nHeight;
	int m_nBorder, m_nGap;
	int m_nSwatchSize;
};
