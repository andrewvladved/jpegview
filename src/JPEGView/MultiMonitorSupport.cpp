#include "StdAfx.h"
#include "MultiMonitorSupport.h"
#include "SettingsProvider.h"
#include "WindowMath.h"

struct EnumMonitorParams {
	EnumMonitorParams(int nIndexMonitor) {
		IndexMonitor = nIndexMonitor;
		NumPixels = 0;
		Iterations = 0;
		rectMonitor = CRect(0, 0, 0, 0);
	}

	int Iterations;
	int IndexMonitor;
	int NumPixels;
	CRect rectMonitor;
};

static CRect defaultWindowRect = CRect(0, 0, 0, 0);

bool CMultiMonitorSupport::IsMultiMonitorSystem() {
	return ::GetSystemMetrics(SM_CMONITORS) > 1;
}

CRect CMultiMonitorSupport::GetVirtualDesktop() {
	return CRect(CPoint(::GetSystemMetrics(SM_XVIRTUALSCREEN), ::GetSystemMetrics(SM_YVIRTUALSCREEN)),
		CSize(::GetSystemMetrics(SM_CXVIRTUALSCREEN), ::GetSystemMetrics(SM_CYVIRTUALSCREEN)));
}

struct CoveredMonitorParams {
	CoveredMonitorParams(const CRect& rect) {
		rectWindow = rect;
		NumCovered = 0;
		rectWork = CRect(0, 0, 0, 0);
	}

	CRect rectWindow;
	int NumCovered;
	CRect rectWork;
};

// Counts the monitors a window rectangle reaches onto, keeping the work area of the last one
static BOOL CALLBACK CoveredMonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
	CoveredMonitorParams* pParams = (CoveredMonitorParams*) dwData;
	MONITORINFO monitorInfo;
	monitorInfo.cbSize = sizeof(MONITORINFO);
	if (::GetMonitorInfo(hMonitor, &monitorInfo)) {
		CRect rectMonitor(monitorInfo.rcMonitor);
		CRect intersection;
		if (intersection.IntersectRect(&pParams->rectWindow, &rectMonitor)) {
			pParams->NumCovered += 1;
			pParams->rectWork = CRect(monitorInfo.rcWork);
		}
	}
	return TRUE;
}

// Callback called during enumeration of monitors
static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
	MONITORINFO monitorInfo;
	monitorInfo.cbSize = sizeof(MONITORINFO);
	EnumMonitorParams* pParams = (EnumMonitorParams*) dwData;
	if (pParams->IndexMonitor == -1) {
		// Use the monitor with largest number of pixels
		int nNumPixels = (lprcMonitor->right - lprcMonitor->left)*(lprcMonitor->bottom - lprcMonitor->top);
		if (nNumPixels > pParams->NumPixels) {
			pParams->NumPixels = nNumPixels;
			pParams->rectMonitor = CRect(lprcMonitor);
		} else if (nNumPixels == pParams->NumPixels) {
			// if same size take primary
			::GetMonitorInfo(hMonitor, &monitorInfo);
			if (monitorInfo.dwFlags & MONITORINFOF_PRIMARY) {
				pParams->rectMonitor = CRect(lprcMonitor);
			}
		}
	} else {
		::GetMonitorInfo(hMonitor, &monitorInfo);
		if (pParams->IndexMonitor == 0) {
			// take primary monitor
			if (monitorInfo.dwFlags & MONITORINFOF_PRIMARY) {
				pParams->rectMonitor = CRect(lprcMonitor);
				return FALSE;
			}
		} else {
			// take the i-th non primary monitor
			if (!(monitorInfo.dwFlags & MONITORINFOF_PRIMARY)) {
				pParams->Iterations += 1;
				if (pParams->IndexMonitor == pParams->Iterations) {
					pParams->rectMonitor = CRect(lprcMonitor);
					return FALSE;
				}
			}
		}
	}
	return TRUE;
}

CRect CMultiMonitorSupport::GetMonitorRect(int nIndex) {
	if (!CMultiMonitorSupport::IsMultiMonitorSystem()) {
		return CRect(0, 0, ::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN));
	}
	EnumMonitorParams params(nIndex);
	::EnumDisplayMonitors(NULL, NULL, &MonitorEnumProc, (LPARAM) &params);
	if (params.rectMonitor.Width() == 0) {
		return CRect(0, 0, ::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN));
	} else {
		return params.rectMonitor;
	}
}

