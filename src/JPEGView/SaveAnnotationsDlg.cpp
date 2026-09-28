#include "StdAfx.h"
#include "SaveAnnotationsDlg.h"
#include "NLS.h"

LRESULT CSaveAnnotationsDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	CenterWindow(GetParent());
	SetWindowText(CNLS::GetString(_T("JPEGView")));
	SetDlgItemText(IDC_ANNOT_TITLE,
		CNLS::GetString(_T("This image has annotations that have not been saved")));
	SetDlgItemText(IDC_ANNOT_FILENAME, m_sFileName);
	SetDlgItemText(IDC_ANNOT_OVERWRITE, CNLS::GetString(_T("Overwrite")));
	SetDlgItemText(IDC_ANNOT_SAVEAS, CNLS::GetString(_T("Save as...")));
	SetDlgItemText(IDC_ANNOT_DISCARD, CNLS::GetString(_T("Do not save")));
	SetDlgItemText(IDCANCEL, CNLS::GetString(_T("Cancel")));

	// Save as... takes the focus, because it is the only choice that cannot destroy the
	// original by an accidental Enter.
	::SetFocus(GetDlgItem(IDC_ANNOT_SAVEAS));
	return FALSE; // focus was set explicitly
}

LRESULT CSaveAnnotationsDlg::OnButton(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	EndDialog(wID);
	return 0;
}
