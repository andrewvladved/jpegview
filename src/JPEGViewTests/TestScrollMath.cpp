#include "TestFramework.h"
#include "ScrollMath.h"

using namespace ScrollMath;

TEST(ResetParksTheImageAtItsTopEdge) {
	SState state;
	Reset(state, 400);
	CHECK(state.ePhase == PHASE_HoldTop);
	CHECK_NEAR(state.dOffsetY, 400.0, 0.001);
	CHECK(state.nPhaseElapsedMs == 0);
	CHECK(!state.bAdvanceToNextImage);
}

TEST(TheImageStandsStillWhileItIsHeldAtTheTop) {
	SState state;
	Reset(state, 400);
	Advance(state, 400, 100.0, 2000, 500);
	CHECK(state.ePhase == PHASE_HoldTop);
	CHECK_NEAR(state.dOffsetY, 400.0, 0.001);
	CHECK(!state.bAdvanceToNextImage);
}

TEST(TheGlideStartsOnceTheHoldTimeHasPassed) {
	SState state;
	Reset(state, 400);
	Advance(state, 400, 100.0, 2000, 2000);
	CHECK(state.ePhase == PHASE_Moving);
	CHECK_NEAR(state.dOffsetY, 400.0, 0.001);
}

TEST(TheSpeedIsInPixelsPerSecond) {
	SState state;
	Reset(state, 400);
	Advance(state, 400, 100.0, 0, 0);   // no hold, straight into the glide
	Advance(state, 400, 100.0, 0, 500); // half a second at 100 px/s is 50 px
	CHECK(state.ePhase == PHASE_Moving);
	CHECK_NEAR(state.dOffsetY, 350.0, 0.001);
}

// A slow timer must not make the image travel faster or slower than a fast one.
TEST(TheDistanceDoesNotDependOnTheTickSize) {
	SState fewTicks, manyTicks;
	Reset(fewTicks, 4000);
	Reset(manyTicks, 4000);
	Advance(fewTicks, 4000, 200.0, 0, 0);
	Advance(manyTicks, 4000, 200.0, 0, 0);
	Advance(fewTicks, 4000, 200.0, 0, 500);
	for (int i = 0; i < 10; i++) {
		Advance(manyTicks, 4000, 200.0, 0, 50);
	}
	CHECK_NEAR(fewTicks.dOffsetY, manyTicks.dOffsetY, 0.001);
}

TEST(TheGlideStopsExactlyAtTheBottomEdge) {
	SState state;
	Reset(state, 300);
	Advance(state, 300, 100.0, 0, 0);
	Advance(state, 300, 100.0, 0, 60000); // far more than enough to reach the bottom
	CHECK(state.ePhase == PHASE_HoldBottom);
	CHECK_NEAR(state.dOffsetY, -300.0, 0.001);
	CHECK(!state.bAdvanceToNextImage);
}

TEST(TheNextImageIsAskedForAfterTheHoldAtTheBottom) {
	SState state;
	Reset(state, 300);
	Advance(state, 300, 100.0, 1000, 1000); // hold at the top is over
	Advance(state, 300, 100.0, 1000, 60000); // glide down, now holding at the bottom
	CHECK(state.ePhase == PHASE_HoldBottom);
	CHECK(!state.bAdvanceToNextImage);
	Advance(state, 300, 100.0, 1000, 1000);
	CHECK(state.bAdvanceToNextImage);
}

// An image no taller than the window has nothing to glide through, but it must still be
// shown for the hold time instead of flashing past.
TEST(AnImageThatNeedsNoScrollingIsStillHeldBeforeTheNextOne) {
	SState state;
	Reset(state, 0);
	Advance(state, 0, 100.0, 1500, 1000);
	CHECK(state.ePhase == PHASE_HoldTop);
	CHECK(!state.bAdvanceToNextImage);
	Advance(state, 0, 100.0, 1500, 500);
	CHECK(state.bAdvanceToNextImage);
}

// The next image command in scroll mode does not wait out the hold at the top: the image
// starts gliding down straight away.
TEST(StartMovingDownSkipsTheWaitAtTheTop) {
	SState state;
	Reset(state, 400);
	Advance(state, 400, 100.0, 10000, 200); // still holding, the wait is ten seconds
	CHECK(state.ePhase == PHASE_HoldTop);
	StartMovingDown(state);
	CHECK(state.ePhase == PHASE_Moving);
	Advance(state, 400, 100.0, 10000, 1000);
	CHECK_NEAR(state.dOffsetY, 300.0, 0.001);
}

