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

static bool Same(const RECT& r, int nLeft, int nTop, int nRight, int nBottom) {
	return r.left == nLeft && r.top == nTop && r.right == nRight && r.bottom == nBottom;
}

TEST(ClampToWorkAreaLeavesARectThatAlreadyFitsAlone) {
	RECT r = WindowMath::ClampToWorkArea(Rect(100, 80, 900, 700), Rect(0, 0, 1920, 1042));
	CHECK(Same(r, 100, 80, 900, 700));
}

// The rect this started from: remembered while the window covered the taskbar, and larger
// than the screen on every side.
TEST(ClampToWorkAreaShrinksARectBiggerThanTheScreen) {
	RECT r = WindowMath::ClampToWorkArea(Rect(-11, 0, 1925, 1096), Rect(0, 0, 1920, 1042));
	CHECK(Same(r, 0, 0, 1920, 1042));
}

TEST(ClampToWorkAreaSlidesARectUpOffTheTaskbarKeepingItsSize) {
	RECT r = WindowMath::ClampToWorkArea(Rect(100, 900, 900, 1100), Rect(0, 0, 1920, 1042));
	CHECK(Same(r, 100, 842, 900, 1042));
}

TEST(ClampToWorkAreaSlidesARectInFromTheRightKeepingItsSize) {
	RECT r = WindowMath::ClampToWorkArea(Rect(1800, 100, 2100, 500), Rect(0, 0, 1920, 1042));
	CHECK(Same(r, 1620, 100, 1920, 500));
}

TEST(ClampToWorkAreaStartsBelowATaskbarAtTheTop) {
	RECT r = WindowMath::ClampToWorkArea(Rect(0, 0, 1920, 1080), Rect(0, 46, 1920, 1080));
	CHECK(Same(r, 0, 46, 1920, 1080));
}

// A second monitor has its own work area, and a rect belonging to it must not be dragged
// back onto the primary one.
TEST(ClampToWorkAreaKeepsARectOnItsOwnMonitor) {
	RECT r = WindowMath::ClampToWorkArea(Rect(2000, 900, 2800, 1100), Rect(1920, 0, 3840, 1034));
	CHECK(Same(r, 2000, 834, 2800, 1034));
}

TEST(ClampToWorkAreaPullsARectInFromTheLeftAndTop) {
	RECT r = WindowMath::ClampToWorkArea(Rect(-200, -150, 600, 450), Rect(0, 0, 1920, 1042));
	CHECK(Same(r, 0, 0, 800, 600));
}
