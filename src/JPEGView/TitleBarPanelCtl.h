#pragma once

#include "PanelController.h"

class CTitleBarPanel;

// Implements functionality of the title bar panel used in transparent title bar mode
// (file path on top, left and minimize/maximize/close buttons on top, right corner of the window)
class CTitleBarPanelCtl : public CPanelController
{
public:
	CTitleBarPanelCtl(CMainDlg* pMainDlg);
	virtual ~CTitleBarPanelCtl();

	virtual float DimFactor() { return 0.0f; } // no dimming, the image shines through unchanged
	virtual bool BlendPanel() { return false; }

	virtual bool IsVisible();
	virtual bool IsActive() { return IsVisible(); }

	virtual void SetVisible(bool bVisible) {} // visibility only depends on the transparent title bar mode
	virtual void SetActive(bool bActive) {}

	virtual void AfterNewImageLoaded();
	virtual void AfterImageRenamed();

	// Gets if the given point is inside the title bar but not on one of its buttons.
	// The window can be dragged in this area. Returns false when the title bar is not visible.
	bool IsPointInDragArea(CPoint pt);

	// Gets if the given point is on one of the window buttons. Returns false when the title bar is not visible.
	bool IsPointInButtonArea(CPoint pt);

	// Height of the title bar in pixels
	int TitleBarHeight();

	// Updates the displayed file path from the main dialog
	void UpdateFilePath();

private:
	CTitleBarPanel* m_pTitleBarPanel;
};
