# Image Annotations Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let the user draw freehand strokes, type text labels and place rectangles over the displayed image, pick colour/opacity/width, and burn the result into the image file.

**Architecture:** Annotations are a vector model stored in original-image coordinates. One GDI+ renderer draws that model twice — scaled to the current zoom for the screen, and at 1:1 into the original pixel buffer on save. Pure logic (model, geometry, renderer) lives in files with no window dependency so it can be unit-tested by a new console test project; the input state machine talks to the main dialog through a narrow `IAnnotationHost` interface so it can be tested with a fake host.

**Tech Stack:** C++ / ATL / WTL, GDI+ (already initialised in `JPEGView.cpp` and linked), MSBuild, Visual Studio 2022 toolset v143 (v142 under VS2019).

**Spec:** `docs/superpowers/specs/2026-09-25-image-annotations-design.md`

## Global Constraints

- Toolset: `v142` when `$(VisualStudioVersion)=='16.0'`, otherwise `v143`. Copy the two `PlatformToolset` lines from `src/JPEGView/JPEGView.vcxproj` verbatim into any new project.
- Every new source file must be added to all four project files: `JPEGView.vcxproj`, `JPEGView.vcxproj.filters`, `JPEGView_VS2017.vcxproj`, `JPEGView_VS2017.vcxproj.filters`. New panels go under the `Panels` filter, the rest under `Source Files` / `Header Files`.
- `src/JPEGView/resource.h` uses **LF** line endings; every other source file uses **CRLF**. Check before editing. Run `git -c core.autocrlf=false` for commits, or set `git config core.autocrlf false` once in the clone.
- `Config/*_ru.txt` ini files are **CP1251**; `strings_ru.txt` is **UTF-8 with BOM**. Never let an editor add a BOM to an ini file — JPEGView silently ignores a BOM-prefixed ini.
- New command IDs occupy the free block **21000–21007** (between `IDM_CROPMODE_USER` 20900 and `IDM_FIRST_USER_CMD` 22000). Each gets the `// :KeyMap:` comment, which is what makes it user-assignable.
- Colour values in ini files are `R G B`, read with `CSettingsProvider::GetColor`.
- Sizes the user picks (pen width, font size) are **screen pixels**; they are divided by the current zoom when an annotation is created, and stored in image pixels.
- Testing the built binary must use `StoreToEXEPath=true` in a scratch folder. Never write to `%APPDATA%\JPEGView\JPEGView.ini` or `KeyMap.txt` — those are the user's personal files.

## Review Focus

Five things the spec implies but does not spell out, each most likely to bite a real user. The test that pins each one is attached to the task that owns the code.

1. **A zero-length stroke.** Click without dragging with the freehand tool: one point, no segment. Must not emit an invisible or degenerate annotation that still blocks navigation with a save prompt. — Task 1.
2. **A rectangle dragged right-to-left or bottom-to-top.** Negative width/height must normalise, not vanish or render inverted. — Task 2.
3. **Drawing outside the image.** With the image fitted to screen there are black borders; a stroke dragged into them must clamp to the image, not produce coordinates that write outside the pixel buffer on save. — Task 2.
4. **Empty text.** Clicking with the text tool and pressing Enter without typing must add nothing, rather than an empty annotation that marks the image dirty. — Task 10.
5. **Fully transparent colour.** Opacity 0 renders nothing; it must still round-trip through the settings write-back rather than being read back as the default. — Task 5.

---

### Task 1: Test project and the annotation model

The repository has no test project at all — the "Build Test" workflows only check that a Debug configuration compiles. This task creates a minimal console test runner and the first unit under test, so every later task has somewhere to put tests.

**Files:**
- Create: `src/JPEGViewTests/JPEGViewTests.vcxproj`
- Create: `src/JPEGViewTests/TestMain.cpp`
- Create: `src/JPEGViewTests/TestFramework.h`
- Create: `src/JPEGViewTests/TestAnnotationModel.cpp`
- Create: `src/JPEGView/AnnotationTypes.h`
- Create: `src/JPEGView/AnnotationModel.h`
- Create: `src/JPEGView/AnnotationModel.cpp`
- Create: `.github/workflows/build-unittests.yml`
- Modify: `src/JPEGView.sln`
- Modify: `src/JPEGView/JPEGView.vcxproj`, `.filters`, `JPEGView_VS2017.vcxproj`, `.filters`

**Interfaces:**
- Consumes: nothing.
- Produces: `struct CPointF { float x, y; }`; `enum EAnnotationType { AT_Freehand, AT_Text, AT_Rectangle }`; `struct CAnnotation`; `class CAnnotationModel` with `void Add(const CAnnotation&)`, `bool Undo()`, `bool Redo()`, `void Clear()`, `bool IsEmpty() const`, `size_t Count() const`, `const std::vector<CAnnotation>& Annotations() const`, `bool IsDirty() const`, `void MarkClean()`.

- [ ] **Step 1: Create the test framework header**

`src/JPEGViewTests/TestFramework.h`:

```cpp
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

#define CHECK_NEAR(actual, expected, tolerance) \
	do { \
		double a = (double)(actual), e = (double)(expected); \
		if (a < e - (tolerance) || a > e + (tolerance)) { \
			printf("FAIL %s:%d  %s: got %f, expected %f\n", __FILE__, __LINE__, #actual, a, e); \
			g_nFailures++; \
		} \
	} while (0)
```

- [ ] **Step 2: Create the test runner**

`src/JPEGViewTests/TestMain.cpp`:

```cpp
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
```

- [ ] **Step 3: Write the failing tests for the model**

`src/JPEGViewTests/TestAnnotationModel.cpp`. The last test is Review Focus item 1 — a click with no drag must not become a dirty-marking annotation.

```cpp
#include "TestFramework.h"
#include "AnnotationModel.h"

static CAnnotation MakeStroke(int nPoints) {
	CAnnotation a;
	a.eType = AT_Freehand;
	a.color = RGB(255, 0, 0);
	a.nAlpha = 180;
	a.fPenWidth = 4.0f;
	a.bFilled = false;
	for (int i = 0; i < nPoints; i++) {
		CPointF pt = { (float)i, (float)i };
		a.points.push_back(pt);
	}
	return a;
}

TEST(NewModelIsEmptyAndClean) {
	CAnnotationModel model;
	CHECK(model.IsEmpty());
	CHECK(model.Count() == 0);
	CHECK(!model.IsDirty());
}

TEST(AddMakesModelDirty) {
	CAnnotationModel model;
	model.Add(MakeStroke(5));
	CHECK(model.Count() == 1);
	CHECK(model.IsDirty());
}

TEST(UndoRemovesLastAnnotation) {
	CAnnotationModel model;
	model.Add(MakeStroke(5));
	model.Add(MakeStroke(7));
	CHECK(model.Undo());
	CHECK(model.Count() == 1);
	CHECK(model.Annotations()[0].points.size() == 5);
}

TEST(UndoOnEmptyModelReturnsFalse) {
	CAnnotationModel model;
	CHECK(!model.Undo());
}

TEST(RedoRestoresUndoneAnnotation) {
	CAnnotationModel model;
	model.Add(MakeStroke(5));
	model.Undo();
	CHECK(model.Redo());
	CHECK(model.Count() == 1);
	CHECK(model.Annotations()[0].points.size() == 5);
}

TEST(RedoOnEmptyStackReturnsFalse) {
	CAnnotationModel model;
	model.Add(MakeStroke(5));
	CHECK(!model.Redo());
}

TEST(AddClearsRedoStack) {
	CAnnotationModel model;
	model.Add(MakeStroke(5));
	model.Undo();
	model.Add(MakeStroke(9));
	CHECK(!model.Redo());
	CHECK(model.Count() == 1);
}

TEST(ClearRemovesEverythingIncludingRedo) {
	CAnnotationModel model;
	model.Add(MakeStroke(5));
	model.Add(MakeStroke(6));
	model.Undo();
	model.Clear();
	CHECK(model.IsEmpty());
	CHECK(!model.Redo());
}

TEST(MarkCleanClearsDirtyButKeepsAnnotations) {
	CAnnotationModel model;
	model.Add(MakeStroke(5));
	model.MarkClean();
	CHECK(!model.IsDirty());
	CHECK(model.Count() == 1);
}

TEST(UndoBackToStartLeavesModelClean) {
	CAnnotationModel model;
	model.Add(MakeStroke(5));
	model.Undo();
	CHECK(!model.IsDirty());
}

// Review Focus 1: a click without a drag produces a single point and must be rejected,
// otherwise an invisible annotation makes the image dirty and triggers the save prompt.
TEST(SinglePointStrokeIsRejected) {
	CAnnotationModel model;
	model.Add(MakeStroke(1));
	CHECK(model.IsEmpty());
	CHECK(!model.IsDirty());
}

TEST(EmptyPointListIsRejected) {
	CAnnotationModel model;
	model.Add(MakeStroke(0));
	CHECK(model.IsEmpty());
}
```

- [ ] **Step 4: Write the types header**

`src/JPEGView/AnnotationTypes.h`:

```cpp
#pragma once

#include <vector>

// A point in image coordinates. The codebase has CRectF in ZoomNavigator.h but no float
// point type; this one is defined here so the annotation model stays free of GDI+ types.
struct CPointF {
	float x, y;
};

enum EAnnotationType {
	AT_Freehand,
	AT_Text,
	AT_Rectangle
};

// One annotation element. Which fields matter depends on eType:
//   AT_Freehand  - points is a polyline of at least two points, fPenWidth is the stroke width
//   AT_Rectangle - points holds exactly two opposite corners, bFilled selects fill or outline
//   AT_Text      - points holds one anchor (top left of the text), sText and fFontHeight are used
// All coordinates and sizes are in pixels of the image as currently displayed.
struct CAnnotation {
	CAnnotation() : eType(AT_Freehand), color(0), nAlpha(255), fPenWidth(1.0f),
		bFilled(false), fFontHeight(12.0f) {}

	EAnnotationType      eType;
	COLORREF             color;
	int                  nAlpha;      // 0 .. 255
	float                fPenWidth;   // image pixels
	bool                 bFilled;     // rectangle only
	std::vector<CPointF> points;
	CString              sText;       // text only
	float                fFontHeight; // text only, image pixels
};
```

- [ ] **Step 5: Write the model header**

`src/JPEGView/AnnotationModel.h`:

```cpp
#pragma once

#include "AnnotationTypes.h"

// Holds the annotations of the currently displayed image, with undo/redo.
// Append-only: an element cannot be moved or edited once added, only undone.
// Knows nothing about windows, painting or the main dialog, so it is unit-testable.
class CAnnotationModel {
public:
	CAnnotationModel();

	// Adds an annotation. Degenerate elements are silently ignored: a freehand stroke
	// with fewer than two points, a rectangle without two corners, empty text.
	// Adding discards the redo stack.
	void Add(const CAnnotation& annotation);

	bool Undo(); // returns false if there was nothing to undo
	bool Redo(); // returns false if there was nothing to redo
	void Clear();

	bool IsEmpty() const { return m_annotations.empty(); }
	size_t Count() const { return m_annotations.size(); }
	const std::vector<CAnnotation>& Annotations() const { return m_annotations; }

	// True when the annotations differ from what was last saved into the image.
	bool IsDirty() const { return m_bDirty; }
	void MarkClean() { m_bDirty = false; }

private:
	static bool IsDegenerate(const CAnnotation& annotation);

	std::vector<CAnnotation> m_annotations;
	std::vector<CAnnotation> m_redoStack;
	bool m_bDirty;
};
```

- [ ] **Step 6: Write the model implementation**

`src/JPEGView/AnnotationModel.cpp`:

```cpp
#include "StdAfx.h"
#include "AnnotationModel.h"

CAnnotationModel::CAnnotationModel() {
	m_bDirty = false;
}

bool CAnnotationModel::IsDegenerate(const CAnnotation& annotation) {
	switch (annotation.eType) {
		case AT_Freehand:
			return annotation.points.size() < 2;
		case AT_Rectangle:
			return annotation.points.size() < 2;
		case AT_Text:
			return annotation.points.empty() || annotation.sText.IsEmpty();
	}
	return true;
}

void CAnnotationModel::Add(const CAnnotation& annotation) {
	if (IsDegenerate(annotation)) {
		return;
	}
	m_annotations.push_back(annotation);
	m_redoStack.clear();
	m_bDirty = true;
}

bool CAnnotationModel::Undo() {
	if (m_annotations.empty()) {
		return false;
	}
	m_redoStack.push_back(m_annotations.back());
	m_annotations.pop_back();
	m_bDirty = !m_annotations.empty();
	return true;
}

bool CAnnotationModel::Redo() {
	if (m_redoStack.empty()) {
		return false;
	}
	m_annotations.push_back(m_redoStack.back());
	m_redoStack.pop_back();
	m_bDirty = true;
	return true;
}

void CAnnotationModel::Clear() {
	bool bHadAnnotations = !m_annotations.empty();
	m_annotations.clear();
	m_redoStack.clear();
	if (bHadAnnotations) {
		m_bDirty = false;
	}
}
```

Note on `Undo` and dirty state: undoing back to an empty model leaves nothing to save, so the model reports clean and the save prompt will not appear. `Clear` does the same. This is what `UndoBackToStartLeavesModelClean` pins.

- [ ] **Step 7: Create the test project**

`src/JPEGViewTests/JPEGViewTests.vcxproj`: a Win32 console application (`<ConfigurationType>Application</ConfigurationType>`, `<SubSystem>Console</SubSystem>`), Debug|Win32, Debug|x64, Release|Win32, Release|x64. Copy the `PlatformToolset` pair from `src/JPEGView/JPEGView.vcxproj`:

```xml
<PlatformToolset Condition="'$(VisualStudioVersion)'=='16.0'">v142</PlatformToolset>
<PlatformToolset Condition="'$(PlatformToolset)'==''">v143</PlatformToolset>
```

