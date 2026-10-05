#include "TestFramework.h"
#include "WindowMath.h"

static RECT Rect(int nLeft, int nTop, int nRight, int nBottom) {
	RECT rect = { nLeft, nTop, nRight, nBottom };
	return rect;
}

TEST(MaximizedPlacementLeavesTheTaskbarAtTheBottomAlone) {
	WindowMath::SPlacement p = WindowMath::MaximizedPlacement(
		Rect(0, 0, 1920, 1080), Rect(0, 0, 1920, 1034));
	CHECK(p.nX == 0);
	CHECK(p.nY == 0);
	CHECK(p.nWidth == 1920);
	CHECK(p.nHeight == 1034);
}

TEST(MaximizedPlacementStartsBelowATaskbarAtTheTop) {
	WindowMath::SPlacement p = WindowMath::MaximizedPlacement(
		Rect(0, 0, 1920, 1080), Rect(0, 46, 1920, 1080));
	CHECK(p.nX == 0);
	CHECK(p.nY == 46);
	CHECK(p.nWidth == 1920);
	CHECK(p.nHeight == 1034);
}

TEST(MaximizedPlacementStartsBesideATaskbarOnTheLeft) {
	WindowMath::SPlacement p = WindowMath::MaximizedPlacement(
		Rect(0, 0, 1920, 1080), Rect(60, 0, 1920, 1080));
	CHECK(p.nX == 60);
	CHECK(p.nY == 0);
	CHECK(p.nWidth == 1860);
	CHECK(p.nHeight == 1080);
}

// The position WM_GETMINMAXINFO wants is measured from the monitor, so a window maximized
// on a second screen must not be pushed off by that screen's place in the virtual desktop.
TEST(MaximizedPlacementIsMeasuredFromTheMonitorNotTheDesktop) {
	WindowMath::SPlacement p = WindowMath::MaximizedPlacement(
		Rect(1920, 0, 3840, 1080), Rect(1920, 0, 3840, 1034));
	CHECK(p.nX == 0);
	CHECK(p.nY == 0);
	CHECK(p.nWidth == 1920);
	CHECK(p.nHeight == 1034);
}

TEST(MaximizedPlacementHandlesAMonitorLeftOfAndAboveThePrimaryOne) {
	WindowMath::SPlacement p = WindowMath::MaximizedPlacement(
		Rect(-1600, -200, 320, 880), Rect(-1600, -160, 320, 880));
	CHECK(p.nX == 0);
	CHECK(p.nY == 40);
	CHECK(p.nWidth == 1920);
	CHECK(p.nHeight == 1040);
}

// A work area equal to the monitor - no taskbar on this screen - is still the whole screen.
TEST(MaximizedPlacementFillsTheScreenWhenNothingIsReserved) {
	WindowMath::SPlacement p = WindowMath::MaximizedPlacement(
		Rect(0, 0, 2560, 1440), Rect(0, 0, 2560, 1440));
	CHECK(p.nX == 0);
	CHECK(p.nY == 0);
	CHECK(p.nWidth == 2560);
	CHECK(p.nHeight == 1440);
}
