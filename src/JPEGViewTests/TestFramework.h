#pragma once

#include <stdio.h>
#include <vector>

// Minimal test runner. A test is a void() function registered by CTestRegistrar.
// CHECK records a failure and continues, so one run reports every broken expectation.

extern int g_nFailures;

struct CTestCase {
	const char* sName;
	void (*pFunction)();
};

std::vector<CTestCase>& AllTests();

struct CTestRegistrar {
	CTestRegistrar(const char* sName, void (*pFunction)()) {
		CTestCase testCase = { sName, pFunction };
		AllTests().push_back(testCase);
	}
};

#define TEST(name) \
	static void name(); \
	static CTestRegistrar registrar_##name(#name, &name); \
	static void name()

#define CHECK(condition) \
	do { \
		if (!(condition)) { \
			printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #condition); \
			g_nFailures++; \
		} \
	} while (0)

// Like CHECK, but abandons the rest of the test. Use it before indexing into a
// container whose size the test has just asserted: with a wrong implementation the
// container can be empty, and an out-of-range subscript in a debug build trips a
// C runtime assertion instead of reporting a plain failure.
#define REQUIRE(condition) \
	do { \
		if (!(condition)) { \
			printf("FAIL %s:%d  %s (required)\n", __FILE__, __LINE__, #condition); \
			g_nFailures++; \
			return; \
		} \
	} while (0)

// The locals below are deliberately named so that no caller's variable can collide
// with them: short names like `a` shadow the caller's own `a` inside the expansion,
// and the expression passed in then resolves to the macro's local instead.
#define CHECK_NEAR(actual, expected, tolerance) \
	do { \
		double dCheckNearActual_ = (double)(actual), dCheckNearExpected_ = (double)(expected); \
		if (dCheckNearActual_ < dCheckNearExpected_ - (tolerance) || \
		    dCheckNearActual_ > dCheckNearExpected_ + (tolerance)) { \
			printf("FAIL %s:%d  %s: got %f, expected %f\n", __FILE__, __LINE__, #actual, \
				dCheckNearActual_, dCheckNearExpected_); \
			g_nFailures++; \
		} \
	} while (0)
