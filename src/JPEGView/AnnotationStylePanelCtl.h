#pragma once

#include "PanelController.h"

class CAnnotationStylePanel;
class CButtonCtrl;

// Shows and hides the annotation style strip and pushes what the user picks into the
// annotation controller, then persists it so the style survives a restart.
class CAnnotationStylePanelCtl : public CPanelController {
public:
	CAnnotationStylePanelCtl(CMainDlg* pMainDlg, CPanel* pNavPanel);
	virtual ~CAnnotationStylePanelCtl();

	virtual bool IsVisible() { return m_bVisible; }
	virtual bool IsActive() { return m_bVisible; }
	virtual void SetVisible(bool bVisible);
	virtual void SetActive(bool bActive) { SetVisible(bActive); }

	// Dimmed, unlike the transparent title bar: this strip has controls to aim at and
	// needs to stay readable over any image.
	virtual float DimFactor() { return 0.5f; }
	virtual bool BlendPanel() { return true; }

	void Toggle() { SetVisible(!m_bVisible); }

	virtual bool OnMouseLButton(EMouseEvent eMouseEvent, int nX, int nY);
	virtual bool OnMouseMove(int nX, int nY);

private:
	static void OnSwatchPressed(void* pContext, int nParameter, CButtonCtrl& sender);
	static void OnOtherColorPressed(void* pContext, int nParameter, CButtonCtrl& sender);

	void ApplyStyle();
	void LoadFromControl();
	void RelabelWidthSlider();

	CAnnotationStylePanel* m_pStylePanel;
	bool m_bVisible;
	double m_dOpacity;   // percent, bound to the opacity slider
	double m_dWidth;     // screen pixels, bound to the width slider
	int m_nPenWidth;     // kept apart so switching tools does not lose the other one
	int m_nFontSize;
	COLORREF m_color;
};
