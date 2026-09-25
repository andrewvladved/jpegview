#pragma once

// Self-sufficient on purpose: the annotation model, geometry and renderer are
// compiled into the unit test project as well as into JPEGView, so they must not
// depend on the application's StdAfx.h (which pulls in WTL).
#include <windows.h>
#include <atlbase.h>
#include <atlstr.h>
#include <atltypes.h>
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