// The previous image command sends it the other way, also at once.
TEST(StartMovingUpGlidesTowardsTheTopEdge) {
	SState state;
	Reset(state, 400);
	StartMovingDown(state);
	Advance(state, 400, 100.0, 0, 3000); // 300 px down, now at 100
	CHECK_NEAR(state.dOffsetY, 100.0, 0.001);
	StartMovingUp(state);
	Advance(state, 400, 100.0, 0, 1000);
	CHECK(state.ePhase == PHASE_Moving);
	CHECK_NEAR(state.dOffsetY, 200.0, 0.001);
}

TEST(GlidingUpStopsExactlyAtTheTopEdge) {
	SState state;
	Reset(state, 400);
	StartMovingDown(state);
	Advance(state, 400, 100.0, 0, 3000);
	StartMovingUp(state);
	Advance(state, 400, 100.0, 1000, 60000); // far more than enough to get back up
	CHECK(state.ePhase == PHASE_HoldTop);
	CHECK_NEAR(state.dOffsetY, 400.0, 0.001);
	CHECK(!state.bAdvanceToNextImage);
}

// Having come back up, the cycle carries on as usual: the hold, then down again.
TEST(AfterGlidingUpTheCycleCarriesOn) {
	SState state;
	Reset(state, 400);
	StartMovingUp(state);
	Advance(state, 400, 100.0, 1000, 60000); // already at the top, so it holds there
	CHECK(state.ePhase == PHASE_HoldTop);
	Advance(state, 400, 100.0, 1000, 1000);
	CHECK(state.ePhase == PHASE_Moving);
	CHECK(!state.bMovingUp);
}

// The down key sets the image gliding down, wherever the cycle happens to be.
TEST(StepDownGlidesDownWhileHoldingAtTheTop) {
	SState state;
	Reset(state, 400);
	CHECK(ActionForStepDown(state) == ACTION_MoveDown);
}

TEST(StepDownGlidesDownWhileAlreadyMoving) {
	SState state;
	Reset(state, 400);
	StartMovingUp(state);
	CHECK(ActionForStepDown(state) == ACTION_MoveDown);
}

// Standing at the bottom there is nothing below to glide to, so it moves on instead of
// waiting out the hold.
TEST(StepDownAtTheBottomGoesToTheNextImage) {
	SState state;
	Reset(state, 300);
	StartMovingDown(state);
	Advance(state, 300, 100.0, 5000, 60000);
	CHECK(state.ePhase == PHASE_HoldBottom);
	CHECK(ActionForStepDown(state) == ACTION_NextImage);
}

TEST(StepUpGlidesUpWhileHoldingAtTheBottom) {
	SState state;
	Reset(state, 300);
	StartMovingDown(state);
	Advance(state, 300, 100.0, 5000, 60000);
	CHECK(ActionForStepUp(state) == ACTION_MoveUp);
}

TEST(StepUpGlidesUpWhileAlreadyMoving) {
	SState state;
	Reset(state, 400);
	StartMovingDown(state);
	CHECK(ActionForStepUp(state) == ACTION_MoveUp);
}

// Standing at the top, up goes back a picture.
TEST(StepUpAtTheTopGoesToThePreviousImage) {
	SState state;
	Reset(state, 400);
	CHECK(state.ePhase == PHASE_HoldTop);
	CHECK(ActionForStepUp(state) == ACTION_PreviousImage);
}

// Accent on center: the glide slows down towards the middle of the image and speeds up again.
TEST(TheAccentFactorIsThirtyPercentAtTheCentre) {
	CHECK_NEAR(AccentSpeedFactor(0.0, 400), 0.3, 0.0001);
}

// Halfway between the centre and an edge the speed is halfway back up: 30% + 70% / 2.
TEST(TheAccentFactorIsHalfwayBackUpHalfwayToTheEdge) {
	CHECK_NEAR(AccentSpeedFactor(200.0, 400), 0.65, 0.0001);
	CHECK_NEAR(AccentSpeedFactor(-200.0, 400), 0.65, 0.0001);
}

