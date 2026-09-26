#include "ValueInput.h"

namespace ValueInput {

int ParseClamped(LPCTSTR sText, int nMin, int nMax, int nFallback) {
	if (sText == NULL) {
		return nFallback;
	}
	// %d skips leading whitespace and stops at the first character that cannot be part
	// of the number, so trailing junk is ignored. What matters is that a number was read
	// at all: on an empty field or on text _stscanf_s returns EOF or 0 respectively.
	int nValue = 0;
	if (_stscanf_s(sText, _T("%d"), &nValue) != 1) {
		return nFallback;
	}
	return (nValue < nMin) ? nMin : (nValue > nMax) ? nMax : nValue;
}

}