Set `<AdditionalIncludeDirectories>..\JPEGView;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>`, `<UseOfMfc>false</UseOfMfc>`, and `<PreprocessorDefinitions>WIN32;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>`. Compile these items:

```xml
<ItemGroup>
  <ClCompile Include="TestMain.cpp" />
  <ClCompile Include="TestAnnotationModel.cpp" />
  <ClCompile Include="..\JPEGView\AnnotationModel.cpp" />
</ItemGroup>
<ItemGroup>
  <ClInclude Include="TestFramework.h" />
</ItemGroup>
```

`AnnotationModel.cpp` includes `StdAfx.h` and uses `CString`, so add `#include <atlstr.h>` at the top of `TestMain.cpp` is not enough — instead give the test project its own tiny `StdAfx.h` in `src/JPEGViewTests/` containing:

```cpp
#pragma once
#include <windows.h>
#include <atlbase.h>
#include <atlstr.h>
```

and put `src/JPEGViewTests` **before** `..\JPEGView` in `AdditionalIncludeDirectories` so `#include "StdAfx.h"` resolves to the test one. ATL is available in toolset v143 on the CI runner.

- [ ] **Step 8: Add the project to the solution**

Add a project entry for `JPEGViewTests\JPEGViewTests.vcxproj` to `src/JPEGView.sln` with a fresh GUID, and configuration rows for Debug|Win32, Debug|x64, Release|Win32, Release|x64 mirroring how `WICLoader` is listed. Leave `src/JPEGView_VS2017.sln` untouched — the test project is VS2022-only, which keeps the VS2017 path exactly as it is today.

- [ ] **Step 9: Run the tests to verify they fail**

Run: `msbuild src\JPEGView.sln /t:JPEGViewTests /p:Configuration=Debug /p:Platform=x64`
Expected: compiles (the model is written), then
Run: `src\bin\x64\Debug\JPEGViewTests.exe`
Expected: `12 test(s) run, 0 failure(s)`, exit code 0.

If the model were not yet written, the build would fail with "cannot open include file: 'AnnotationModel.h'" — write the tests first and confirm that failure before Step 4 if implementing strictly test-first.

- [ ] **Step 10: Add the CI workflow**

`.github/workflows/build-unittests.yml`, modelled on `.github/workflows/build-debug-x64.yml` but with a run step:

```yaml
name: Unit Tests (x64)

on: [push, pull_request]

jobs:
  test:
    runs-on: windows-2022
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive
      - uses: microsoft/setup-msbuild@v2
        with:
          vs-version: '[17.0,18.0)'
      - name: Build tests
        run: msbuild src\JPEGView.sln /t:JPEGViewTests /p:Configuration=Debug /p:Platform=x64 /m
      - name: Run tests
        run: src\bin\x64\Debug\JPEGViewTests.exe
```

Confirm the output path matches what the build actually produced; if `OutDir` differs, use the real path rather than assuming.

- [ ] **Step 11: Add the new JPEGView sources to the four project files**

Add `AnnotationTypes.h` and `AnnotationModel.h` under `Header Files`, `AnnotationModel.cpp` under `Source Files`, in `JPEGView.vcxproj`, `JPEGView.vcxproj.filters`, `JPEGView_VS2017.vcxproj`, `JPEGView_VS2017.vcxproj.filters`.

- [ ] **Step 12: Commit**

```bash
git add src/JPEGViewTests src/JPEGView/AnnotationTypes.h src/JPEGView/AnnotationModel.h src/JPEGView/AnnotationModel.cpp src/JPEGView.sln src/JPEGView/JPEGView.vcxproj src/JPEGView/JPEGView.vcxproj.filters src/JPEGView/JPEGView_VS2017.vcxproj src/JPEGView/JPEGView_VS2017.vcxproj.filters .github/workflows/build-unittests.yml
git commit -m "test: add unit test project and the annotation model"
```

---

### Task 2: Coordinate geometry

**Files:**
- Create: `src/JPEGView/AnnotationGeometry.h`
- Create: `src/JPEGView/AnnotationGeometry.cpp`
- Create: `src/JPEGViewTests/TestAnnotationGeometry.cpp`
- Modify: `src/JPEGViewTests/JPEGViewTests.vcxproj`, the four JPEGView project files

**Interfaces:**
- Consumes: `CPointF`, `CAnnotation` from Task 1.
- Produces: `namespace AnnotationGeometry` with `CPointF ScreenToImage(CPoint ptScreen, CPoint ptImageOrigin, float fZoom)`, `CPointF ImageToScreen(CPointF ptImage, CPoint ptImageOrigin, float fZoom)`, `CPointF ClampToImage(CPointF pt, CSize sizeImage)`, `void NormalizeRectangle(CAnnotation& annotation)`, `CRect BoundingBoxOnScreen(const CAnnotation& annotation, CPoint ptImageOrigin, float fZoom)`.

- [ ] **Step 1: Write the failing tests**

`src/JPEGViewTests/TestAnnotationGeometry.cpp`. Tests 4–6 are Review Focus items 2 and 3.

```cpp
#include "TestFramework.h"
#include "AnnotationGeometry.h"

TEST(ScreenToImageAtZoomOneIsATranslation) {
	CPointF pt = AnnotationGeometry::ScreenToImage(CPoint(150, 120), CPoint(100, 100), 1.0f);
	CHECK_NEAR(pt.x, 50.0, 0.001);
	CHECK_NEAR(pt.y, 20.0, 0.001);
}

TEST(ScreenToImageDividesByZoom) {
	CPointF pt = AnnotationGeometry::ScreenToImage(CPoint(300, 100), CPoint(100, 50), 2.0f);
	CHECK_NEAR(pt.x, 100.0, 0.001);
	CHECK_NEAR(pt.y, 25.0, 0.001);
}

TEST(ImageToScreenIsTheInverseOfScreenToImage) {
	CPoint ptOrigin(37, 91);
	float fZoom = 0.634f;
	CPointF ptImage = AnnotationGeometry::ScreenToImage(CPoint(512, 333), ptOrigin, fZoom);
	CPointF ptBack = AnnotationGeometry::ImageToScreen(ptImage, ptOrigin, fZoom);
	CHECK_NEAR(ptBack.x, 512.0, 0.01);
	CHECK_NEAR(ptBack.y, 333.0, 0.01);
}

// Review Focus 3: with the image fitted to screen there are black borders around it.
// A stroke dragged into them must clamp, or saving would write outside the pixel buffer.
TEST(ClampToImageKeepsPointsInsideBounds) {
	CSize sizeImage(800, 600);
	CPointF ptLow = AnnotationGeometry::ClampToImage(CPointF{ -30.0f, -5.0f }, sizeImage);
	CHECK_NEAR(ptLow.x, 0.0, 0.001);
	CHECK_NEAR(ptLow.y, 0.0, 0.001);
	CPointF ptHigh = AnnotationGeometry::ClampToImage(CPointF{ 9999.0f, 700.0f }, sizeImage);
	CHECK_NEAR(ptHigh.x, 799.0, 0.001);
	CHECK_NEAR(ptHigh.y, 599.0, 0.001);
}

TEST(ClampToImageLeavesInteriorPointsAlone) {
	CPointF pt = AnnotationGeometry::ClampToImage(CPointF{ 400.0f, 300.0f }, CSize(800, 600));
	CHECK_NEAR(pt.x, 400.0, 0.001);
	CHECK_NEAR(pt.y, 300.0, 0.001);
}

// Review Focus 2: dragging a rectangle right-to-left or bottom-to-top gives a negative
// width or height; it must normalise rather than render inverted or vanish.
TEST(NormalizeRectangleOrdersCorners) {
	CAnnotation a;
	a.eType = AT_Rectangle;
	a.points.push_back(CPointF{ 300.0f, 250.0f });
	a.points.push_back(CPointF{ 100.0f, 50.0f });
	AnnotationGeometry::NormalizeRectangle(a);
	CHECK_NEAR(a.points[0].x, 100.0, 0.001);
	CHECK_NEAR(a.points[0].y, 50.0, 0.001);
	CHECK_NEAR(a.points[1].x, 300.0, 0.001);
	CHECK_NEAR(a.points[1].y, 250.0, 0.001);
}

TEST(NormalizeRectangleLeavesAlreadyOrderedCornersAlone) {
	CAnnotation a;
	a.eType = AT_Rectangle;
	a.points.push_back(CPointF{ 10.0f, 20.0f });
	a.points.push_back(CPointF{ 30.0f, 40.0f });
	AnnotationGeometry::NormalizeRectangle(a);
	CHECK_NEAR(a.points[0].x, 10.0, 0.001);
	CHECK_NEAR(a.points[1].y, 40.0, 0.001);
}

TEST(BoundingBoxCoversTheStrokePlusPenWidth) {
	CAnnotation a;
	a.eType = AT_Freehand;
	a.fPenWidth = 10.0f;
	a.points.push_back(CPointF{ 100.0f, 100.0f });
	a.points.push_back(CPointF{ 200.0f, 150.0f });
	CRect rect = AnnotationGeometry::BoundingBoxOnScreen(a, CPoint(0, 0), 1.0f);
	CHECK(rect.left <= 95);
	CHECK(rect.top <= 95);
	CHECK(rect.right >= 205);
	CHECK(rect.bottom >= 155);
}
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `msbuild src\JPEGView.sln /t:JPEGViewTests /p:Configuration=Debug /p:Platform=x64`
Expected: FAIL with "Cannot open include file: 'AnnotationGeometry.h'".

- [ ] **Step 3: Write the implementation**

`src/JPEGView/AnnotationGeometry.h`:

```cpp
#pragma once

#include "AnnotationTypes.h"

// Conversions between screen pixels and image pixels for annotations.
// ptImageOrigin is the screen position of the image's top left corner, i.e. what
// HelpersGUI::DrawDIB32bppWithBlackBorders returns; fZoom is the realized zoom.
namespace AnnotationGeometry {
	CPointF ScreenToImage(CPoint ptScreen, CPoint ptImageOrigin, float fZoom);
	CPointF ImageToScreen(CPointF ptImage, CPoint ptImageOrigin, float fZoom);

	// Clamps to [0, width-1] x [0, height-1] so a drag into the black borders
	// cannot produce coordinates outside the pixel buffer.
	CPointF ClampToImage(CPointF pt, CSize sizeImage);

	// Orders the two corners of a rectangle annotation so that points[0] is top left.
	void NormalizeRectangle(CAnnotation& annotation);

	// Screen rectangle the annotation paints into, inflated by the pen width, for Invalidate.
	CRect BoundingBoxOnScreen(const CAnnotation& annotation, CPoint ptImageOrigin, float fZoom);
}
```

`src/JPEGView/AnnotationGeometry.cpp`:

```cpp
#include "StdAfx.h"
#include "AnnotationGeometry.h"
#include <math.h>

namespace AnnotationGeometry {

CPointF ScreenToImage(CPoint ptScreen, CPoint ptImageOrigin, float fZoom) {
	CPointF pt = { (ptScreen.x - ptImageOrigin.x) / fZoom, (ptScreen.y - ptImageOrigin.y) / fZoom };
	return pt;
}

CPointF ImageToScreen(CPointF ptImage, CPoint ptImageOrigin, float fZoom) {
	CPointF pt = { ptImage.x * fZoom + ptImageOrigin.x, ptImage.y * fZoom + ptImageOrigin.y };
	return pt;
}

CPointF ClampToImage(CPointF pt, CSize sizeImage) {
	float fMaxX = (float)max(0, sizeImage.cx - 1);
	float fMaxY = (float)max(0, sizeImage.cy - 1);
	CPointF ptClamped = { min(max(0.0f, pt.x), fMaxX), min(max(0.0f, pt.y), fMaxY) };
	return ptClamped;
}

void NormalizeRectangle(CAnnotation& annotation) {
	if (annotation.eType != AT_Rectangle || annotation.points.size() < 2) {
		return;
	}
	float fLeft = min(annotation.points[0].x, annotation.points[1].x);
	float fTop = min(annotation.points[0].y, annotation.points[1].y);
	float fRight = max(annotation.points[0].x, annotation.points[1].x);
	float fBottom = max(annotation.points[0].y, annotation.points[1].y);
	annotation.points[0].x = fLeft;
	annotation.points[0].y = fTop;
	annotation.points[1].x = fRight;
	annotation.points[1].y = fBottom;
}

CRect BoundingBoxOnScreen(const CAnnotation& annotation, CPoint ptImageOrigin, float fZoom) {
	if (annotation.points.empty()) {
		return CRect(0, 0, 0, 0);
	}
	float fLeft = annotation.points[0].x, fRight = annotation.points[0].x;
	float fTop = annotation.points[0].y, fBottom = annotation.points[0].y;
	for (size_t i = 1; i < annotation.points.size(); i++) {
		fLeft = min(fLeft, annotation.points[i].x);
		fRight = max(fRight, annotation.points[i].x);
		fTop = min(fTop, annotation.points[i].y);
		fBottom = max(fBottom, annotation.points[i].y);
	}
	// Text grows right and down from its anchor; the width is not known here, so use a
	// generous multiple of the font height. Over-invalidating only costs a repaint.
	if (annotation.eType == AT_Text) {
		fRight += annotation.fFontHeight * 40.0f;
		fBottom += annotation.fFontHeight * 1.5f;
	}
	float fMargin = max(annotation.fPenWidth, 1.0f);
	CPointF ptTopLeft = ImageToScreen(CPointF{ fLeft - fMargin, fTop - fMargin }, ptImageOrigin, fZoom);
	CPointF ptBottomRight = ImageToScreen(CPointF{ fRight + fMargin, fBottom + fMargin }, ptImageOrigin, fZoom);
	return CRect((int)floor(ptTopLeft.x), (int)floor(ptTopLeft.y),
		(int)ceil(ptBottomRight.x) + 1, (int)ceil(ptBottomRight.y) + 1);
}

}
```

- [ ] **Step 4: Add to the projects and run the tests**

Add `AnnotationGeometry.cpp` to `JPEGViewTests.vcxproj` and the four JPEGView project files.
Run: `msbuild src\JPEGView.sln /t:JPEGViewTests /p:Configuration=Debug /p:Platform=x64` then `src\bin\x64\Debug\JPEGViewTests.exe`
Expected: `20 test(s) run, 0 failure(s)`.

- [ ] **Step 5: Commit**

```bash
git add src/JPEGView/AnnotationGeometry.h src/JPEGView/AnnotationGeometry.cpp src/JPEGViewTests/TestAnnotationGeometry.cpp src/JPEGViewTests/JPEGViewTests.vcxproj src/JPEGView/JPEGView.vcxproj src/JPEGView/JPEGView.vcxproj.filters src/JPEGView/JPEGView_VS2017.vcxproj src/JPEGView/JPEGView_VS2017.vcxproj.filters
git commit -m "feat: add annotation coordinate geometry"
```

---

### Task 3: The renderer

**Files:**
- Create: `src/JPEGView/AnnotationRenderer.h`
- Create: `src/JPEGView/AnnotationRenderer.cpp`
- Create: `src/JPEGViewTests/TestAnnotationRenderer.cpp`
- Modify: `src/JPEGViewTests/JPEGViewTests.vcxproj`, the four JPEGView project files

**Interfaces:**
- Consumes: `CAnnotation` (Task 1), `AnnotationGeometry::ImageToScreen` (Task 2).
- Produces: `class CAnnotationRenderer` with `static void Render(Gdiplus::Graphics& g, const std::vector<CAnnotation>& annotations, float fScale, const Gdiplus::PointF& ptOrigin)` and `static void RenderOne(Gdiplus::Graphics& g, const CAnnotation& annotation, float fScale, const Gdiplus::PointF& ptOrigin)`.

GDI+ works without a window, so these tests render into an in-memory `Gdiplus::Bitmap` and assert pixel colours. `Gdiplus::GdiplusStartup` must be called once in the test runner.

- [ ] **Step 1: Initialise GDI+ in the test runner**

Modify `src/JPEGViewTests/TestMain.cpp` `main()`:

```cpp
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