TEST(TheAccentFactorIsTheFullSpeedAtBothEdges) {
	CHECK_NEAR(AccentSpeedFactor(400.0, 400), 1.0, 0.0001);
	CHECK_NEAR(AccentSpeedFactor(-400.0, 400), 1.0, 0.0001);
}

TEST(TheAccentFactorIsTheSameAboveAndBelowTheCentre) {
	CHECK_NEAR(AccentSpeedFactor(150.0, 400), AccentSpeedFactor(-150.0, 400), 0.0001);
}

TEST(TheAccentFactorGrowsSteadilyFromTheCentreToTheEdge) {
	double dLast = AccentSpeedFactor(0.0, 400);
	for (int i = 1; i <= 40; i++) {
		double dFactor = AccentSpeedFactor(i * 10.0, 400);
		CHECK(dFactor > dLast);
		dLast = dFactor;
	}
}

// Smooth: next to the centre and next to the edges the speed hardly changes, so there is no kink.
TEST(TheAccentFactorHasNoKinkAtTheCentreOrTheEdges) {
	CHECK(AccentSpeedFactor(4.0, 400) - AccentSpeedFactor(0.0, 400) < 0.001);
	CHECK(AccentSpeedFactor(400.0, 400) - AccentSpeedFactor(396.0, 400) < 0.001);
}

TEST(AnImageWithNothingToGlideThroughHasTheFullSpeed) {
	CHECK_NEAR(AccentSpeedFactor(0.0, 0), 1.0, 0.0001);
}

TEST(WithTheAccentTheGlideLeavesTheTopEdgeAtFullSpeed) {
	SState state;
	Reset(state, 4000);
	StartMovingDown(state);
	Advance(state, 4000, 100.0, 0, 100, true);
	CHECK_NEAR(state.dOffsetY, 3990.0, 0.01);
}

TEST(WithTheAccentTheGlidePassesTheCentreAtThirtyPercent) {
	SState state;
	Reset(state, 4000);
	StartMovingDown(state);
	state.dOffsetY = 5.0;
	Advance(state, 4000, 100.0, 0, 100, true); // 10 px at full speed, 3 px at the centre
	CHECK_NEAR(state.dOffsetY, 2.0, 0.01);
}

TEST(WithTheAccentTheGlideUpSlowsDownAtTheCentreToo) {
	SState state;
	Reset(state, 4000);
	StartMovingUp(state);
	state.dOffsetY = -5.0;
	Advance(state, 4000, 100.0, 0, 100, true);
	CHECK_NEAR(state.dOffsetY, -2.0, 0.01);
}

TEST(WithTheAccentTheGlideStillStopsExactlyAtTheBottomEdge) {
	SState state;
	Reset(state, 300);
	StartMovingDown(state);
	Advance(state, 300, 100.0, 0, 60000, true);
	CHECK(state.ePhase == PHASE_HoldBottom);
	CHECK_NEAR(state.dOffsetY, -300.0, 0.001);
}

// Slower in the middle, so the whole way down takes longer than without the accent.
TEST(WithTheAccentTheWayDownTakesLonger) {
	SState plain, accented;
	Reset(plain, 1000);
	Reset(accented, 1000);
	StartMovingDown(plain);
	StartMovingDown(accented);
	for (int i = 0; i < 21000 / 33; i++) { // 20 s is the plain way down
		Advance(plain, 1000, 100.0, 0, 33);
		Advance(accented, 1000, 100.0, 0, 33, true);
	}
	CHECK(plain.ePhase == PHASE_HoldBottom);
	CHECK(accented.ePhase == PHASE_Moving);
}

TEST(WithTheAccentTheDistanceDoesNotDependOnTheTickSize) {
	SState fewTicks, manyTicks;
	Reset(fewTicks, 4000);
	Reset(manyTicks, 4000);
	StartMovingDown(fewTicks);
	StartMovingDown(manyTicks);
	Advance(fewTicks, 4000, 200.0, 0, 5000, true);
	for (int i = 0; i < 100; i++) {
		Advance(manyTicks, 4000, 200.0, 0, 50, true);
	}
	CHECK_NEAR(fewTicks.dOffsetY, manyTicks.dOffsetY, 0.5);
}

