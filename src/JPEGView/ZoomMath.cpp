#include "ZoomMath.h"
#include <math.h>

namespace ZoomMath {

double StepMultiplier(double dZoomToFit, bool bRelativeZoom) {
	if (bRelativeZoom || dZoomToFit <= 0.0) {
		// The anchor is the fitted image, so there is nothing per image left to hit and
		// every image steps by the same amount.
		return 1.1;
	}
	// Around 1.1, but adjusted so that a whole number of steps from the fitted image
	// lands exactly on the image's own 100%.
	int n = (int)floor(log(1 / dZoomToFit) / log(1.1) + 0.5);
	return (n == 0) ? 1.1 : exp(log(1 / dZoomToFit) / n);
}

double AbsoluteZoom(double dFactor, double dBase) {
	return dFactor * dBase;
}

CString FormatZoom(double dZoom, double dBase) {
	CString s;
	if (dBase <= 0.0 || fabs(dBase - 1.0) < 0.0001) {
		s.Format(_T("%d %%"), (int)(dZoom * 100 + 0.5));
	} else {
		s.Format(_T("%d %% (%d %%)"), (int)(dZoom / dBase * 100 + 0.5), (int)(dZoom * 100 + 0.5));
	}
	return s;
}

}
