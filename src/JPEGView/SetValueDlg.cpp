#include "StdAfx.h"
#include "SetValueDlg.h"
#include "NLS.h"
#include "ValueInput.h"

LRESULT CSetValueDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	// Open where the mouse is, the way the 'Set Fixed Crop Size' dialog does: both are
	// reached from the context menu, so the pointer is already at the right place.
	CPoint mousePos;
	CRect wndRect;
	GetWindowRect(&wndRect);
	::GetCursorPos(&mousePos);
	this->SetWindowPos(m_hWnd, mousePos.x - wndRect.Width()/2, mousePos.y - wndRect.Height()/2, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

	this->SetWindowText(m_sTitle);
	::SetWindowText(GetDlgItem(IDC_SETVAL_LABEL), m_sLabel);
	::SetWindowText(GetDlgItem(IDC_SETVAL_UNIT), m_sUnit);
	::SetWindowText(GetDlgItem(IDOK), CNLS::GetString(_T("OK")));
	::SetWindowText(GetDlgItem(IDCANCEL), CNLS::GetString(_T("Cancel")));

	m_edtValue.Attach(GetDlgItem(IDC_SETVAL_EDIT));
	CString sValue;
	sValue.Format(_T("%d"), m_nValue);
	m_edtValue.SetWindowText(sValue);
	m_edtValue.SetSelAll();
	m_edtValue.SetFocus();
	return FALSE; // focus was set above, do not let the dialog manager move it
}

LRESULT CSetValueDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	const int MAX_LEN = 16;
	TCHAR sText[MAX_LEN];
	m_edtValue.GetWindowText(sText, MAX_LEN);
	m_nValue = ValueInput::ParseClamped(sText, m_nMin, m_nMax, m_nValue);
	EndDialog(IDOK);
	return 0;
}

LRESULT CSetValueDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	EndDialog(IDCANCEL);
	return 0;
}
