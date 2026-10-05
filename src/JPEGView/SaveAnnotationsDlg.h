// Asks what to do with unsaved annotations before leaving an image
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "resource.h"

// Four choices do not fit a MessageBox, and TaskDialog needs Windows Vista while this
// project targets _WIN32_WINNT 0x0501, so this is a plain dialog resource.
// DoModal returns IDC_ANNOT_OVERWRITE, IDC_ANNOT_SAVEAS, IDC_ANNOT_DISCARD or IDCANCEL.
class CSaveAnnotationsDlg : public CDialogImpl<CSaveAnnotationsDlg>
{
public:
	enum { IDD = IDD_SAVE_ANNOTATIONS };

	CSaveAnnotationsDlg(LPCTSTR sFileName) : m_sFileName(sFileName) {}

	BEGIN_MSG_MAP(CSaveAnnotationsDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDC_ANNOT_OVERWRITE, OnButton)
		COMMAND_ID_HANDLER(IDC_ANNOT_SAVEAS, OnButton)
		COMMAND_ID_HANDLER(IDC_ANNOT_DISCARD, OnButton)
		COMMAND_ID_HANDLER(IDCANCEL, OnButton)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnButton(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

private:
	CString m_sFileName;
};
