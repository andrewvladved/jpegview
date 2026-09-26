// Asks the user for a single number
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "resource.h"

// A one field dialog used by the 'Set Waiting Time' and 'Set Playback Speed' menu
// entries. The caller passes the texts already translated, the value to start from and
// the range to accept; DoModal returns IDOK or IDCANCEL and GetValue() holds the
// number the user settled on.
class CSetValueDlg : public CDialogImpl<CSetValueDlg>
{
public:
	enum { IDD = IDD_SET_VALUE };

	CSetValueDlg(LPCTSTR sTitle, LPCTSTR sLabel, LPCTSTR sUnit, int nValue, int nMin, int nMax)
		: m_sTitle(sTitle), m_sLabel(sLabel), m_sUnit(sUnit), m_nValue(nValue), m_nMin(nMin), m_nMax(nMax) {}

	BEGIN_MSG_MAP(CSetValueDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	int GetValue() const { return m_nValue; }

private:
	CString m_sTitle;
	CString m_sLabel;
	CString m_sUnit;
	int m_nValue;
	int m_nMin;
	int m_nMax;

	CEdit m_edtValue;
};