CRect CMultiMonitorSupport::GetMonitorRect(HWND hWnd) {
	HMONITOR hMonitor = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO monitorInfo;
	monitorInfo.cbSize = sizeof(MONITORINFO);
	::GetMonitorInfo(hMonitor, &monitorInfo);
	return CRect(monitorInfo.rcMonitor);
}

CRect CMultiMonitorSupport::GetWorkingRect(HWND hWnd) {
	HMONITOR hMonitor = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO monitorInfo;
	monitorInfo.cbSize = sizeof(MONITORINFO);
	::GetMonitorInfo(hMonitor, &monitorInfo);
	return CRect(monitorInfo.rcWork);
}

CRect CMultiMonitorSupport::GetDefaultWindowRect() {
	CSettingsProvider& settings = CSettingsProvider::This();
	CRect windowRect = !defaultWindowRect.IsRectEmpty() ? defaultWindowRect : settings.StickyWindowSize() ? settings.StickyWindowRect() : settings.DefaultWindowRect();
	// A remembered rectangle can be larger than the screen it comes back to - it was saved on
	// a bigger screen, or while the window was covering the taskbar - and would put the window
	// over the taskbar again. It is brought back inside the work area, but only when it lies on
	// a single monitor: a window deliberately stretched across several screens is left as it is,
	// since there is no one work area to measure it against.
	if (!windowRect.IsRectEmpty()) {
		CoveredMonitorParams params(windowRect);
		::EnumDisplayMonitors(NULL, NULL, CoveredMonitorEnumProc, (LPARAM)&params);
		if (params.NumCovered == 1 && !params.rectWork.IsRectEmpty()) {
			// A window cannot be smaller than MinimalWindowSize: the window would be grown to it
			// afterwards, from its top left corner, and pushed back over the taskbar. So the
			// size it will really have is what gets placed.
			CSize minimalSize = settings.MinimalWindowSize();
			if (windowRect.Width() < minimalSize.cx) {
				windowRect.right = windowRect.left + minimalSize.cx;
			}
			if (windowRect.Height() < minimalSize.cy) {
				windowRect.bottom = windowRect.top + minimalSize.cy;
			}
			windowRect = CRect(WindowMath::ClampToWorkArea(windowRect, params.rectWork));
		}
	}
	CRect rectAllScreens = CMultiMonitorSupport::GetVirtualDesktop();
	if (windowRect.IsRectEmpty() || !rectAllScreens.IntersectRect(&rectAllScreens, &windowRect)) {
		CRect monitorRect = CMultiMonitorSupport::GetMonitorRect(settings.DisplayMonitor());
		int nDesiredWidth = monitorRect.Width()*2/3;
		int nDesiredHeight = nDesiredWidth*3/4;
		CSize borderSize = Helpers::GetTotalBorderSize();
		nDesiredWidth += borderSize.cx;
		nDesiredHeight += borderSize.cy;
		windowRect = CRect(CPoint(monitorRect.left + (monitorRect.Width() - nDesiredWidth) / 2, monitorRect.top + (monitorRect.Height() - nDesiredHeight) / 2), CSize(nDesiredWidth, nDesiredHeight));
	}
	return windowRect;
}

void CMultiMonitorSupport::SetDefaultWindowRect(CRect rect) {
	defaultWindowRect = rect;
}

CRect CMultiMonitorSupport::GetDefaultClientRectInWindowMode(bool bAutoFitWndToImage) {
	if (bAutoFitWndToImage) {
		CRect monitorRect = CMultiMonitorSupport::GetMonitorRect(CSettingsProvider::This().DisplayMonitor());
		return CRect(0, 0, monitorRect.Width(), monitorRect.Height());
	}
	CRect wndRect = CMultiMonitorSupport::GetDefaultWindowRect();
	CSize borderSize = Helpers::GetTotalBorderSize();
	return CRect(0, 0, wndRect.Width() - borderSize.cx, wndRect.Height() - borderSize.cy);
}
