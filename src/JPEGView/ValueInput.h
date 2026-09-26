// Parsing of the numbers typed into the value entry dialog
/////////////////////////////////////////////////////////////////////////////

#pragma once

// Self-sufficient on purpose: this is compiled into the unit test project as well as
// into JPEGView, so it must not depend on the application's StdAfx.h (which pulls in WTL).
#include <windows.h>
#include <tchar.h>

namespace ValueInput {

	// Reads the number the user typed into a numeric input field. Text that is not a
	// number at all - an empty field, only spaces, something pasted in - yields
	// nFallback, so the value the field was opened with survives. A number outside the
	// allowed range is clamped rather than rejected: a typo should not leave the user
	// without a way to start a slideshow.
	int ParseClamped(LPCTSTR sText, int nMin, int nMax, int nFallback);
}
