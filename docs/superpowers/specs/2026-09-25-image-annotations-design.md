# Image annotations — design

Date: 2026-09-25
Status: approved design, not yet implemented
Branch: `annotations`

## Purpose

Let the user mark up the image being viewed — freehand strokes with the mouse, typed text
labels, and rectangles (outline or semi-transparent fill) — choose the colour, opacity and
line width, and burn the result into the image file.

This is an **editing** feature, not an overlay: annotations exist only until they are written
into the picture. When the user navigates away with unsaved annotations, JPEGView asks whether
to save.

## Decisions taken with the user

| Question | Decision |
|---|---|
| Lifetime of annotations | Live only for the currently open image; on leaving it, prompt to save |
| Where saving writes | Dialog offers **Overwrite** / **Save as…** / **Don't save** / **Cancel** |
| Editability | Append-only with undo/redo and "clear all"; individual elements cannot be moved |
| Drawing vs navigation | Selecting a tool enters a mode: left-drag draws, panning stays on the middle button |
| Text entry | Typed in place on the image, with a live caret |
| Colour / opacity / width | One "style" button on the navigation panel opening a popup |
| Buttons on the navigation panel | Five: freehand, text, rectangle, clear, style |

## Architecture

Annotations are a **vector model stored in original-image coordinates**. A single renderer
draws that model twice: scaled to the current zoom for the screen, and at 1:1 into the
full-size pixel buffer when saving. Screen and file therefore cannot diverge, undo is a pop
from a vector, and memory cost is negligible — which matters because JPEGView opens images up
to 300000 pixels per side, making a full-size raster undo stack impossible.

Two rejected alternatives: a full-size ARGB overlay layer (undo would cost ~96 MB per step on
a 24 MP photo), and storing annotations in screen coordinates (they would detach from the
image on zoom and pan, and resample badly when saved at original size).

### New files

```
AnnotationTypes.h              the CAnnotation struct and the tool/type enums
AnnotationRenderer.h/cpp       the single drawing routine, GDI+
AnnotationCtl.h/cpp            input state machine, bound to CMainDlg (modelled on CCropCtl)
AnnotationStylePanel.h/cpp     the style popup panel (colour, opacity, width)
AnnotationStylePanelCtl.h/cpp  its controller
```

### Touched files

`MainDlg.h/cpp` (ownership, paint, mouse, keyboard, the save prompt, new commands),
`NavigationPanel.h/cpp` (five buttons), `JPEGImage.h/cpp` (burn-in),
`SettingsProvider.h/cpp` (four settings plus style write-back), `resource.h`, `JPEGView.rc`,
`HelpDisplayCtl.cpp`, and `Config/` (ini files, `KeyMap.txt.default` and `_ru`, `symbols.km`,
`strings_ru.txt`), plus the four `.vcxproj`/`.filters` variants.

## Data model

The codebase has a `CRectF` in `ZoomNavigator.h` but no float point type, so
`AnnotationTypes.h` defines a minimal one rather than leaking `Gdiplus::PointF` into
`JPEGImage.h`:

```cpp
struct CPointF { float x, y; };

enum EAnnotationType { AT_Freehand, AT_Text, AT_Rectangle };

struct CAnnotation {
    EAnnotationType      eType;
    COLORREF             color;
    int                  nAlpha;      // 0..255
    float                fPenWidth;   // image pixels
    bool                 bFilled;     // rectangle only
    std::vector<CPointF> points;      // freehand: polyline; rectangle: 2 corners; text: 1 anchor
    CString              sText;       // text only
    float                fFontHeight; // text only, image pixels
};
```

`CAnnotationCtl` holds `std::vector<CAnnotation> m_annotations` and
`std::vector<CAnnotation> m_redoStack`.

Pen width and font height are chosen by the user in **screen** pixels and divided by the
current zoom when the element is created. A stroke therefore keeps a constant apparent
thickness while drawing, and scales proportionally in the saved file.

Coordinates are relative to the image as currently displayed, including any 90° rotation
already applied, and are taken through `CMainDlg::ScreenToImage`.

