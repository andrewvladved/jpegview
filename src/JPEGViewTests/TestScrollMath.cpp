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
