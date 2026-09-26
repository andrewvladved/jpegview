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

	// Dimmed, unlike the transparent title bar: this strip has colour swatches and thin
	// slider marks that must stay legible over any image, including a bright one.
	// BlendPanel is false as it is for every other panel carrying controls - only the
	// navigation panel sets it, and only to drive its fade-in animation. Leaving it on
	// blended the strip a second time on top of the dimming and made it washed out.
	virtual float DimFactor() { return 0.75f; }
	virtual bool BlendPanel() { return false; }

	void Toggle() { SetVisible(!m_bVisible); }

	// Called whenever the selected tool or one of its modes changes. The two numbers
	// belong to the tool, so the sliders have to be reloaded, the width slider
	// relabelled, and the back colour button shown only for a text label with a backing.
	void OnToolChanged();

	// Called back by CMainDlg when the user typed a number into one of the two fields.
	enum { ENTRY_OPACITY = 1, ENTRY_WIDTH = 2 };
	void SetValueFromEntry(int nWhich, int nValue);

	virtual bool OnMouseLButton(EMouseEvent eMouseEvent, int nX, int nY);
	virtual bool OnMouseMove(int nX, int nY);

private:
	static void OnSwatchPressed(void* pContext, int nParameter, CButtonCtrl& sender);
	static void OnOtherColorPressed(void* pContext, int nParameter, CButtonCtrl& sender);
	static void OnBackColorPressed(void* pContext, int nParameter, CButtonCtrl& sender);
	static bool PickColor(CMainDlg* pMainDlg, COLORREF& color);

	void ApplyStyle(bool bPersist);
	void LoadFromControl();
	void RelabelWidthSlider();
	void UpdateBackColorButton();
	void CheckForValueEntry();

	CAnnotationStylePanel* m_pStylePanel;
	bool m_bVisible;
	double m_dOpacity;   // percent, bound to the opacity slider
	double m_dWidth;     // screen pixels, bound to the width slider
	COLORREF m_color;
};
