#pragma once

// Told when the user finishes or abandons typing an annotation. CMainDlg implements it,
// which keeps this control from having to know about the dialog.
class IAnnotationTextListener {
public:
	virtual ~IAnnotationTextListener() {}
	virtual void OnAnnotationTextCommitted() = 0;
	virtual void OnAnnotationTextCancelled() = 0;
};

// The in-place text field for the text annotation tool. A real edit control is used so
// that Backspace, selection, clipboard paste and non-Latin keyboard layouts work without
// being reimplemented; Enter and Esc are intercepted because a focused edit control
// receives them before the dialog does.
class CAnnotationTextEdit : public CWindowImpl<CAnnotationTextEdit, CEdit> {
public:
	DECLARE_WND_SUPERCLASS(NULL, CEdit::GetWndClassName())

	CAnnotationTextEdit() : m_pListener(NULL) {}

	void SetListener(IAnnotationTextListener* pListener) { m_pListener = pListener; }

	BEGIN_MSG_MAP(CAnnotationTextEdit)
		MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
		MESSAGE_HANDLER(WM_CHAR, OnChar)
	END_MSG_MAP()

private:
	LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	IAnnotationTextListener* m_pListener;
};
