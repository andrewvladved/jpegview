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
	AT_Rectangle,
	AT_Ellipse,
	AT_Triangle
};

// The three shapes the shape tool cycles through. They differ only in what is drawn:
// each is described by the two opposite corners of the same dragged box.
inline bool IsTwoCornerShape(EAnnotationType eType) {
	return eType == AT_Rectangle || eType == AT_Ellipse || eType == AT_Triangle;
}

// How far the arrow head reaches past the end of a freehand line, as a multiple of the
// pen width. The renderer and the code deciding what to repaint have to agree on it.
const float ARROW_LENGTH_IN_PEN_WIDTHS = 4.0f;

// One annotation element. Which fields matter depends on eType:
//   AT_Freehand  - points is a polyline of at least two points, fPenWidth is the stroke
//                  width, bArrowHead adds an arrow at the end of the line
//   AT_Rectangle,
//   AT_Ellipse,
//   AT_Triangle  - points holds exactly two opposite corners of the box the shape is
//                  drawn in, bFilled selects fill or outline
//   AT_Text      - points holds one anchor (top left of the text), sText and fFontHeight
//                  are used, bBackground fills backColor behind the glyphs
// All coordinates and sizes are in pixels of the image as currently displayed.
struct CAnnotation {
	CAnnotation() : eType(AT_Freehand), color(0), nAlpha(255), fPenWidth(1.0f),
		bFilled(false), bArrowHead(false), bBackground(false), backColor(0),
		fFontHeight(12.0f) {}

	EAnnotationType      eType;
	COLORREF             color;
	int                  nAlpha;      // 0 .. 255
	float                fPenWidth;   // image pixels
	bool                 bFilled;     // shapes only
	bool                 bArrowHead;  // freehand only
	bool                 bBackground; // text only
	COLORREF             backColor;   // text only, the colour behind the glyphs
	std::vector<CPointF> points;
	CString              sText;       // text only
	float                fFontHeight; // text only, image pixels
};