int main() {
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	std::vector<CTestCase>& tests = AllTests();
	for (size_t i = 0; i < tests.size(); i++) {
		printf("[ RUN ] %s\n", tests[i].sName);
		tests[i].pFunction();
	}
	printf("\n%d test(s) run, %d failure(s)\n", (int)tests.size(), g_nFailures);

	Gdiplus::GdiplusShutdown(gdiplusToken);
	return g_nFailures == 0 ? 0 : 1;
}
```

- [ ] **Step 2: Write the failing tests**

`src/JPEGViewTests/TestAnnotationRenderer.cpp`. The last test is Review Focus item 5.

```cpp
#include "TestFramework.h"
#include "AnnotationRenderer.h"
#include <gdiplus.h>

// Renders into a white 200x200 bitmap and returns the colour at (x, y).
static Gdiplus::Color RenderAndSample(const CAnnotation& annotation, float fScale, int x, int y) {
	Gdiplus::Bitmap bitmap(200, 200, PixelFormat32bppARGB);
	Gdiplus::Graphics g(&bitmap);
	g.Clear(Gdiplus::Color(255, 255, 255, 255));
	std::vector<CAnnotation> annotations;
	annotations.push_back(annotation);
	CAnnotationRenderer::Render(g, annotations, fScale, Gdiplus::PointF(0.0f, 0.0f));
	Gdiplus::Color color;
	bitmap.GetPixel(x, y, &color);
	return color;
}

static CAnnotation MakeFilledRect(int nAlpha) {
	CAnnotation a;
	a.eType = AT_Rectangle;
	a.bFilled = true;
	a.color = RGB(255, 0, 0);
	a.nAlpha = nAlpha;
	a.fPenWidth = 2.0f;
	a.points.push_back(CPointF{ 50.0f, 50.0f });
	a.points.push_back(CPointF{ 150.0f, 150.0f });
	return a;
}

TEST(OpaqueFilledRectanglePaintsItsColourInside) {
	Gdiplus::Color color = RenderAndSample(MakeFilledRect(255), 1.0f, 100, 100);
	CHECK(color.GetR() == 255);
	CHECK(color.GetG() == 0);
	CHECK(color.GetB() == 0);
}

TEST(FilledRectangleLeavesTheOutsideUntouched) {
	Gdiplus::Color color = RenderAndSample(MakeFilledRect(255), 1.0f, 10, 10);
	CHECK(color.GetR() == 255);
	CHECK(color.GetG() == 255);
	CHECK(color.GetB() == 255);
}

TEST(HalfTransparentFillBlendsWithTheBackground) {
	// Red at alpha 128 over white gives roughly (255, 127, 127).
	Gdiplus::Color color = RenderAndSample(MakeFilledRect(128), 1.0f, 100, 100);
	CHECK(color.GetR() > 240);
	CHECK(color.GetG() > 110 && color.GetG() < 145);
	CHECK(color.GetB() > 110 && color.GetB() < 145);
}

TEST(OutlineRectangleLeavesItsInteriorUntouched) {
	CAnnotation a = MakeFilledRect(255);
	a.bFilled = false;
	Gdiplus::Color color = RenderAndSample(a, 1.0f, 100, 100);
	CHECK(color.GetR() == 255);
	CHECK(color.GetG() == 255);
	CHECK(color.GetB() == 255);
}

TEST(OutlineRectanglePaintsItsBorder) {
	CAnnotation a = MakeFilledRect(255);
	a.bFilled = false;
	a.fPenWidth = 6.0f;
	Gdiplus::Color color = RenderAndSample(a, 1.0f, 50, 100);
	CHECK(color.GetR() == 255);
	CHECK(color.GetG() < 60);
}

TEST(ScaleMovesGeometryProportionally) {
	// At scale 0.5 the rectangle spans image 50..150 -> screen 25..75, so (100,100) is outside.
	Gdiplus::Color inside = RenderAndSample(MakeFilledRect(255), 0.5f, 50, 50);
	CHECK(inside.GetG() == 0);
	Gdiplus::Color outside = RenderAndSample(MakeFilledRect(255), 0.5f, 100, 100);
	CHECK(outside.GetG() == 255);
}

TEST(FreehandStrokePaintsAlongItsPath) {
	CAnnotation a;
	a.eType = AT_Freehand;
	a.color = RGB(0, 0, 255);
	a.nAlpha = 255;
	a.fPenWidth = 8.0f;
	a.points.push_back(CPointF{ 20.0f, 100.0f });
	a.points.push_back(CPointF{ 180.0f, 100.0f });
	Gdiplus::Color color = RenderAndSample(a, 1.0f, 100, 100);
	CHECK(color.GetB() == 255);
	CHECK(color.GetR() < 60);
}

// Review Focus 5: opacity 0 must render nothing at all rather than a faint mark.
TEST(ZeroAlphaRendersNothing) {
	Gdiplus::Color color = RenderAndSample(MakeFilledRect(0), 1.0f, 100, 100);
	CHECK(color.GetR() == 255);
	CHECK(color.GetG() == 255);
	CHECK(color.GetB() == 255);
}

TEST(EmptyAnnotationListRendersNothing) {
	Gdiplus::Bitmap bitmap(200, 200, PixelFormat32bppARGB);
	Gdiplus::Graphics g(&bitmap);
	g.Clear(Gdiplus::Color(255, 255, 255, 255));
	std::vector<CAnnotation> annotations;
	CAnnotationRenderer::Render(g, annotations, 1.0f, Gdiplus::PointF(0.0f, 0.0f));
	Gdiplus::Color color;
	bitmap.GetPixel(100, 100, &color);
	CHECK(color.GetG() == 255);
}
```

- [ ] **Step 3: Run the tests to verify they fail**

Run: `msbuild src\JPEGView.sln /t:JPEGViewTests /p:Configuration=Debug /p:Platform=x64`
Expected: FAIL with "Cannot open include file: 'AnnotationRenderer.h'".

- [ ] **Step 4: Write the implementation**

`src/JPEGView/AnnotationRenderer.h`:

```cpp
#pragma once

#include "AnnotationTypes.h"
#include <gdiplus.h>

// The only place annotations are turned into pixels. Called twice:
// from CMainDlg::OnPaint with the realized zoom as fScale and the image origin as ptOrigin,
// and from CJPEGImage::ApplyAnnotationsToOriginalPixels with fScale 1 and origin (0, 0).
class CAnnotationRenderer {
public:
	static void Render(Gdiplus::Graphics& g, const std::vector<CAnnotation>& annotations,
		float fScale, const Gdiplus::PointF& ptOrigin);
	static void RenderOne(Gdiplus::Graphics& g, const CAnnotation& annotation,
		float fScale, const Gdiplus::PointF& ptOrigin);

private:
	static Gdiplus::PointF Transform(const CPointF& pt, float fScale, const Gdiplus::PointF& ptOrigin);
	static Gdiplus::Color ToColor(const CAnnotation& annotation);
};
```

`src/JPEGView/AnnotationRenderer.cpp`:

```cpp
#include "StdAfx.h"
#include "AnnotationRenderer.h"

Gdiplus::PointF CAnnotationRenderer::Transform(const CPointF& pt, float fScale, const Gdiplus::PointF& ptOrigin) {
	return Gdiplus::PointF(pt.x * fScale + ptOrigin.X, pt.y * fScale + ptOrigin.Y);
}

Gdiplus::Color CAnnotationRenderer::ToColor(const CAnnotation& annotation) {
	return Gdiplus::Color((BYTE)annotation.nAlpha, GetRValue(annotation.color),
		GetGValue(annotation.color), GetBValue(annotation.color));
}

void CAnnotationRenderer::Render(Gdiplus::Graphics& g, const std::vector<CAnnotation>& annotations,
		float fScale, const Gdiplus::PointF& ptOrigin) {
	Gdiplus::SmoothingMode oldSmoothing = g.GetSmoothingMode();
	Gdiplus::TextRenderingHint oldHint = g.GetTextRenderingHint();
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
	for (size_t i = 0; i < annotations.size(); i++) {
		RenderOne(g, annotations[i], fScale, ptOrigin);
	}
	g.SetTextRenderingHint(oldHint);
	g.SetSmoothingMode(oldSmoothing);
}

