#include "StdAfx.h"
#include "AnnotationTextEdit.h"

LRESULT CAnnotationTextEdit::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (m_pListener != NULL) {
		if (wParam == VK_RETURN) {
			bHandled = TRUE;
			m_pListener->OnAnnotationTextCommitted();
			return 0;
		}
		if (wParam == VK_ESCAPE) {
			bHandled = TRUE;
			m_pListener->OnAnnotationTextCancelled();
			return 0;
		}
	}
	bHandled = FALSE;
	return 0;
}

LRESULT CAnnotationTextEdit::OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	// A single line edit control beeps at Enter and Esc; they were handled in OnKeyDown.
	if (wParam == VK_RETURN || wParam == VK_ESCAPE) {
		bHandled = TRUE;
		return 0;
	}
	bHandled = FALSE;
	return 0;
}
