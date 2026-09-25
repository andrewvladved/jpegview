#pragma once

#include "Panel.h"

// Title bar panel - shows the file path on the left and the window buttons
// (minimize, maximize/restore, close) on the right. The panel has no background of its own,
// it is painted directly over the image. Used in transparent title bar mode.
class CTitleBarPanel : public CPanel {
public:
	// IDs of the controls on this panel
	enum {
		ID_txtFilePath,
		ID_btnMinimize,
		ID_btnMaximize,
		ID_btnClose
	};
public:
	// The panel is on the given window, spanning the whole width of the top border
	CTitleBarPanel(HWND hWnd, INotifiyMouseCapture* pNotifyMouseCapture);

	CTextCtrl* GetTextFilePath() { return GetControl<CTextCtrl*>(ID_txtFilePath); }
	CButtonCtrl* GetBtnMinimize() { return GetControl<CButtonCtrl*>(ID_btnMinimize); }
	CButtonCtrl* GetBtnMaximize() { return GetControl<CButtonCtrl*>(ID_btnMaximize); }
	CButtonCtrl* GetBtnClose() { return GetControl<CButtonCtrl*>(ID_btnClose); }

	virtual CRect PanelRect();
	virtual void RequestRepositioning();

	int TitleBarHeight() { return m_nHeight; }

	// Rectangle occupied by the window buttons, in client coordinates
	CRect ButtonAreaRect();

	// Sets the text displayed on the left side of the title bar
	void SetFilePath(LPCTSTR sFilePath);

protected:
	virtual void RepositionAll();

private:
	// Painting handlers for the buttons. pContext is the panel.
	static void PaintMinimizeBtn(void* pContext, const CRect& rect, CDC& dc);
	static void PaintMaximizeBtn(void* pContext, const CRect& rect, CDC& dc);
	static void PaintCloseBtn(void* pContext, const CRect& rect, CDC& dc);

	int NumberOfButtons();

	CRect m_clientRect;
	int m_nHeight;
};