## Rendering

```cpp
class CAnnotationRenderer {
public:
    static void Render(Gdiplus::Graphics& g, const std::vector<CAnnotation>& annotations,
                       float fScale, const Gdiplus::PointF& ptOrigin);
    static void RenderOne(Gdiplus::Graphics& g, const CAnnotation& annotation,
                          float fScale, const Gdiplus::PointF& ptOrigin);
};
```

This is the only place that draws annotations, and it is used from exactly two callers.

**Screen.** In `CMainDlg::OnPaint`, immediately after `DrawDIB32bppWithBlackBorders` returns
`ptDIBStart`, a `Gdiplus::Graphics` is created on the same `CPaintDC` and clipped to the image
rectangle. Panels paint afterwards and stay on top. GDI+ is already initialised in
`JPEGView.cpp` and linked, so no new dependency is introduced.

**While dragging.** Only the newly added segment is drawn straight into the window DC, the way
`CCropCtl::PaintCropRect` already updates the crop rectangle, instead of repainting the whole
image per mouse-move. Where consecutive segments overlap, the semi-transparent colour blends
twice and the joint looks slightly darker; the full repaint on mouse-up renders the stroke as a
single path and removes this.

## Burn-in and saving

```cpp
bool CJPEGImage::ApplyAnnotationsToOriginalPixels(const std::vector<CAnnotation>& annotations);
```

Modelled directly on the existing `RotateOriginalPixels` / `TrapezoidOriginalPixels` /
`ResizeOriginalPixels`: render into `m_pOrigPixels` through a `Gdiplus::Bitmap` constructed
over the existing buffer, then invalidate the cached DIBs so the next paint reflects the
change. If `GetOriginalChannelCount()` reports 3, the buffer is converted to 32 bpp first.

Writing the file then goes through the existing `CSaveImage::SaveImage(..., bFullSize = true)`,
so file formats, JPEG quality and EXIF handling are inherited rather than reimplemented.

### The save prompt

Shown whenever unsaved annotations exist and the user requests an action that would discard or
invalidate them:

* navigating to another image, opening a file, or closing the window
* rotating, cropping, resizing, or applying perspective correction

Geometry-changing operations are included because annotations are stored in image coordinates
and would otherwise end up in the wrong place. One rule covers all of them.

Buttons: **Overwrite** / **Save as…** / **Don't save** / **Cancel**. Cancel aborts the pending
action and leaves the user on the current image with annotations intact.

## Input

`CAnnotationCtl`, owned by `CMainDlg`, mirroring how `CCropCtl` is owned and called:

```cpp
enum EAnnotationTool { AT_None, AT_ToolFreehand, AT_ToolText, AT_ToolRectangle };

bool IsAnnotating();            // a tool is active
bool HasUnsavedAnnotations();
void SetTool(EAnnotationTool eTool);
bool OnLButtonDown(int nX, int nY);
bool OnMouseMove(int nX, int nY);
bool OnLButtonUp(int nX, int nY);
bool OnKeyDown(...);            // text entry and Esc
void OnPaint(CPaintDC& dc);
void Undo(); void Redo(); void Clear();
```

While a tool is active the cursor becomes a crosshair and left-drag draws. Panning stays
available on the middle button and the arrow keys, and the mouse wheel still zooms. `Esc`
leaves annotation mode, as does clicking the active tool's own button again — except for the
rectangle button, whose repeat click toggles fill (below), so that tool is left with `Esc` or by
choosing another tool.

**Rectangle fill.** `CUICtrl` exposes only `OnMouseLButton` and `OnMouseMove`; the panel
framework has no right-button routing at all. Rather than thread `WM_RBUTTONDOWN` through
`CUICtrl`, `CPanel` and `CPanelMgr` for the benefit of one button, clicking the rectangle
button while it is already active toggles outline ↔ filled. The button glyph shows the current
state and the tooltip names the next one.

