#include "TestFramework.h"

#include <windows.h>
#include <crtdbg.h>

int g_nFailures = 0;

std::vector<CTestCase>& AllTests() {
	static std::vector<CTestCase> tests;
	return tests;
}

// By default a C runtime assertion or an access violation opens a modal dialog.
// On a CI runner nobody closes it and the job hangs until the whole workflow is
// killed, which is far worse than a red test. Send every report to stderr instead,
// so a broken test fails the run in seconds rather than stalling it.
static void SilenceCrashDialogs() {
	::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
	_set_error_mode(_OUT_TO_STDERR);
	int aReports[] = { _CRT_ASSERT, _CRT_ERROR, _CRT_WARN };
	for (int i = 0; i < 3; i++) {
		_CrtSetReportMode(aReports[i], _CRTDBG_MODE_FILE);
		_CrtSetReportFile(aReports[i], _CRTDBG_FILE_STDERR);
	}
}

int main() {
	SilenceCrashDialogs();

	std::vector<CTestCase>& tests = AllTests();
	for (size_t i = 0; i < tests.size(); i++) {
		printf("[ RUN ] %s\n", tests[i].sName);
		fflush(stdout);
		tests[i].pFunction();
	}
	printf("\n%d test(s) run, %d failure(s)\n", (int)tests.size(), g_nFailures);
	fflush(stdout);
	return g_nFailures == 0 ? 0 : 1;
}