// Zoom mode: the zoom has no end point. It starts from the zoom it is given and grows at
// the zoom speed for the zoom duration - the same for every image.
TEST(TheZoomRunsForItsDurationWhateverTheImage) {
	int nMax = ZoomRunMaxOffset(10.0, 3000);
	CHECK(nMax > 0);
	double dSpeed = ZoomSpeedUnitsPerSecond(10.0);
	SState state;
	Reset(state, nMax);
	StartMovingDown(state);
	Advance(state, nMax, dSpeed, 500, 2950);
	CHECK(state.ePhase == PHASE_Moving);
	Advance(state, nMax, dSpeed, 500, 100);
	CHECK(state.ePhase == PHASE_HoldBottom);
}

TEST(TheZoomStartsFromTheZoomItIsGiven) {
	int nMax = ZoomRunMaxOffset(10.0, 3000);
	CHECK_NEAR(ZoomRunZoomAt(nMax, nMax, 0.37, false), 0.37, 0.000001);
	CHECK_NEAR(ZoomRunZoomAt(nMax, nMax, 0.37, true), 0.37, 0.000001);
}

// At 10% per second the zoom is 1.1 times larger after a second, and after the whole three
// seconds 1.1^3 times - for a small start zoom and a large one alike.
TEST(TheZoomGrowsByTheZoomSpeedEverySecond) {
	int nMax = ZoomRunMaxOffset(10.0, 3000);
	double dSpeed = ZoomSpeedUnitsPerSecond(10.0);
	SState state;
	Reset(state, nMax);
	StartMovingDown(state);
	Advance(state, nMax, dSpeed, 0, 1000);
	CHECK_NEAR(ZoomRunZoomAt(state.dOffsetY, nMax, 0.5, false), 0.55, 0.001);
	CHECK_NEAR(ZoomRunZoomAt(state.dOffsetY, nMax, 2.0, false), 2.2, 0.002);
	Advance(state, nMax, dSpeed, 0, 60000);
	CHECK_NEAR(ZoomRunZoomAt(state.dOffsetY, nMax, 0.5, false), 0.5 * 1.331, 0.001);
}

TEST(InverseZoomShrinksByTheZoomSpeedEverySecond) {
	int nMax = ZoomRunMaxOffset(10.0, 3000);
	double dSpeed = ZoomSpeedUnitsPerSecond(10.0);
	SState state;
	Reset(state, nMax);
	StartMovingDown(state);
	Advance(state, nMax, dSpeed, 0, 1000);
	CHECK_NEAR(ZoomRunZoomAt(state.dOffsetY, nMax, 1.0, true), 1.0 / 1.1, 0.001);
	Advance(state, nMax, dSpeed, 0, 60000);
	CHECK_NEAR(ZoomRunZoomAt(state.dOffsetY, nMax, 1.0, true), 1.0 / 1.331, 0.001);
}

TEST(AZeroDurationLeavesNothingToZoomThrough) {
	CHECK(ZoomRunMaxOffset(10.0, 0) == 0);
	CHECK_NEAR(ZoomRunZoomAt(0, 0, 0.75, false), 0.75, 0.000001);
}

// Inverse pulls back from fill with crop, but never further than fit to screen: once there
// it stays for the rest of the duration.
TEST(InverseZoomStopsAtFitToScreen) {
	int nMax = ZoomRunMaxOffset(20.0, 6000);
	double dStart = 2.0, dFit = 1.5;
	double dEarly = ZoomRunZoomAt(nMax - 10, nMax, dStart, true);
	CHECK(dEarly > dFit);
	CHECK_NEAR(ZoomRunLimitToFit(dEarly, dFit, true), dEarly, 0.000001);
	double dLate = ZoomRunZoomAt(-nMax, nMax, dStart, true);
	CHECK(dLate < dFit);
	CHECK_NEAR(ZoomRunLimitToFit(dLate, dFit, true), dFit, 0.000001);
}

TEST(ZoomingInIsNotLimitedByFitToScreen) {
	CHECK_NEAR(ZoomRunLimitToFit(3.0, 1.5, false), 3.0, 0.000001);
	CHECK_NEAR(ZoomRunLimitToFit(1.0, 1.5, false), 1.0, 0.000001);
}