**Text.** Clicking with the text tool creates a borderless child `CEdit` at that point, using
the current colour and font size, and focuses it. `Enter` or a click elsewhere commits the
typed string as an `AT_Text` annotation and destroys the control; `Esc` discards it. A real
edit control is used so that Backspace, selection, clipboard paste and non-Latin keyboard
layouts work without being reimplemented. `CEdit` cannot render a truly transparent background,
so while typing the text sits on a dim backing; once committed it is drawn by the renderer
directly over the image with no backing. `CMainDlg` already handles `WM_CTLCOLOREDIT`, which is
where the colour is applied.

## Navigation panel

Five buttons appended after `ID_btnShowInfo`, behind a gap: freehand, text, rectangle, clear
all, style. The active tool's button is drawn highlighted.

The style button shows a dot in the current colour at the current width, so both parameters are
readable without opening anything. Clicking it opens `CAnnotationStylePanel`, built from
existing controls: eight preset colour swatches, a `CSliderDouble` for opacity, a
`CSliderDouble` for width, and a "Other colour…" entry opening the system `ChooseColor` dialog.
When the text tool is active, the width slider sets font height instead of pen width, and is
labelled accordingly.

Undo and redo have no buttons; they are reachable by shortcut and from the context menu.

`CNavigationPanel::AdjustMaximalWidth` already scales the whole panel down to fit the window, so
five extra buttons shrink every button proportionally on narrow windows rather than overflowing.

## Commands

New IDs in `resource.h`, each carrying the `:KeyMap:` comment that makes it user-assignable:

```
IDM_ANNOTATE_FREEHAND      select the freehand tool
IDM_ANNOTATE_TEXT          select the text tool
IDM_ANNOTATE_RECT          select the rectangle tool / toggle fill
IDM_ANNOTATE_UNDO          undo the last annotation
IDM_ANNOTATE_REDO          redo
IDM_ANNOTATE_CLEAR         remove all annotations
IDM_ANNOTATE_OFF           leave annotation mode
IDM_ANNOTATE_APPLY_SAVE    burn annotations in and save
```

Colour, opacity and width are deliberately **not** commands, as agreed.

Default bindings are added to `KeyMap.txt.default` and `KeyMap_ru.txt.default`, chosen from
keys those files leave free — to be verified against the current file rather than assumed.

Note for testing: an installed JPEGView reads the user's own `%APPDATA%\JPEGView\KeyMap.txt`,
which will not contain the new bindings. Testing must use `StoreToEXEPath=true` in a scratch
folder so the user's personal keymap and ini are not touched.

## Settings

```ini
; Annotation defaults. Colour is R G B, as used by BackgroundColor.
AnnotationColor=255 0 0
AnnotationOpacity=70         ; percent, 0..100
AnnotationPenWidth=4         ; screen pixels
AnnotationFontSize=24        ; screen pixels
```

Read through the existing `GetColor` / `GetInt` helpers in `CSettingsProvider`.

The last style the user picked is remembered. `CSettingsProvider` gains a public
`SaveAnnotationStyle(COLORREF color, int nAlpha, int nPenWidth, int nFontSize)` that calls
`MakeSureUserINIExists()` and the existing private `WriteInt` / `WriteString`, following how
`SaveSettings` already persists processing parameters.

## Out of scope

Moving or deleting an individual annotation after it is drawn; arrows, ellipses, callouts,
dashed lines, font selection, blur-out regions; annotations surviving a restart. Annotations
are either burned into the file or lost.

## Testing

Unit-testable without a window: coordinate mapping at several zoom levels, undo/redo stack
behaviour, and that the renderer produces identical geometry at scale 1.0 and at a zoom factor
scaled back up.

Requiring the built binary, as in the transparent title bar work: each tool draws; annotations
stay pinned to the image under zoom and pan; the style popup changes colour, opacity and width;
the width slider switches to font size in text mode; undo, redo and clear behave; the save
prompt appears on navigation and on rotation; Overwrite and Save as… both produce a file whose
pixels match what was on screen; Don't save discards; Cancel keeps the user in place.

CI builds Win32 and x64 in Release plus both Debug test configurations.