void CAnnotationRenderer::RenderOne(Gdiplus::Graphics& g, const CAnnotation& annotation,
		float fScale, const Gdiplus::PointF& ptOrigin) {
	if (annotation.nAlpha <= 0 || annotation.points.empty()) {
		return; // fully transparent draws nothing at all
	}
	Gdiplus::Color color = ToColor(annotation);

	switch (annotation.eType) {
		case AT_Freehand: {
			if (annotation.points.size() < 2) {
				return;
			}
			Gdiplus::Pen pen(color, max(1.0f, annotation.fPenWidth * fScale));
			pen.SetStartCap(Gdiplus::LineCapRound);
			pen.SetEndCap(Gdiplus::LineCapRound);
			pen.SetLineJoin(Gdiplus::LineJoinRound);
			std::vector<Gdiplus::PointF> pts;
			pts.reserve(annotation.points.size());
			for (size_t i = 0; i < annotation.points.size(); i++) {
				pts.push_back(Transform(annotation.points[i], fScale, ptOrigin));
			}
			// One DrawLines call, so overlapping segments of a translucent stroke
			// are composited once rather than blending on top of each other.
			g.DrawLines(&pen, &pts[0], (INT)pts.size());
			break;
		}
		case AT_Rectangle: {
			if (annotation.points.size() < 2) {
				return;
			}
			Gdiplus::PointF ptTopLeft = Transform(annotation.points[0], fScale, ptOrigin);
			Gdiplus::PointF ptBottomRight = Transform(annotation.points[1], fScale, ptOrigin);
			Gdiplus::RectF rect(ptTopLeft.X, ptTopLeft.Y,
				ptBottomRight.X - ptTopLeft.X, ptBottomRight.Y - ptTopLeft.Y);
			if (annotation.bFilled) {
				Gdiplus::SolidBrush brush(color);
				g.FillRectangle(&brush, rect);
			} else {
				Gdiplus::Pen pen(color, max(1.0f, annotation.fPenWidth * fScale));
				g.DrawRectangle(&pen, rect);
			}
			break;
		}
		case AT_Text: {
			if (annotation.sText.IsEmpty()) {
				return;
			}
			Gdiplus::PointF ptAnchor = Transform(annotation.points[0], fScale, ptOrigin);
			Gdiplus::FontFamily fontFamily(L"Segoe UI");
			Gdiplus::Font font(&fontFamily, max(1.0f, annotation.fFontHeight * fScale),
				Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
			Gdiplus::SolidBrush brush(color);
			g.DrawString(annotation.sText, -1, &font, ptAnchor, &brush);
			break;
		}
	}
}
```

Note: `DrawRectangle` with a negative width or height draws nothing, which is exactly why `AnnotationGeometry::NormalizeRectangle` must run before a rectangle is added to the model. Task 5 does that.

- [ ] **Step 5: Add to the projects and run the tests**

Add `AnnotationRenderer.cpp` and `TestAnnotationRenderer.cpp` to `JPEGViewTests.vcxproj`, and `AnnotationRenderer.{h,cpp}` to the four JPEGView project files.
Run: `msbuild src\JPEGView.sln /t:JPEGViewTests /p:Configuration=Debug /p:Platform=x64` then `src\bin\x64\Debug\JPEGViewTests.exe`
Expected: `29 test(s) run, 0 failure(s)`.

- [ ] **Step 6: Commit**

```bash
git add src/JPEGView/AnnotationRenderer.h src/JPEGView/AnnotationRenderer.cpp src/JPEGViewTests src/JPEGView/JPEGView.vcxproj src/JPEGView/JPEGView.vcxproj.filters src/JPEGView/JPEGView_VS2017.vcxproj src/JPEGView/JPEGView_VS2017.vcxproj.filters
git commit -m "feat: add the GDI+ annotation renderer"
```

---

### Task 4: Commands, keymap, menu and help

Delivered before the UI so that later tasks have IDs to bind to. Ends with the commands present and doing nothing yet, which builds and runs unchanged.

**Files:**
- Modify: `src/JPEGView/resource.h` (LF file)
- Modify: `src/JPEGView/Config/symbols.km`
- Modify: `src/JPEGView/Config/KeyMap.txt.default`, `KeyMap_ru.txt.default`
- Modify: `src/JPEGView/JPEGView.rc`
- Modify: `src/JPEGView/Config/strings_ru.txt` (UTF-8 BOM)

**Interfaces:**
- Produces: the nine `IDM_ANNOTATE_*` constants used by Tasks 5–11.

- [ ] **Step 1: Add the command IDs**

Append to `src/JPEGView/resource.h`, after `#define IDM_CROPMODE_USER 20900`. Keep LF line endings and the tab before the comment:

```
#define IDM_ANNOTATE_FREEHAND 21000		// :KeyMap: select the freehand annotation tool
#define IDM_ANNOTATE_TEXT   21001		// :KeyMap: select the text annotation tool
#define IDM_ANNOTATE_RECT   21002		// :KeyMap: select the rectangle annotation tool, repeat to toggle fill
#define IDM_ANNOTATE_UNDO   21003		// :KeyMap: undo the last annotation
#define IDM_ANNOTATE_REDO   21004		// :KeyMap: redo the last undone annotation
#define IDM_ANNOTATE_CLEAR  21005		// :KeyMap: remove all annotations from the current image
#define IDM_ANNOTATE_OFF    21006		// :KeyMap: leave annotation mode
#define IDM_ANNOTATE_APPLY_SAVE 21007		// :KeyMap: burn the annotations into the image and save it
```

- [ ] **Step 2: Add the symbols**

Add the same nine names to `src/JPEGView/Config/symbols.km` in the format that file already uses for `IDM_TRANSPARENT_TITLE_BAR`.

- [ ] **Step 3: Add the default key bindings**

These keys were checked against the current `KeyMap.txt.default` and are unused (`Ctrl+Shift+S` is taken by `IDM_SAVE_SCREEN`, so apply-and-save uses `Ctrl+Shift+A`). The command name starts at column 25.

```
Ctrl+Shift+D            IDM_ANNOTATE_FREEHAND
Ctrl+Shift+T            IDM_ANNOTATE_TEXT
Ctrl+Shift+R            IDM_ANNOTATE_RECT
Ctrl+Z                  IDM_ANNOTATE_UNDO
Ctrl+Y                  IDM_ANNOTATE_REDO
Ctrl+Shift+Del          IDM_ANNOTATE_CLEAR
Ctrl+Shift+A            IDM_ANNOTATE_APPLY_SAVE
```

`IDM_ANNOTATE_OFF` gets **no** default binding: Esc is not in the keymap at all and is handled directly in `CMainDlg::OnKeyDown` (Task 6, Step 4), next to the existing Esc cases for the help dialog and cropping. Add the same seven lines to `KeyMap_ru.txt.default`.

- [ ] **Step 4: Add the menu items**

In `src/JPEGView/JPEGView.rc`, add a popup submenu to the context menu, placed after the existing crop-related entries:

```
POPUP "Annotations"
BEGIN
    MENUITEM "Freehand",                    IDM_ANNOTATE_FREEHAND
    MENUITEM "Text",                        IDM_ANNOTATE_TEXT
    MENUITEM "Rectangle",                   IDM_ANNOTATE_RECT
    MENUITEM SEPARATOR
    MENUITEM "Undo annotation",             IDM_ANNOTATE_UNDO
    MENUITEM "Redo annotation",             IDM_ANNOTATE_REDO
    MENUITEM "Clear annotations",           IDM_ANNOTATE_CLEAR
    MENUITEM SEPARATOR
    MENUITEM "Save image with annotations", IDM_ANNOTATE_APPLY_SAVE
END
```

- [ ] **Step 5: Add the Russian strings**

Append to `src/JPEGView/Config/strings_ru.txt`, preserving its UTF-8 BOM and its existing `msgid`/`msgstr`-style pairing. The English strings are the keys, so add one entry per string introduced above plus the tooltips Task 7 will use:

`Annotations`, `Freehand`, `Text`, `Rectangle`, `Undo annotation`, `Redo annotation`, `Clear annotations`, `Save image with annotations`, `Draw freehand`, `Add text`, `Draw rectangle`, `Annotation colour, opacity and width`, `Line width`, `Font size`, `Opacity`, `Other colour...`.

- [ ] **Step 6: Build and verify nothing regressed**

Run: `msbuild src\JPEGView.sln /t:JPEGView /p:Configuration=Release /p:Platform=x64`
Expected: build succeeds. The new menu entries appear but do nothing yet.

- [ ] **Step 7: Commit**

```bash
git add src/JPEGView/resource.h src/JPEGView/JPEGView.rc src/JPEGView/Config
git commit -m "feat: add annotation command IDs, key bindings and menu entries"
```

---

### Task 5: The input state machine

**Files:**
- Create: `src/JPEGView/AnnotationCtl.h`
- Create: `src/JPEGView/AnnotationCtl.cpp`
- Create: `src/JPEGViewTests/TestAnnotationCtl.cpp`
- Modify: `src/JPEGViewTests/JPEGViewTests.vcxproj`, the four JPEGView project files

**Interfaces:**
- Consumes: `CAnnotationModel` (Task 1), `AnnotationGeometry` (Task 2), the `IDM_ANNOTATE_*` IDs (Task 4).
- Produces: `class IAnnotationHost` with `virtual CPoint GetImageOrigin() = 0`, `virtual float GetRealizedZoom() = 0`, `virtual CSize GetImageSize() = 0`, `virtual void InvalidateScreenRect(const CRect& rect) = 0`; `enum EAnnotationTool { ATOOL_None, ATOOL_Freehand, ATOOL_Text, ATOOL_Rectangle }`; `class CAnnotationCtl` with `CAnnotationCtl(IAnnotationHost* pHost)`, `void SetTool(EAnnotationTool)`, `EAnnotationTool GetTool() const`, `bool IsRectangleFilled() const`, `bool IsAnnotating() const`, `bool HasUnsavedAnnotations() const`, `bool OnLButtonDown(int nX, int nY)`, `bool OnMouseMove(int nX, int nY)`, `bool OnLButtonUp(int nX, int nY)`, `void CommitText(LPCTSTR sText)`, `CPoint GetPendingTextPosition() const`, `void Undo()`, `void Redo()`, `void Clear()`, `void MarkSaved()`, `CAnnotationModel& Model()`, `void SetStyle(COLORREF, int nAlpha, int nPenWidthScreen, int nFontSizeScreen)`, `COLORREF GetColor() const`, `int GetAlpha() const`, `int GetPenWidthScreen() const`, `int GetFontSizeScreen() const`.

The host interface is what makes this testable: the tests supply a fake host instead of `CMainDlg`.

- [ ] **Step 1: Write the failing tests**

`src/JPEGViewTests/TestAnnotationCtl.cpp`:

```cpp
#include "TestFramework.h"
#include "AnnotationCtl.h"

class CFakeHost : public IAnnotationHost {
public:
	CFakeHost() : m_ptOrigin(0, 0), m_fZoom(1.0f), m_sizeImage(800, 600), m_nInvalidateCount(0) {}
	virtual CPoint GetImageOrigin() { return m_ptOrigin; }
	virtual float GetRealizedZoom() { return m_fZoom; }
	virtual CSize GetImageSize() { return m_sizeImage; }
	virtual void InvalidateScreenRect(const CRect& rect) { m_nInvalidateCount++; }

	CPoint m_ptOrigin;
	float m_fZoom;
	CSize m_sizeImage;
	int m_nInvalidateCount;
};

TEST(NoToolMeansMouseIsNotConsumed) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	CHECK(!ctl.IsAnnotating());
	CHECK(!ctl.OnLButtonDown(100, 100));
	CHECK(!ctl.OnMouseMove(110, 110));
	CHECK(!ctl.OnLButtonUp(110, 110));
}

TEST(FreehandDragAddsOneStroke) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Freehand);
	CHECK(ctl.IsAnnotating());
	CHECK(ctl.OnLButtonDown(100, 100));
	CHECK(ctl.OnMouseMove(120, 110));
	CHECK(ctl.OnMouseMove(140, 130));
	CHECK(ctl.OnLButtonUp(140, 130));
	CHECK(ctl.Model().Count() == 1);
	CHECK(ctl.Model().Annotations()[0].eType == AT_Freehand);
	CHECK(ctl.Model().Annotations()[0].points.size() == 3);
}

TEST(FreehandClickWithoutDragAddsNothing) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Freehand);
	ctl.OnLButtonDown(100, 100);
	ctl.OnLButtonUp(100, 100);
	CHECK(ctl.Model().IsEmpty());
	CHECK(!ctl.HasUnsavedAnnotations());
}

TEST(FreehandStoresImageCoordinatesNotScreenCoordinates) {
	CFakeHost host;
	host.m_ptOrigin = CPoint(40, 20);
	host.m_fZoom = 2.0f;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Freehand);
	ctl.OnLButtonDown(240, 220);
	ctl.OnMouseMove(440, 420);
	ctl.OnLButtonUp(440, 420);
	const CAnnotation& a = ctl.Model().Annotations()[0];
	CHECK_NEAR(a.points[0].x, 100.0, 0.01);
	CHECK_NEAR(a.points[0].y, 100.0, 0.01);
	CHECK_NEAR(a.points[1].x, 200.0, 0.01);
	CHECK_NEAR(a.points[1].y, 200.0, 0.01);
}

TEST(PenWidthIsStoredInImagePixels) {
	CFakeHost host;
	host.m_fZoom = 4.0f;
	CAnnotationCtl ctl(&host);
	ctl.SetStyle(RGB(255, 0, 0), 200, 8, 24);
	ctl.SetTool(ATOOL_Freehand);
	ctl.OnLButtonDown(0, 0);
	ctl.OnMouseMove(40, 40);
	ctl.OnLButtonUp(40, 40);
	// 8 screen pixels at zoom 4 is 2 image pixels
	CHECK_NEAR(ctl.Model().Annotations()[0].fPenWidth, 2.0, 0.01);
}

TEST(StrokeIsClampedToTheImage) {
	CFakeHost host;
	host.m_sizeImage = CSize(200, 200);
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Freehand);
	ctl.OnLButtonDown(-50, -50);
	ctl.OnMouseMove(5000, 5000);
	ctl.OnLButtonUp(5000, 5000);
	const CAnnotation& a = ctl.Model().Annotations()[0];
	CHECK(a.points[0].x >= 0.0f && a.points[0].y >= 0.0f);
	CHECK(a.points[1].x <= 199.0f && a.points[1].y <= 199.0f);
}

TEST(RectangleDragAddsANormalizedRectangle) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Rectangle);
	ctl.OnLButtonDown(300, 250);
	ctl.OnMouseMove(100, 50);
	ctl.OnLButtonUp(100, 50);
	CHECK(ctl.Model().Count() == 1);
	const CAnnotation& a = ctl.Model().Annotations()[0];
	CHECK(a.eType == AT_Rectangle);
	CHECK(a.points[0].x < a.points[1].x);
	CHECK(a.points[0].y < a.points[1].y);
}

TEST(RepeatSelectionOfRectangleTogglesFill) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Rectangle);
	CHECK(!ctl.IsRectangleFilled());
	ctl.SetTool(ATOOL_Rectangle);
	CHECK(ctl.IsRectangleFilled());
	ctl.SetTool(ATOOL_Rectangle);
	CHECK(!ctl.IsRectangleFilled());
}

TEST(SwitchingAwayAndBackResetsFillToOutline) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Rectangle);
	ctl.SetTool(ATOOL_Rectangle);
	CHECK(ctl.IsRectangleFilled());
	ctl.SetTool(ATOOL_Freehand);
	ctl.SetTool(ATOOL_Rectangle);
	CHECK(!ctl.IsRectangleFilled());
}

TEST(TextClickRecordsAPendingPositionAndConsumesTheClick) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Text);
	CHECK(ctl.OnLButtonDown(120, 140));
	CHECK(ctl.GetPendingTextPosition() == CPoint(120, 140));
	CHECK(ctl.Model().IsEmpty());
}

TEST(CommitTextAddsATextAnnotation) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetStyle(RGB(0, 255, 0), 255, 4, 30);
	ctl.SetTool(ATOOL_Text);
	ctl.OnLButtonDown(100, 100);
	ctl.CommitText(_T("hello"));
	CHECK(ctl.Model().Count() == 1);
	const CAnnotation& a = ctl.Model().Annotations()[0];
	CHECK(a.eType == AT_Text);
	CHECK(a.sText == CString(_T("hello")));
	CHECK_NEAR(a.fFontHeight, 30.0, 0.01);
}

// Review Focus 4: committing empty text must add nothing and leave the image clean.
TEST(CommitEmptyTextAddsNothing) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Text);
	ctl.OnLButtonDown(100, 100);
	ctl.CommitText(_T(""));
	CHECK(ctl.Model().IsEmpty());
	CHECK(!ctl.HasUnsavedAnnotations());
}

TEST(SetToolNoneLeavesAnnotationMode) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Freehand);
	ctl.SetTool(ATOOL_None);
	CHECK(!ctl.IsAnnotating());
	CHECK(!ctl.OnLButtonDown(10, 10));
}

TEST(LeavingAnnotationModeKeepsTheAnnotations) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Freehand);
	ctl.OnLButtonDown(10, 10);
	ctl.OnMouseMove(50, 50);
	ctl.OnLButtonUp(50, 50);
	ctl.SetTool(ATOOL_None);
	CHECK(ctl.Model().Count() == 1);
	CHECK(ctl.HasUnsavedAnnotations());
}

TEST(MarkSavedClearsTheUnsavedFlagButKeepsAnnotations) {
	CFakeHost host;
	CAnnotationCtl ctl(&host);
	ctl.SetTool(ATOOL_Rectangle);
	ctl.OnLButtonDown(10, 10);
	ctl.OnMouseMove(50, 50);
	ctl.OnLButtonUp(50, 50);
	ctl.MarkSaved();
	CHECK(!ctl.HasUnsavedAnnotations());
	CHECK(ctl.Model().Count() == 1);
}
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `msbuild src\JPEGView.sln /t:JPEGViewTests /p:Configuration=Debug /p:Platform=x64`
Expected: FAIL with "Cannot open include file: 'AnnotationCtl.h'".

- [ ] **Step 3: Write the header**

`src/JPEGView/AnnotationCtl.h`:

```cpp
#pragma once

#include "AnnotationModel.h"

// What the annotation controller needs from the window showing the image.
// CMainDlg implements this; the unit tests supply a fake.
class IAnnotationHost {
public:
	virtual ~IAnnotationHost() {}
	virtual CPoint GetImageOrigin() = 0;   // screen position of the image's top left corner
	virtual float GetRealizedZoom() = 0;   // image pixels to screen pixels factor
	virtual CSize GetImageSize() = 0;      // size of the image as currently displayed
	// Named InvalidateScreenRect, not InvalidateRect: CMainDlg implements this interface and
	// already inherits CWindow::InvalidateRect, which would collide.
	virtual void InvalidateScreenRect(const CRect& rect) = 0;
};

enum EAnnotationTool {
	ATOOL_None,
	ATOOL_Freehand,
	ATOOL_Text,
	ATOOL_Rectangle
};

// Turns mouse input into annotations. Owns the model.
class CAnnotationCtl {
public:
	CAnnotationCtl(IAnnotationHost* pHost);

	// Selecting the tool that is already selected toggles rectangle fill; for the other
	// tools it does nothing. Leaving annotation mode is done with SetTool(ATOOL_None).
	void SetTool(EAnnotationTool eTool);
	EAnnotationTool GetTool() const { return m_eTool; }
	bool IsRectangleFilled() const { return m_bRectangleFilled; }
	bool IsAnnotating() const { return m_eTool != ATOOL_None; }
	bool HasUnsavedAnnotations() const { return m_model.IsDirty(); }

	// Return true when the event was consumed and must not reach panning, cropping or zoom.
	bool OnLButtonDown(int nX, int nY);
	bool OnMouseMove(int nX, int nY);
	bool OnLButtonUp(int nX, int nY);

	// Text tool: OnLButtonDown records where the caret goes; the dialog creates the edit
	// control there and calls CommitText when the user confirms. Empty text adds nothing.
	bool HasPendingText() const { return m_bPendingText; }
	CPoint GetPendingTextPosition() const { return m_ptPendingText; }
	void CancelText() { m_bPendingText = false; }
	void CommitText(LPCTSTR sText);

	void Undo();
	void Redo();
	void Clear();
	void MarkSaved() { m_model.MarkClean(); }

	CAnnotationModel& Model() { return m_model; }
	const CAnnotationModel& Model() const { return m_model; }

	// The annotation being drawn right now, or NULL. Painted on top of the committed ones.
	const CAnnotation* PendingAnnotation() const { return m_bDrawing ? &m_pending : NULL; }

	void SetStyle(COLORREF color, int nAlpha, int nPenWidthScreen, int nFontSizeScreen);
	COLORREF GetColor() const { return m_color; }
	int GetAlpha() const { return m_nAlpha; }
	int GetPenWidthScreen() const { return m_nPenWidthScreen; }
	int GetFontSizeScreen() const { return m_nFontSizeScreen; }

private:
	CPointF ToImage(int nX, int nY);
	void StartStyle(CAnnotation& annotation, EAnnotationType eType);
	void InvalidatePending();

	IAnnotationHost* m_pHost;
	CAnnotationModel m_model;
	EAnnotationTool m_eTool;
	bool m_bRectangleFilled;
	bool m_bDrawing;
	CAnnotation m_pending;
	bool m_bPendingText;
	CPoint m_ptPendingText;

	COLORREF m_color;
	int m_nAlpha;
	int m_nPenWidthScreen;
	int m_nFontSizeScreen;
};
```

- [ ] **Step 4: Write the implementation**

`src/JPEGView/AnnotationCtl.cpp`:

```cpp
#include "StdAfx.h"
#include "AnnotationCtl.h"
#include "AnnotationGeometry.h"

CAnnotationCtl::CAnnotationCtl(IAnnotationHost* pHost) {
	m_pHost = pHost;
	m_eTool = ATOOL_None;
	m_bRectangleFilled = false;
	m_bDrawing = false;
	m_bPendingText = false;
	m_ptPendingText = CPoint(0, 0);
	m_color = RGB(255, 0, 0);
	m_nAlpha = 180;
	m_nPenWidthScreen = 4;
	m_nFontSizeScreen = 24;
}

void CAnnotationCtl::SetTool(EAnnotationTool eTool) {
	if (eTool == ATOOL_Rectangle && m_eTool == ATOOL_Rectangle) {
		m_bRectangleFilled = !m_bRectangleFilled; // repeat click toggles fill
		return;
	}
	if (eTool != ATOOL_Rectangle) {
		m_bRectangleFilled = false;
	}
	if (eTool != ATOOL_Text) {
		m_bPendingText = false;
	}
	m_eTool = eTool;
	m_bDrawing = false;
}

void CAnnotationCtl::SetStyle(COLORREF color, int nAlpha, int nPenWidthScreen, int nFontSizeScreen) {
	m_color = color;
	m_nAlpha = max(0, min(255, nAlpha));
	m_nPenWidthScreen = max(1, nPenWidthScreen);
	m_nFontSizeScreen = max(4, nFontSizeScreen);
}

CPointF CAnnotationCtl::ToImage(int nX, int nY) {
	CPointF pt = AnnotationGeometry::ScreenToImage(CPoint(nX, nY),
		m_pHost->GetImageOrigin(), m_pHost->GetRealizedZoom());
	return AnnotationGeometry::ClampToImage(pt, m_pHost->GetImageSize());
}

void CAnnotationCtl::StartStyle(CAnnotation& annotation, EAnnotationType eType) {
	float fZoom = m_pHost->GetRealizedZoom();
	annotation = CAnnotation();
	annotation.eType = eType;
	annotation.color = m_color;
	annotation.nAlpha = m_nAlpha;
	annotation.fPenWidth = m_nPenWidthScreen / fZoom;
	annotation.fFontHeight = m_nFontSizeScreen / fZoom;
	annotation.bFilled = m_bRectangleFilled;
}

void CAnnotationCtl::InvalidatePending() {
	if (!m_bDrawing) {
		return;
	}
	m_pHost->InvalidateScreenRect(AnnotationGeometry::BoundingBoxOnScreen(m_pending,
		m_pHost->GetImageOrigin(), m_pHost->GetRealizedZoom()));
}

bool CAnnotationCtl::OnLButtonDown(int nX, int nY) {
	switch (m_eTool) {
		case ATOOL_None:
			return false;
		case ATOOL_Text:
			m_bPendingText = true;
			m_ptPendingText = CPoint(nX, nY);
			return true;
		case ATOOL_Freehand:
			StartStyle(m_pending, AT_Freehand);
			m_pending.points.push_back(ToImage(nX, nY));
			m_bDrawing = true;
			return true;
		case ATOOL_Rectangle:
			StartStyle(m_pending, AT_Rectangle);
			m_pending.points.push_back(ToImage(nX, nY));
			m_pending.points.push_back(ToImage(nX, nY));
			m_bDrawing = true;
			return true;
	}
	return false;
}

bool CAnnotationCtl::OnMouseMove(int nX, int nY) {
	if (!m_bDrawing) {
		return false;
	}
	InvalidatePending(); // old extent
	if (m_pending.eType == AT_Freehand) {
		m_pending.points.push_back(ToImage(nX, nY));
	} else if (m_pending.eType == AT_Rectangle) {
		m_pending.points[1] = ToImage(nX, nY);
	}
	InvalidatePending(); // new extent
	return true;
}

bool CAnnotationCtl::OnLButtonUp(int nX, int nY) {
	if (!m_bDrawing) {
		return false;
	}
	if (m_pending.eType == AT_Rectangle) {
		m_pending.points[1] = ToImage(nX, nY);
		AnnotationGeometry::NormalizeRectangle(m_pending);
	}
	InvalidatePending();
	m_bDrawing = false;
	// A click without a drag leaves a one-point stroke or a zero-size rectangle;
	// CAnnotationModel::Add drops both.
	m_model.Add(m_pending);
	return true;
}

void CAnnotationCtl::CommitText(LPCTSTR sText) {
	if (!m_bPendingText) {
		return;
	}
	m_bPendingText = false;
	CAnnotation annotation;
	StartStyle(annotation, AT_Text);
	annotation.points.push_back(ToImage(m_ptPendingText.x, m_ptPendingText.y));
	annotation.sText = (sText == NULL) ? _T("") : sText;
	m_model.Add(annotation); // empty text is dropped by the model
}

void CAnnotationCtl::Undo() { m_model.Undo(); }
void CAnnotationCtl::Redo() { m_model.Redo(); }
void CAnnotationCtl::Clear() { m_model.Clear(); }
```

Note on the zero-size rectangle: `CAnnotationModel::IsDegenerate` rejects a rectangle with fewer than two points but accepts one whose corners coincide. Add that case to the model now — change `AT_Rectangle` in `IsDegenerate` to:

```cpp
case AT_Rectangle:
	return annotation.points.size() < 2 ||
		(annotation.points[0].x == annotation.points[1].x &&
		 annotation.points[0].y == annotation.points[1].y);
```

and add this test to `TestAnnotationModel.cpp`:

```cpp
TEST(ZeroSizeRectangleIsRejected) {
	CAnnotation a;
	a.eType = AT_Rectangle;
	a.points.push_back(CPointF{ 10.0f, 10.0f });
	a.points.push_back(CPointF{ 10.0f, 10.0f });
	CAnnotationModel model;
	model.Add(a);
	CHECK(model.IsEmpty());
}
```

- [ ] **Step 5: Add to the projects and run the tests**

Run: `msbuild src\JPEGView.sln /t:JPEGViewTests /p:Configuration=Debug /p:Platform=x64` then `src\bin\x64\Debug\JPEGViewTests.exe`
Expected: `45 test(s) run, 0 failure(s)`.

- [ ] **Step 6: Commit**

```bash
git add src/JPEGView/AnnotationCtl.h src/JPEGView/AnnotationCtl.cpp src/JPEGView/AnnotationModel.cpp src/JPEGViewTests src/JPEGView/JPEGView.vcxproj src/JPEGView/JPEGView.vcxproj.filters src/JPEGView/JPEGView_VS2017.vcxproj src/JPEGView/JPEGView_VS2017.vcxproj.filters
git commit -m "feat: add the annotation input state machine"
```

---

### Task 6: Wire into the main dialog

After this task the freehand and rectangle tools work end to end from the keyboard shortcuts, with nothing on the navigation panel yet.

**Files:**
- Modify: `src/JPEGView/MainDlg.h`
- Modify: `src/JPEGView/MainDlg.cpp`

**Interfaces:**
- Consumes: `CAnnotationCtl`, `IAnnotationHost` (Task 5), `CAnnotationRenderer` (Task 3), the command IDs (Task 4).
- Produces: `CMainDlg::GetAnnotationCtl()`, `CMainDlg::IsAnnotating()`, and `CMainDlg` implementing `IAnnotationHost`.

- [ ] **Step 1: Make CMainDlg an annotation host**

In `MainDlg.h`, add `#include "AnnotationCtl.h"`, change the class declaration to `class CMainDlg : public CDialogImpl<CMainDlg>, public IAnnotationHost`, and add:

```cpp
public:
	CAnnotationCtl* GetAnnotationCtl() { return m_pAnnotationCtl; }
	bool IsAnnotating() { return m_pAnnotationCtl != NULL && m_pAnnotationCtl->IsAnnotating(); }

	// IAnnotationHost
	virtual CPoint GetImageOrigin() { return m_ptImageOrigin; }
	virtual float GetRealizedZoom() { return (float)m_dRealizedZoom; }
	virtual CSize GetImageSize() { return (m_pCurrentImage == NULL) ? CSize(0, 0) : m_pCurrentImage->OrigSize(); }
	virtual void InvalidateScreenRect(const CRect& rect) { this->InvalidateRect(&rect, FALSE); }

private:
	CAnnotationCtl* m_pAnnotationCtl;
	CPoint m_ptImageOrigin;
```

The interface method is `InvalidateScreenRect`, not `InvalidateRect`, precisely so it does not collide with the `CWindow::InvalidateRect` that `CMainDlg` already inherits. The implementation above forwards to that inherited one.

- [ ] **Step 2: Create and destroy the controller**

In `CMainDlg`'s constructor add `m_pAnnotationCtl = NULL; m_ptImageOrigin = CPoint(0, 0);`. In `OnInitDialog`, after the other controllers are created, add `m_pAnnotationCtl = new CAnnotationCtl(this);`. In the destructor, `delete m_pAnnotationCtl;`.

- [ ] **Step 3: Record the image origin and paint the annotations**

In `CMainDlg::OnPaint` (`MainDlg.cpp`, in the `pDIBData != NULL` branch), the existing line

```cpp
CPoint ptDIBStart = HelpersGUI::DrawDIB32bppWithBlackBorders(dc, bmInfo, pDIBData, backBrush, m_clientRect, clippedSize, m_DIBOffsets);
```

is followed by `memDCMgr.BlitImageToMemDC(...)`. After that call, add:

```cpp
m_ptImageOrigin = ptDIBStart;
if (m_pAnnotationCtl != NULL && !m_pAnnotationCtl->Model().IsEmpty()) {
	Gdiplus::Graphics graphics(dc);
	CAnnotationRenderer::Render(graphics, m_pAnnotationCtl->Model().Annotations(),
		(float)m_dRealizedZoom, Gdiplus::PointF((float)ptDIBStart.x, (float)ptDIBStart.y));
}
if (m_pAnnotationCtl != NULL && m_pAnnotationCtl->PendingAnnotation() != NULL) {
	Gdiplus::Graphics graphics(dc);
	CAnnotationRenderer::RenderOne(graphics, *m_pAnnotationCtl->PendingAnnotation(),
		(float)m_dRealizedZoom, Gdiplus::PointF((float)ptDIBStart.x, (float)ptDIBStart.y));
}
```

Add `#include "AnnotationRenderer.h"` to the includes at the top of `MainDlg.cpp`.

The pending annotation is drawn from the model's paint path rather than straight into the window DC during the drag. The spec allowed drawing only the new segment; painting the whole pending stroke each time is simpler, and because `CAnnotationCtl::OnMouseMove` invalidates only the stroke's bounding box, the repaint stays local. If freehand drawing feels sluggish on large images during manual testing, revisit this; do not optimise it before measuring.

- [ ] **Step 4: Route the mouse and Esc**

In `OnLButtonDown` (`MainDlg.cpp:757`), inside the `if (!bEatenByPanel) {` block, before `bool bDraggingRequired = ...`:

```cpp
if (m_pAnnotationCtl != NULL && m_pAnnotationCtl->OnLButtonDown(pointClicked.x, pointClicked.y)) {
	return 0; // annotation tool owns the drag; no panning, cropping or zoom
}
```

In `OnLButtonUp`, make the annotation case the first branch:

```cpp
if (m_pAnnotationCtl != NULL && m_pAnnotationCtl->OnLButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))) {
	Invalidate(FALSE);
} else if (m_bZoomMode) {
	// the rest of the existing chain follows unchanged, starting at m_bZoomMode = false;
```

In `OnMouseMove`, before the existing `} else if (m_pCropCtl->IsCropping()) {` chain, add an annotation branch that calls `m_pAnnotationCtl->OnMouseMove(m_nMouseX, m_nMouseY)` and returns early when it returns true.

In `OnKeyDown` (`MainDlg.cpp:1018`), insert a case into the existing Esc chain, after the crop case:

```cpp
} else if (wParam == VK_ESCAPE && m_pAnnotationCtl != NULL && m_pAnnotationCtl->IsAnnotating()) {
	bHandled = true;
	m_pAnnotationCtl->SetTool(ATOOL_None);
	SetCursorForMoveSection();
	Invalidate(FALSE);
}
```

Place it **after** the crop case so cropping keeps priority, matching how the existing chain is ordered.

- [ ] **Step 5: Set the crosshair cursor**

In `SetCursorForMoveSection()`, return `::LoadCursor(NULL, IDC_CROSS)` when `IsAnnotating()` is true, before the existing logic.

- [ ] **Step 6: Handle the commands**

In `CMainDlg::ExecuteCommand`, add:

```cpp
case IDM_ANNOTATE_FREEHAND:
	m_pAnnotationCtl->SetTool(ATOOL_Freehand);
	SetCursorForMoveSection();
	Invalidate(FALSE);
	break;
case IDM_ANNOTATE_TEXT:
	m_pAnnotationCtl->SetTool(ATOOL_Text);
	SetCursorForMoveSection();
	Invalidate(FALSE);
	break;
case IDM_ANNOTATE_RECT:
	m_pAnnotationCtl->SetTool(ATOOL_Rectangle);
	SetCursorForMoveSection();
	Invalidate(FALSE);
	break;
case IDM_ANNOTATE_OFF:
	m_pAnnotationCtl->SetTool(ATOOL_None);
	SetCursorForMoveSection();
	Invalidate(FALSE);
	break;
case IDM_ANNOTATE_UNDO:
	m_pAnnotationCtl->Undo();
	Invalidate(FALSE);
	break;
case IDM_ANNOTATE_REDO:
	m_pAnnotationCtl->Redo();
	Invalidate(FALSE);
	break;
case IDM_ANNOTATE_CLEAR:
	m_pAnnotationCtl->Clear();
	Invalidate(FALSE);
	break;
```

`IDM_ANNOTATE_APPLY_SAVE` is added in Task 9.

- [ ] **Step 7: Build and verify by hand**

Run: `msbuild src\JPEGView.sln /t:JPEGView /p:Configuration=Release /p:Platform=x64`

Copy the binary and `Config\*` to a scratch folder, set `StoreToEXEPath=true` in its `JPEGView.ini`, and copy `KeyMap.txt.default` to `KeyMap.txt` there so the new bindings exist. Open an image and check: `Ctrl+Shift+D` gives a crosshair; dragging draws a red line that stays put when zooming with the wheel and panning with the middle button; `Ctrl+Shift+R` draws rectangles; pressing it again switches to filled; `Ctrl+Z` and `Ctrl+Y` step back and forward; `Ctrl+Shift+Del` clears; `Esc` returns to the normal cursor.

- [ ] **Step 8: Commit**

```bash
git add src/JPEGView/MainDlg.h src/JPEGView/MainDlg.cpp src/JPEGView/AnnotationCtl.h src/JPEGView/AnnotationCtl.cpp src/JPEGViewTests/TestAnnotationCtl.cpp
git commit -m "feat: wire annotations into the main dialog"
```

---

### Task 7: Settings

**Files:**
- Modify: `src/JPEGView/SettingsProvider.h`
- Modify: `src/JPEGView/SettingsProvider.cpp`
- Modify: `src/JPEGView/Config/JPEGView.ini` and the three other ini defaults

**Interfaces:**
- Produces: `CSettingsProvider::AnnotationColor()`, `AnnotationOpacity()`, `AnnotationPenWidth()`, `AnnotationFontSize()`, `SaveAnnotationStyle(COLORREF, int, int, int)`.

- [ ] **Step 1: Add the getters and fields**

In `SettingsProvider.h`, next to the other colour getters:

```cpp
COLORREF AnnotationColor() { return m_colorAnnotation; }
int AnnotationOpacity() { return m_nAnnotationOpacity; }   // percent, 0..100
int AnnotationPenWidth() { return m_nAnnotationPenWidth; } // screen pixels
int AnnotationFontSize() { return m_nAnnotationFontSize; } // screen pixels

// Persists the style the user last picked, so it is restored on the next start.
void SaveAnnotationStyle(COLORREF color, int nOpacityPercent, int nPenWidth, int nFontSize);
```

and the four private members `COLORREF m_colorAnnotation; int m_nAnnotationOpacity, m_nAnnotationPenWidth, m_nAnnotationFontSize;`.

- [ ] **Step 2: Read them**

In `SettingsProvider.cpp`, next to the existing `m_colorTransparency = GetColor(...)` at about line 231:

```cpp
m_colorAnnotation = GetColor(_T("AnnotationColor"), RGB(255, 0, 0));
m_nAnnotationOpacity = max(0, min(100, GetInt(_T("AnnotationOpacity"), 70)));
m_nAnnotationPenWidth = max(1, min(100, GetInt(_T("AnnotationPenWidth"), 4)));
m_nAnnotationFontSize = max(4, min(400, GetInt(_T("AnnotationFontSize"), 24)));
```

Check the exact name and signature of the integer reader in this file before using `GetInt` — match whatever the neighbouring lines use.

- [ ] **Step 3: Implement the write-back**

```cpp
void CSettingsProvider::SaveAnnotationStyle(COLORREF color, int nOpacityPercent, int nPenWidth, int nFontSize) {
	MakeSureUserINIExists();
	CString sColor;
	sColor.Format(_T("%d %d %d"), GetRValue(color), GetGValue(color), GetBValue(color));
	WriteString(_T("AnnotationColor"), sColor);
	WriteInt(_T("AnnotationOpacity"), nOpacityPercent);
	WriteInt(_T("AnnotationPenWidth"), nPenWidth);
	WriteInt(_T("AnnotationFontSize"), nFontSize);
	m_colorAnnotation = color;
	m_nAnnotationOpacity = nOpacityPercent;
	m_nAnnotationPenWidth = nPenWidth;
	m_nAnnotationFontSize = nFontSize;
}
```

Review Focus 5: opacity 0 must survive this round trip. `WriteInt(..., 0)` writes `0`, and the reader clamps to `[0, 100]` without treating 0 as missing — confirm `GetInt` returns the stored 0 rather than the default when the key is present with value 0. If it does not, store opacity as `nOpacityPercent` and read with an explicit "key exists" check.

- [ ] **Step 4: Add the ini documentation**

Add to `src/JPEGView/Config/JPEGView.ini` near the other colour settings, and to the three other ini default files. Write the Russian ones as **CP1251 without a BOM**:

```ini
; Default colour for annotations (freehand, text, rectangles), R G B format as used by BackgroundColor
AnnotationColor=255 0 0
; Default annotation opacity in percent, 0 (invisible) .. 100 (opaque)
AnnotationOpacity=70
; Default annotation line width in screen pixels
AnnotationPenWidth=4
; Default annotation font size in screen pixels
AnnotationFontSize=24
```

- [ ] **Step 5: Apply the settings at startup**

In `CMainDlg::OnInitDialog`, right after `m_pAnnotationCtl = new CAnnotationCtl(this);`:

```cpp
CSettingsProvider& sp = CSettingsProvider::This();
m_pAnnotationCtl->SetStyle(sp.AnnotationColor(), sp.AnnotationOpacity() * 255 / 100,
	sp.AnnotationPenWidth(), sp.AnnotationFontSize());
```

- [ ] **Step 6: Build and verify**

Build Release x64, set `AnnotationColor=0 0 255` and `AnnotationOpacity=30` in the scratch folder's ini, and confirm a freehand stroke comes out blue and faint.

- [ ] **Step 7: Commit**

```bash
git add src/JPEGView/SettingsProvider.h src/JPEGView/SettingsProvider.cpp src/JPEGView/Config src/JPEGView/MainDlg.cpp
git commit -m "feat: add annotation style settings"
```

---

### Task 8: Navigation panel buttons

**Files:**
- Modify: `src/JPEGView/NavigationPanel.h`
- Modify: `src/JPEGView/NavigationPanel.cpp`
- Modify: `src/JPEGView/NavigationPanelCtl.cpp`

**Interfaces:**
- Consumes: the command IDs (Task 4), `CMainDlg::GetAnnotationCtl` (Task 6).
- Produces: `GetBtnAnnotateFreehand()`, `GetBtnAnnotateText()`, `GetBtnAnnotateRect()`, `GetBtnAnnotateClear()`, `GetBtnAnnotateStyle()` on `CNavigationPanel`.

- [ ] **Step 1: Add the control IDs and accessors**

In `NavigationPanel.h`, extend the anonymous enum after `ID_btnShowInfo` with `ID_gap7, ID_btnAnnotateFreehand, ID_btnAnnotateText, ID_btnAnnotateRect, ID_btnAnnotateClear, ID_btnAnnotateStyle`, add the five `GetBtn...` accessors in the style of the existing ones, and declare five static paint handlers plus `static LPCTSTR RectangleTooltip(void* pContext);`.

- [ ] **Step 2: Create the buttons**

At the end of `CNavigationPanel::CNavigationPanel`, before `m_nOptimalWidth = PanelRect().Width();`:

```cpp
AddGap(ID_gap7, 16);
AddUserPaintButton(ID_btnAnnotateFreehand, GetTooltip(keyMap, _T("Draw freehand"), IDM_ANNOTATE_FREEHAND), &PaintAnnotateFreehandBtn);
AddUserPaintButton(ID_btnAnnotateText, GetTooltip(keyMap, _T("Add text"), IDM_ANNOTATE_TEXT), &PaintAnnotateTextBtn);
AddUserPaintButton(ID_btnAnnotateRect, &RectangleTooltip, &PaintAnnotateRectBtn, NULL, this);
AddUserPaintButton(ID_btnAnnotateClear, GetTooltip(keyMap, _T("Clear annotations"), IDM_ANNOTATE_CLEAR), &PaintAnnotateClearBtn);
AddUserPaintButton(ID_btnAnnotateStyle, GetTooltip(keyMap, _T("Annotation colour, opacity and width"), 0), &PaintAnnotateStyleBtn, NULL, this);
```

`RectangleTooltip` follows the pattern of the existing `WindowModeTooltip`, returning "Draw rectangle (outline)" or "Draw rectangle (filled)" depending on `m_pAnnotationCtl->IsRectangleFilled()`. The panel has no pointer to the dialog, so reach it through the same route `WindowModeTooltip` uses — read that function before writing this one and copy its mechanism rather than inventing a new one.

- [ ] **Step 3: Write the glyph painters**

Follow the existing painters in `NavigationPanel.cpp` exactly: they take `(void* pContext, const CRect& rect, CDC& dc)` and draw with the pen and brush already selected into `dc`, using `Helpers::InflateRect` to inset. Five glyphs:

```cpp
void CNavigationPanel::PaintAnnotateFreehandBtn(void* pContext, const CRect& rect, CDC& dc) {
	// a short wavy line, drawn as three segments
	CRect r = Helpers::InflateRect(rect, 0.20f);
	dc.MoveTo(r.left, r.bottom);
	dc.LineTo(r.left + r.Width() / 3, r.top + r.Height() / 4);
	dc.LineTo(r.left + 2 * r.Width() / 3, r.bottom - r.Height() / 4);
	dc.LineTo(r.right, r.top);
}

void CNavigationPanel::PaintAnnotateTextBtn(void* pContext, const CRect& rect, CDC& dc) {
	// a capital T: a horizontal bar and a vertical stem
	CRect r = Helpers::InflateRect(rect, 0.25f);
	dc.MoveTo(r.left, r.top);
	dc.LineTo(r.right, r.top);
	dc.MoveTo((r.left + r.right) / 2, r.top);
	dc.LineTo((r.left + r.right) / 2, r.bottom);
}

void CNavigationPanel::PaintAnnotateRectBtn(void* pContext, const CRect& rect, CDC& dc) {
	CRect r = Helpers::InflateRect(rect, 0.25f);
	dc.Rectangle(r.left, r.top, r.right, r.bottom);
	// when filled mode is active, hatch the interior with two diagonals
	CNavigationPanel* pPanel = (CNavigationPanel*)pContext;
	if (pPanel != NULL && pPanel->IsRectangleFilledMode()) {
		dc.MoveTo(r.left, r.bottom); dc.LineTo(r.right, r.top);
		dc.MoveTo(r.left, r.top); dc.LineTo(r.right, r.bottom);
	}
}

void CNavigationPanel::PaintAnnotateClearBtn(void* pContext, const CRect& rect, CDC& dc) {
	// an X
	CRect r = Helpers::InflateRect(rect, 0.28f);
	dc.MoveTo(r.left, r.top); dc.LineTo(r.right, r.bottom);
	dc.MoveTo(r.left, r.bottom); dc.LineTo(r.right, r.top);
}

void CNavigationPanel::PaintAnnotateStyleBtn(void* pContext, const CRect& rect, CDC& dc) {
	// a filled disc in the current annotation colour, sized by the current pen width
	CNavigationPanel* pPanel = (CNavigationPanel*)pContext;
	CRect r = Helpers::InflateRect(rect, 0.22f);
	int nRadius = r.Width() / 2;
	if (pPanel != NULL) {
		int nWidth = pPanel->CurrentPenWidth(); // 1..100
		nRadius = max(2, min(r.Width() / 2, 2 + (r.Width() / 2 - 2) * min(nWidth, 20) / 20));
	}
	CPoint ptCenter((r.left + r.right) / 2, (r.top + r.bottom) / 2);
	CBrush brush;
	brush.CreateSolidBrush(pPanel == NULL ? RGB(255, 0, 0) : pPanel->CurrentColor());
	HBRUSH hOld = dc.SelectBrush(brush);
	dc.Ellipse(ptCenter.x - nRadius, ptCenter.y - nRadius, ptCenter.x + nRadius, ptCenter.y + nRadius);
	dc.SelectBrush(hOld);
}
```

Add `bool IsRectangleFilledMode();`, `COLORREF CurrentColor();` and `int CurrentPenWidth();` to `CNavigationPanel`, reading from the annotation controller through the same route `RectangleTooltip` uses.

The style glyph is drawn with a solid brush rather than the outlined style the other glyphs use, because its whole job is to show the current colour. `CUICtrl::OnPaint` will still draw it five times for the outline; that is correct and gives the disc a dark rim.

- [ ] **Step 4: Bind the handlers**

In `CNavigationPanelCtl`'s constructor, next to the existing `SetButtonPressedHandler` calls:

```cpp
m_pNavPanel->GetBtnAnnotateFreehand()->SetButtonPressedHandler(&CMainDlg::OnExecuteCommand, pMainDlg, IDM_ANNOTATE_FREEHAND);
m_pNavPanel->GetBtnAnnotateText()->SetButtonPressedHandler(&CMainDlg::OnExecuteCommand, pMainDlg, IDM_ANNOTATE_TEXT);
m_pNavPanel->GetBtnAnnotateRect()->SetButtonPressedHandler(&CMainDlg::OnExecuteCommand, pMainDlg, IDM_ANNOTATE_RECT);
m_pNavPanel->GetBtnAnnotateClear()->SetButtonPressedHandler(&CMainDlg::OnExecuteCommand, pMainDlg, IDM_ANNOTATE_CLEAR);
```

The style button is bound in Task 9.

- [ ] **Step 5: Reflect the active tool**

Add a `CNavigationPanelCtl::UpdateAnnotationButtons()` that calls `SetActive` on each of the three tool buttons according to `pMainDlg->GetAnnotationCtl()->GetTool()`, and call it from `CMainDlg::ExecuteCommand` after each `SetTool` in Task 6, Step 6.

- [ ] **Step 6: Build and verify by hand**

Build Release x64. Check on a 1024-wide window that the whole navigation panel still fits and every button is reachable; that clicking each tool button highlights it and draws; that clicking the rectangle button twice hatches its glyph and switches to filled; that the style disc shows red.

- [ ] **Step 7: Commit**

```bash
git add src/JPEGView/NavigationPanel.h src/JPEGView/NavigationPanel.cpp src/JPEGView/NavigationPanelCtl.h src/JPEGView/NavigationPanelCtl.cpp src/JPEGView/MainDlg.cpp
git commit -m "feat: add annotation buttons to the navigation panel"
```

---

### Task 9: The style popup

**Files:**
- Create: `src/JPEGView/AnnotationStylePanel.h`, `.cpp`
- Create: `src/JPEGView/AnnotationStylePanelCtl.h`, `.cpp`
- Modify: `src/JPEGView/MainDlg.h`, `MainDlg.cpp`, `NavigationPanelCtl.cpp`
- Modify: the four project files (`Panels` filter)

**Interfaces:**
- Consumes: `CAnnotationCtl::SetStyle` (Task 5), `CSettingsProvider::SaveAnnotationStyle` (Task 7).
- Produces: `CAnnotationStylePanelCtl` with `bool IsVisible()`, `void SetVisible(bool)`.

- [ ] **Step 1: Write the panel**

Model it on `CTitleBarPanel` for structure and on `CImageProcessingPanel` for sliders. `CAnnotationStylePanel : public CPanel` holds eight `ID_btnColor0..7` user-paint buttons each drawing a filled swatch, `ID_sliderOpacity` and `ID_sliderWidth` created with the inherited

```cpp
CSliderDouble* AddSlider(int nID, LPCTSTR sName, double* pdValue, bool* pbEnable,
    double dMin, double dMax, double dDefaultValue, bool bAllowPreviewAndReset, bool bLogarithmic, bool bInvert, int nWidth);
```

bound to `double m_dOpacity` (0..100) and `double m_dWidth` (1..100), and an `ID_btnOtherColor` text button. The eight presets: red `255 0 0`, yellow `255 220 0`, green `0 200 0`, cyan `0 200 255`, blue `0 80 255`, magenta `255 0 200`, white `255 255 255`, black `0 0 0`.

`PanelRect()` returns a rectangle centred horizontally, sitting directly above the navigation panel — take the navigation panel's `PanelRect()` as the anchor the way `CNavigationPanel` takes `m_pImageProcPanel`.

Guard `RepositionAll()` against controls that are not in the map yet: `CPanel::AddText` and `AddSlider` insert into `m_controls` only **after** the control's constructor runs, and that constructor already calls `RequestRepositioning()`. Every `GetControl` result in `RepositionAll` must be null-checked. This exact mistake crashed the transparent title bar work on startup.

- [ ] **Step 2: Write the controller**

`CAnnotationStylePanelCtl : public CPanelController`, following `CTitleBarPanelCtl`. `IsVisible()` returns an explicit `m_bVisible` flag toggled by the style button. `DimFactor()` returns a small positive value (e.g. `0.5f`) so the popup is readable over any image — unlike the title bar, this panel has controls to hit and benefits from a darkened backing. Register it with `m_pPanelMgr->AddPanelController(...)` in `OnInitDialog`.

When a swatch is clicked or a slider moves, call

```cpp
m_pMainDlg->GetAnnotationCtl()->SetStyle(color, (int)(m_dOpacity * 255 / 100), (int)m_dWidth, (int)m_dWidth);
```

Note both width and font size are driven by the same slider, per the agreed design: the slider sets pen width when the active tool is not text, and font size when it is. Store them separately in the controller so switching tools does not lose the other value:

```cpp
if (m_pMainDlg->GetAnnotationCtl()->GetTool() == ATOOL_Text) {
	m_nFontSize = (int)m_dWidth;
} else {
	m_nPenWidth = (int)m_dWidth;
}
m_pMainDlg->GetAnnotationCtl()->SetStyle(m_color, (int)(m_dOpacity * 255 / 100), m_nPenWidth, m_nFontSize);
```

and when the panel is shown, load `m_dWidth` from whichever of the two applies to the current tool, and relabel the slider "Line width" or "Font size" accordingly.

- [ ] **Step 3: Persist on change**

After every style change, call

```cpp
CSettingsProvider::This().SaveAnnotationStyle(m_color, (int)m_dOpacity, m_nPenWidth, m_nFontSize);
```

- [ ] **Step 4: Bind the style button**

In `CNavigationPanelCtl`, bind `GetBtnAnnotateStyle()` to a handler that toggles `CAnnotationStylePanelCtl::SetVisible`.

- [ ] **Step 5: Wire "Other colour…"**

Open the system dialog with `CHOOSECOLOR` / `::ChooseColor`, seeded with the current colour and with a `lpCustColors` array of 16 `COLORREF` held as a static in the controller so custom colours persist within the session.

- [ ] **Step 6: Build and verify by hand**

Build Release x64. Check: the style button opens and closes the popup; each swatch changes the colour of the next stroke and the disc on the button; the opacity slider visibly changes translucency; the width slider changes stroke thickness and, with the text tool active, is labelled "Font size" and changes text size instead; restarting JPEGView restores the last colour, opacity and width.

- [ ] **Step 7: Commit**

```bash
git add src/JPEGView/AnnotationStylePanel.h src/JPEGView/AnnotationStylePanel.cpp src/JPEGView/AnnotationStylePanelCtl.h src/JPEGView/AnnotationStylePanelCtl.cpp src/JPEGView/MainDlg.h src/JPEGView/MainDlg.cpp src/JPEGView/NavigationPanelCtl.cpp src/JPEGView/JPEGView.vcxproj src/JPEGView/JPEGView.vcxproj.filters src/JPEGView/JPEGView_VS2017.vcxproj src/JPEGView/JPEGView_VS2017.vcxproj.filters
git commit -m "feat: add the annotation style popup"
```

---

### Task 10: The text tool

**Files:**
- Modify: `src/JPEGView/MainDlg.h`, `MainDlg.cpp`

**Interfaces:**
- Consumes: `CAnnotationCtl::HasPendingText`, `GetPendingTextPosition`, `CommitText`, `CancelText` (Task 5).

- [ ] **Step 1: Create the edit control on click**

Add `CEdit m_annotationEdit;` and `bool m_bAnnotationEditActive;` to `CMainDlg`. In `OnLButtonDown`, when `m_pAnnotationCtl->OnLButtonDown(...)` returns true and `m_pAnnotationCtl->HasPendingText()`, create the control:

```cpp
void CMainDlg::StartAnnotationTextEdit() {
	CPoint pt = m_pAnnotationCtl->GetPendingTextPosition();
	int nHeight = m_pAnnotationCtl->GetFontSizeScreen();
	CRect rect(pt, CSize(nHeight * 20, (int)(nHeight * 1.4)));
	if (!m_annotationEdit.IsWindow()) {
		m_annotationEdit.Create(m_hWnd, rect, NULL,
			WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 0, IDC_ANNOTATION_EDIT);
	} else {
		m_annotationEdit.MoveWindow(&rect);
		m_annotationEdit.ShowWindow(SW_SHOW);
	}
	if (m_annotationEditFont.m_hFont != NULL) m_annotationEditFont.DeleteObject();
	m_annotationEditFont.CreateFont(-nHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
		DEFAULT_PITCH, _T("Segoe UI"));
	m_annotationEdit.SetFont(m_annotationEditFont);
	m_annotationEdit.SetWindowText(_T(""));
	m_annotationEdit.SetFocus();
	m_bAnnotationEditActive = true;
}
```

Add `CFont m_annotationEditFont;` and `#define IDC_ANNOTATION_EDIT 1701` (check `resource.h` for a free control ID in that range first).

- [ ] **Step 2: Colour it**

`CMainDlg` already handles `WM_CTLCOLOREDIT` in `OnCtlColorEdit`. Extend it: when the control is `m_annotationEdit`, `SetTextColor` to `m_pAnnotationCtl->GetColor()`, `SetBkColor` to a dim grey such as `RGB(40, 40, 40)`, and return a cached grey brush. The dim backing is deliberate — `CEdit` cannot paint a transparent background, and the text is redrawn properly by the renderer once committed.

- [ ] **Step 3: Commit and cancel**

In `OnKeyDown`, before everything else, when `m_bAnnotationEditActive`:

- `VK_RETURN`: read the text with `GetWindowText`, hide the control, `m_bAnnotationEditActive = false`, call `m_pAnnotationCtl->CommitText(sText)`, `Invalidate(FALSE)`.
- `VK_ESCAPE`: hide the control, `m_bAnnotationEditActive = false`, `m_pAnnotationCtl->CancelText()`, `Invalidate(FALSE)`. This consumes the first Esc, so a second Esc leaves annotation mode, matching the spec.

The edit control has focus, so these keys arrive at the control, not the dialog. Route them by handling `WM_COMMAND` with `EN_*` is not enough — subclass the edit or handle `WM_GETDLGCODE`. `CMainDlg` already has an `OnGetDlgCode` handler; return `DLGC_WANTALLKEYS` while `m_bAnnotationEditActive` so the dialog sees Enter and Esc, and verify by hand that typing still reaches the control.

Also commit the text when the user clicks elsewhere on the image: at the top of `OnLButtonDown`, if `m_bAnnotationEditActive`, commit first and let the click proceed.

- [ ] **Step 4: Build and verify by hand**

Build Release x64. Check: `Ctrl+Shift+T` then a click puts a caret on the image; typing shows the text in the chosen colour; Backspace, selection and `Ctrl+V` work; Russian layout types Cyrillic; Enter commits and the backing disappears; Esc discards; clicking elsewhere commits; an empty Enter adds nothing and leaves no save prompt on navigation (Review Focus 4, confirmed manually here and by `CommitEmptyTextAddsNothing` in Task 5).

- [ ] **Step 5: Commit**

```bash
git add src/JPEGView/MainDlg.h src/JPEGView/MainDlg.cpp src/JPEGView/resource.h
git commit -m "feat: add in-place text entry for annotations"
```

---

### Task 11: Burn-in and the save prompt

**Files:**
- Modify: `src/JPEGView/JPEGImage.h`, `JPEGImage.cpp`
- Modify: `src/JPEGView/MainDlg.h`, `MainDlg.cpp`
- Create: `src/JPEGViewTests/TestAnnotationBurnIn.cpp`
- Modify: `src/JPEGViewTests/JPEGViewTests.vcxproj`

**Interfaces:**
- Consumes: `CAnnotationRenderer::Render` (Task 3), `CAnnotationCtl::Model`, `MarkSaved` (Task 5).
- Produces: `CJPEGImage::ApplyAnnotationsToOriginalPixels(const std::vector<CAnnotation>&)`; `CMainDlg::PromptSaveAnnotations()` returning `bool` (false means the caller must abort its action).

- [ ] **Step 1: Write the failing burn-in test**

The renderer is what does the work, so the test exercises rendering into a raw 32 bpp buffer the same way the image class will, without needing `CJPEGImage`:

`src/JPEGViewTests/TestAnnotationBurnIn.cpp`:

```cpp
#include "TestFramework.h"
#include "AnnotationRenderer.h"
#include <gdiplus.h>

// Mirrors what CJPEGImage::ApplyAnnotationsToOriginalPixels does: wrap an existing
// 32 bpp buffer in a Gdiplus::Bitmap without copying, and render into it at scale 1.
static void BurnIn(void* pPixels, int nWidth, int nHeight, const std::vector<CAnnotation>& annotations) {
	Gdiplus::Bitmap bitmap(nWidth, nHeight, nWidth * 4, PixelFormat32bppRGB, (BYTE*)pPixels);
	Gdiplus::Graphics g(&bitmap);
	CAnnotationRenderer::Render(g, annotations, 1.0f, Gdiplus::PointF(0.0f, 0.0f));
}

TEST(BurnInWritesIntoTheCallersBuffer) {
	const int nWidth = 64, nHeight = 64;
	std::vector<unsigned int> pixels(nWidth * nHeight, 0x00FFFFFF); // white

	CAnnotation a;
	a.eType = AT_Rectangle;
	a.bFilled = true;
	a.color = RGB(0, 0, 255);
	a.nAlpha = 255;
	a.points.push_back(CPointF{ 10.0f, 10.0f });
	a.points.push_back(CPointF{ 50.0f, 50.0f });
	std::vector<CAnnotation> annotations;
	annotations.push_back(a);

	BurnIn(&pixels[0], nWidth, nHeight, annotations);

	unsigned int inside = pixels[32 * nWidth + 32] & 0x00FFFFFF;
	CHECK(inside == 0x000000FF);                       // BGRA little endian: blue
	unsigned int outside = pixels[2 * nWidth + 2] & 0x00FFFFFF;
	CHECK(outside == 0x00FFFFFF);                      // untouched white
}

TEST(BurnInAtScaleOneMatchesTheImageCoordinates) {
	const int nWidth = 64, nHeight = 64;
	std::vector<unsigned int> pixels(nWidth * nHeight, 0x00FFFFFF);
	CAnnotation a;
	a.eType = AT_Rectangle;
	a.bFilled = true;
	a.color = RGB(0, 255, 0);
	a.nAlpha = 255;
	a.points.push_back(CPointF{ 0.0f, 0.0f });
	a.points.push_back(CPointF{ 5.0f, 5.0f });
	std::vector<CAnnotation> annotations;
	annotations.push_back(a);
	BurnIn(&pixels[0], nWidth, nHeight, annotations);
	CHECK((pixels[2 * nWidth + 2] & 0x00FFFFFF) == 0x0000FF00);
	CHECK((pixels[20 * nWidth + 20] & 0x00FFFFFF) == 0x00FFFFFF);
}
```

- [ ] **Step 2: Run to verify it fails, then passes**

Run: `msbuild src\JPEGView.sln /t:JPEGViewTests /p:Configuration=Debug /p:Platform=x64` then run the exe. These tests use only the existing renderer, so they should pass immediately; if they do not, the renderer's coordinate handling is wrong and must be fixed before proceeding.

- [ ] **Step 3: Implement the image method**

In `JPEGImage.h`, next to `RotateOriginalPixels` / `TrapezoidOriginalPixels`:

```cpp
// Renders the annotations into the original pixels, permanently. Returns false on failure.
bool ApplyAnnotationsToOriginalPixels(const std::vector<CAnnotation>& annotations);
```

In `JPEGImage.cpp`, read `RotateOriginalPixels` first and follow its structure for locking, replacing `m_pOrigPixels` and invalidating the cached DIBs. If `GetOriginalChannelCount()` returns 3, convert to 32 bpp first using whichever helper `BasicProcessing` already provides — find it rather than writing a new one. Then:

```cpp
Gdiplus::Bitmap bitmap(m_nOrigWidth, m_nOrigHeight, m_nOrigWidth * 4,
	PixelFormat32bppRGB, (BYTE*)m_pOrigPixels);
Gdiplus::Graphics g(&bitmap);
CAnnotationRenderer::Render(g, annotations, 1.0f, Gdiplus::PointF(0.0f, 0.0f));
```

and invalidate the caches exactly as the rotation method does at its end.

- [ ] **Step 4: Add the save prompt**

`MessageBox` offers at most three buttons, so add a dialog resource `IDD_SAVE_ANNOTATIONS` to `JPEGView.rc` with a static text and four push buttons — `IDC_ANNOT_OVERWRITE`, `IDC_ANNOT_SAVEAS`, `IDC_ANNOT_DISCARD`, `IDCANCEL` — and a `CSaveAnnotationsDlg : public CDialogImpl<CSaveAnnotationsDlg>` that maps each to `EndDialog` with its own ID. Model the dialog class on the existing `CCropSizeDlg` in `CropSizeDlg.h/cpp`, which is the smallest dialog in this codebase.

```cpp
// Returns false if the caller must abort whatever it was about to do.
bool CMainDlg::PromptSaveAnnotations() {
	if (m_pAnnotationCtl == NULL || !m_pAnnotationCtl->HasUnsavedAnnotations() || m_pCurrentImage == NULL) {
		return true;
	}
	CSaveAnnotationsDlg dlg(CurrentFileName(false));
	int nResult = (int)dlg.DoModal(m_hWnd);
	if (nResult == IDCANCEL) {
		return false;
	}
	if (nResult == IDC_ANNOT_DISCARD) {
		m_pAnnotationCtl->Clear();
		Invalidate(FALSE);
		return true;
	}

	CString sTargetFile = CurrentFileName(false);
	if (nResult == IDC_ANNOT_SAVEAS) {
		// Reuse the existing save-as flow; read how IDM_SAVE builds its file dialog in
		// ExecuteCommand and call the same helper rather than opening a second one here.
		if (!AskForSaveFileName(sTargetFile)) {
			return false; // cancelling the file dialog cancels the whole action
		}
	}

	if (!m_pCurrentImage->ApplyAnnotationsToOriginalPixels(m_pAnnotationCtl->Model().Annotations())) {
		::MessageBox(m_hWnd, CNLS::GetString(_T("Could not apply the annotations to the image")),
			CNLS::GetString(_T("Error")), MB_ICONSTOP | MB_OK);
		return false;
	}
	bool bSuccess = CSaveImage::SaveImage(sTargetFile, m_pCurrentImage, *m_pImageProcParams,
		CreateDefaultProcessingFlags(), true, CSettingsProvider::This().UseLosslessWEBP());
	if (!bSuccess) {
		::MessageBox(m_hWnd, CNLS::GetString(_T("Could not save file")),
			CNLS::GetString(_T("Error")), MB_ICONSTOP | MB_OK);
		return false;
	}
	m_pAnnotationCtl->MarkSaved();
	Invalidate(FALSE);
	return true;
}
```

Three names here must be checked against the real code before use rather than trusted from this plan: `CurrentFileName(bool)`, the processing-flags helper (`CreateDefaultProcessingFlags` may be spelled differently — `ExecuteCommand`'s `IDM_SAVE` case shows the exact call), and whether a save-as filename helper already exists. If there is no `AskForSaveFileName`, extract one from the `IDM_SAVE` case rather than duplicating its dialog setup.

Returning `false` after a failed save is deliberate: the annotations are still unsaved, so the user must stay on this image rather than silently losing the work.

- [ ] **Step 5: Call it from every exit**

Call `PromptSaveAnnotations()` and return early when it is false, at the start of: `GotoImage`, `OpenFileWithDialog`, `OnClose`, `OnDropFiles`, and the `ExecuteCommand` cases for `IDM_ROTATE_90`, `IDM_ROTATE_270`, `IDM_ROTATE`, `IDM_PERSPECTIVE`, `IDM_CROP_SEL`, `IDM_LOSSLESS_CROP_SEL` and `IDM_RESIZE`. Find each by name before editing; do not guess line numbers.

Also clear the annotations without prompting when a new image finishes loading after the prompt has already been answered — `OnImageLoadCompleted` should call `m_pAnnotationCtl->Clear()`.

- [ ] **Step 6: Add the apply-and-save command**

```cpp
case IDM_ANNOTATE_APPLY_SAVE:
	PromptSaveAnnotations();
	Invalidate(FALSE);
	break;
```

- [ ] **Step 7: Build and verify by hand**

Build Release x64. Check on a **copy** of a test image, never on an original worth keeping: draw, press `Ctrl+Shift+A`, pick Overwrite, confirm the file on disk now contains the drawing at full resolution and that the strokes sit where they were on screen; repeat with Save as…; confirm Don't save discards; confirm Cancel keeps you on the image; confirm the prompt appears when pressing the next-image key and when rotating.

- [ ] **Step 8: Commit**

```bash
git add src/JPEGView/JPEGImage.h src/JPEGView/JPEGImage.cpp src/JPEGView/MainDlg.h src/JPEGView/MainDlg.cpp src/JPEGView/JPEGView.rc src/JPEGView/resource.h src/JPEGViewTests
git commit -m "feat: burn annotations into the image and prompt to save"
```

---

### Task 12: Help screen and final pass

**Files:**
- Modify: `src/JPEGView/HelpDisplayCtl.cpp`
- Modify: `src/JPEGView/Config/strings_ru.txt`
- Modify: `README.md` or the docs the repository keeps for features, if any

- [ ] **Step 1: Add the help lines**

In `HelpDisplayCtl.cpp`, following the existing `AddLineInfo(_KeyDesc(IDM_TRANSPARENT_TITLE_BAR), ...)` pattern, add lines for `IDM_ANNOTATE_FREEHAND`, `IDM_ANNOTATE_TEXT`, `IDM_ANNOTATE_RECT`, `IDM_ANNOTATE_UNDO`, `IDM_ANNOTATE_CLEAR` and `IDM_ANNOTATE_APPLY_SAVE`, with the active-state flag reading from `m_pMainDlg->GetAnnotationCtl()->GetTool()`.

- [ ] **Step 2: Complete the Russian strings**

Re-check every user-visible English string added across Tasks 4–11 has an entry in `strings_ru.txt`, including the four button captions of the save prompt. Keep the UTF-8 BOM.

- [ ] **Step 3: Full build and test run**

Run:
```
msbuild src\JPEGView.sln /t:JPEGViewTests /p:Configuration=Debug /p:Platform=x64
src\bin\x64\Debug\JPEGViewTests.exe
msbuild src\JPEGView.sln /t:JPEGView /p:Configuration=Release /p:Platform=x64
msbuild src\JPEGView.sln /t:JPEGView /p:Configuration=Release /p:Platform=Win32
msbuild src\JPEGView.sln /t:JPEGView /p:Configuration=Debug /p:Platform=x64
msbuild src\JPEGView.sln /t:JPEGView /p:Configuration=Debug /p:Platform=Win32
```
Expected: all succeed, tests report zero failures.

- [ ] **Step 4: Push and confirm CI**

```bash
git add -A
git commit -m "docs: add annotations to the help screen and Russian strings"
git push fork annotations
```

Confirm all five workflows pass on the pushed commit before opening the pull request.

---

## Notes for whoever executes this

- Every manual verification step must run from a scratch folder with `StoreToEXEPath=true`. The user's `%APPDATA%\JPEGView\JPEGView.ini` and `KeyMap.txt` are off limits.
- `JPEGView.cpp` wraps the message loop in `catch (...)` and the project builds with `/EHa`, so an access violation is swallowed and the application exits with code 0 and no message. If the built binary starts and vanishes, that is the cause; build Debug and look at the assertion rather than guessing.
- `CPanel::AddText` / `AddSlider` / `AddUserPaintButton` insert into `m_controls` only after the control's constructor has run, and that constructor already calls `RequestRepositioning()`. Null-check every `GetControl` result inside any `RepositionAll` override.
