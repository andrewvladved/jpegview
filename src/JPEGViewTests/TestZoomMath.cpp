#include "TestFramework.h"
#include "ZoomMath.h"
#include <math.h>

// Without relative mode the step is chosen so that zooming from fit-to-screen reaches the
// image's own 100% exactly. That is what makes the step depend on the image.
TEST(StepMultiplierReachesTheImagesOwnScaleInWholeSteps) {
	double dZoomToFit = 0.25;
	double dStep = ZoomMath::StepMultiplier(dZoomToFit, false);
	int nSteps = (int)(log(1 / dZoomToFit) / log(dStep) + 0.5);
	CHECK_NEAR(dZoomToFit * pow(dStep, nSteps), 1.0, 0.0001);
	CHECK(dStep > 1.05 && dStep < 1.15);
}

TEST(StepMultiplierIsOnePointOneWhenTheImageAlreadyFits) {
	CHECK_NEAR(ZoomMath::StepMultiplier(1.0, false), 1.1, 0.0001);
}

// The point of the relative mode: the same step for every image, whatever its size.
TEST(StepMultiplierIsTheSameForEveryImageInRelativeMode) {
	double dBigPhoto = ZoomMath::StepMultiplier(0.11, true);
	double dSmallPicture = ZoomMath::StepMultiplier(0.97, true);
	CHECK_NEAR(dBigPhoto, dSmallPicture, 0.000001);
	CHECK_NEAR(dBigPhoto, 1.1, 0.0001);
}

TEST(StepMultiplierDiffersBetweenImagesWithoutRelativeMode) {
	double dBigPhoto = ZoomMath::StepMultiplier(0.11, false);
	double dSmallPicture = ZoomMath::StepMultiplier(0.97, false);
	CHECK(fabs(dBigPhoto - dSmallPicture) > 0.001);
}

TEST(AbsoluteZoomScalesTheFactorByTheAnchor) {
	CHECK_NEAR(ZoomMath::AbsoluteZoom(2.0, 0.25), 0.5, 0.000001);
	CHECK_NEAR(ZoomMath::AbsoluteZoom(0.5, 0.25), 0.125, 0.000001);
}

TEST(AbsoluteZoomLeavesTheFactorAloneWithoutAnAnchor) {
	CHECK_NEAR(ZoomMath::AbsoluteZoom(4.0, 1.0), 4.0, 0.000001);
}

TEST(FormatZoomShowsOnePercentageWithoutAnAnchor) {
	CHECK(ZoomMath::FormatZoom(2.0, 1.0) == CString(_T("200 %")));
}

// In relative mode the read-out has to agree with the menu entry that was pressed, so the
// value against the anchor comes first; the true scale stays visible in brackets.
TEST(FormatZoomShowsBothPercentagesWithAnAnchor) {
	CHECK(ZoomMath::FormatZoom(0.5, 0.25) == CString(_T("200 % (50 %)")));
	CHECK(ZoomMath::FormatZoom(0.25, 0.25) == CString(_T("100 % (25 %)")));
}
