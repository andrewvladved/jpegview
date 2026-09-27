// Size, side and placement of the preview pane
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "resource.h"
#include "Helpers.h"

// The three settings behind the 'Set Preview Settings' menu entry. Whether the pane is
// drawn on top of the image is a check mark in the menu instead, so it can be switched
// without opening a dialog. The caller passes the current values and reads them back
// after DoModal() returns IDOK.
class CPreviewSettingsDlg : public CDialogImpl<CPreviewSettingsDlg>
{
public:
	enum { IDD = IDD_PREVIEW_SETTINGS };

	CPreviewSettingsDlg(int nSizePercent, bool bOnLeft, Helpers::EPreviewFloor eFloor)
		: m_nSizePercent(nSizePercent), m_bOnLeft(bOnLeft), m_eFloor(eFloor) {}

	BEGIN_MSG_MAP(CPreviewSettingsDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	int GetSizePercent() const { return m_nSizePercent; }
	bool IsOnLeft() const { return m_bOnLeft; }
	Helpers::EPreviewFloor GetFloor() const { return m_eFloor; }

	// The size is offered in steps of five percent, from a tenth of the window to all of it
	static const int MIN_SIZE_PERCENT = 10;
	static const int MAX_SIZE_PERCENT = 100;
	static const int SIZE_PERCENT_STEP = 5;

private:
	int m_nSizePercent;
	bool m_bOnLeft;
	Helpers::EPreviewFloor m_eFloor;

	CComboBox m_cbSize;
	CButton m_rbLeft;
	CButton m_rbRight;
	CButton m_rbTop;
	CButton m_rbMid;
	CButton m_rbBottom;
};
