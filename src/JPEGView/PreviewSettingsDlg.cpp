#include "StdAfx.h"
#include "PreviewSettingsDlg.h"
#include "NLS.h"

LRESULT CPreviewSettingsDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	// Open where the mouse is, the way the other small dialogs do: they are all reached
	// from the context menu, so the pointer is already at the right place.
	CPoint mousePos;
	CRect wndRect;
	GetWindowRect(&wndRect);
	::GetCursorPos(&mousePos);
	this->SetWindowPos(m_hWnd, mousePos.x - wndRect.Width()/2, mousePos.y - wndRect.Height()/2, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

	this->SetWindowText(CNLS::GetString(_T("Set Preview Settings")));
	::SetWindowText(GetDlgItem(IDC_PV_LBL_SIZE), CNLS::GetString(_T("Preview size")));
	::SetWindowText(GetDlgItem(IDC_PV_LBL_SIDE), CNLS::GetString(_T("Preview side")));
	::SetWindowText(GetDlgItem(IDOK), CNLS::GetString(_T("OK")));
	::SetWindowText(GetDlgItem(IDCANCEL), CNLS::GetString(_T("Cancel")));

	m_cbSize.Attach(GetDlgItem(IDC_PV_SIZE));
	m_rbLeft.Attach(GetDlgItem(IDC_PV_RB_LEFT));
	m_rbRight.Attach(GetDlgItem(IDC_PV_RB_RIGHT));
	m_chkOnTop.Attach(GetDlgItem(IDC_PV_ON_TOP));

	m_rbLeft.SetWindowText(CNLS::GetString(_T("Left")));
	m_rbRight.SetWindowText(CNLS::GetString(_T("Right")));
	m_chkOnTop.SetWindowText(CNLS::GetString(_T("On top of the image")));

	int nSelected = 0;
	for (int nPercent = MIN_SIZE_PERCENT; nPercent <= MAX_SIZE_PERCENT; nPercent += SIZE_PERCENT_STEP) {
		CString sItem;
		sItem.Format(_T("%d"), nPercent);
		int nIndex = m_cbSize.AddString(sItem);
		if (nPercent <= m_nSizePercent) {
			nSelected = nIndex; // the largest step that is not above the current value
		}
	}
	m_cbSize.SetCurSel(nSelected);

	m_rbLeft.SetCheck(m_bOnLeft ? BST_CHECKED : BST_UNCHECKED);
	m_rbRight.SetCheck(m_bOnLeft ? BST_UNCHECKED : BST_CHECKED);
	m_chkOnTop.SetCheck(m_bOnTop ? BST_CHECKED : BST_UNCHECKED);
	return TRUE;
}

LRESULT CPreviewSettingsDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int nIndex = m_cbSize.GetCurSel();
	if (nIndex >= 0) {
		m_nSizePercent = MIN_SIZE_PERCENT + nIndex * SIZE_PERCENT_STEP;
	}
	m_bOnLeft = m_rbLeft.GetCheck() == BST_CHECKED;
	m_bOnTop = m_chkOnTop.GetCheck() == BST_CHECKED;
	EndDialog(IDOK);
	return 0;
}

LRESULT CPreviewSettingsDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	EndDialog(IDCANCEL);
	return 0;
}
