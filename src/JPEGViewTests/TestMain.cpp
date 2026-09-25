#include "StdAfx.h"
#include "TestFramework.h"

int g_nFailures = 0;

std::vector<CTestCase>& AllTests() {
	static std::vector<CTestCase> tests;
	return tests;
}

int main() {
	std::vector<CTestCase>& tests = AllTests();
	for (size_t i = 0; i < tests.size(); i++) {
		printf("[ RUN ] %s\n", tests[i].sName);
		tests[i].pFunction();
	}
	printf("\n%d test(s) run, %d failure(s)\n", (int)tests.size(), g_nFailures);
	return g_nFailures == 0 ? 0 : 1;
}
