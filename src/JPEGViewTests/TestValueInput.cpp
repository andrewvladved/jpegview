#include "TestFramework.h"
#include "ValueInput.h"

TEST(ParseClampedReturnsTheNumberTyped) {
	CHECK(ValueInput::ParseClamped(_T("7"), 1, 100, 3) == 7);
}

TEST(ParseClampedFallsBackOnAnEmptyField) {
	CHECK(ValueInput::ParseClamped(_T(""), 1, 100, 3) == 3);
}

TEST(ParseClampedFallsBackOnTextThatIsNotANumber) {
	CHECK(ValueInput::ParseClamped(_T("abc"), 1, 100, 3) == 3);
	CHECK(ValueInput::ParseClamped(_T("   "), 1, 100, 3) == 3);
}

// ES_NUMBER keeps a minus sign and letters from being typed, but not from being pasted.
TEST(ParseClampedClampsBelowTheMinimum) {
	CHECK(ValueInput::ParseClamped(_T("0"), 1, 100, 3) == 1);
	CHECK(ValueInput::ParseClamped(_T("-5"), 1, 100, 3) == 1);
}

TEST(ParseClampedClampsAboveTheMaximum) {
	CHECK(ValueInput::ParseClamped(_T("100000"), 1, 100, 3) == 100);
}

TEST(ParseClampedIgnoresSurroundingSpaces) {
	CHECK(ValueInput::ParseClamped(_T("  12  "), 1, 100, 3) == 12);
}
