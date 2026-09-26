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
