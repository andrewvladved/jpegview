#include "ZoomMath.h"

namespace ZoomMath {

double StepMultiplier(double dZoomToFit, bool bRelativeZoom) {
	return 1.1;
}

double AbsoluteZoom(double dFactor, double dBase) {
	return dFactor;
}

CString FormatZoom(double dZoom, double dBase) {
	CString s;
	s.Format(_T("%d %%"), (int)(dZoom * 100 + 0.5));
	return s;
}

}
