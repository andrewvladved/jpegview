// Two detectors look at the image. The neural one (AnimeFaceDetect) knows drawn faces. The
// one Windows 10 and later have built in knows photographs; it is a Windows Runtime class
// this file talks to through the plain ABI, finding combase.dll at run time, so JPEGView
// imports nothing new and still starts on systems that have no Windows Runtime at all.
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#undef NTDDI_VERSION
#define NTDDI_VERSION 0x0A000000
#include <windows.h>
#include <atlbase.h>

#include "FaceDetect.h"
#include "AnimeFaceDetect.h"

// The Windows XP build's SDK has no Windows Runtime headers; it gets the neural detector only.
#if defined(__has_include)
#if __has_include(<windows.media.faceanalysis.h>)
#define FACE_DETECT_WINRT
#endif
#endif

#ifdef FACE_DETECT_WINRT
#include <roapi.h>
#include <robuffer.h>
#include <windows.foundation.h>
#include <windows.foundation.collections.h>
#include <windows.graphics.imaging.h>
#include <windows.media.faceanalysis.h>
#include <windows.storage.streams.h>

using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Graphics::Imaging;
using namespace ABI::Windows::Media::FaceAnalysis;
using namespace ABI::Windows::Storage::Streams;

namespace {

	// The detector finds faces down to a small size in pixels, so a large photo is only
	// shrunk to this much on its long side - enough for small faces in a group, and fast.
	const int MAX_DETECT_SIZE = 1600;
	const DWORD TIMEOUT_MS = 4000;

	typedef HRESULT (WINAPI *PRoGetActivationFactory)(HSTRING, REFIID, void**);
	typedef HRESULT (WINAPI *PWindowsCreateString)(PCNZWCH, UINT32, HSTRING*);
	typedef HRESULT (WINAPI *PWindowsDeleteString)(HSTRING);

	struct SRuntime {
		PRoGetActivationFactory pGetFactory;
		PWindowsCreateString pCreateString;
		PWindowsDeleteString pDeleteString;
	};

	bool LoadRuntime(SRuntime& runtime) {
		HMODULE hCombase = ::LoadLibraryW(L"combase.dll");
		if (hCombase == NULL) {
			return false;
		}
		runtime.pGetFactory = (PRoGetActivationFactory)::GetProcAddress(hCombase, "RoGetActivationFactory");
		runtime.pCreateString = (PWindowsCreateString)::GetProcAddress(hCombase, "WindowsCreateString");
		runtime.pDeleteString = (PWindowsDeleteString)::GetProcAddress(hCombase, "WindowsDeleteString");
		return runtime.pGetFactory != NULL && runtime.pCreateString != NULL && runtime.pDeleteString != NULL;
	}

	template <class TFactory>
	HRESULT GetFactory(const SRuntime& runtime, const wchar_t* sClassName, TFactory** ppFactory) {
		HSTRING hClassName = NULL;
		HRESULT hr = runtime.pCreateString(sClassName, (UINT32)wcslen(sClassName), &hClassName);
		if (FAILED(hr)) {
			return hr;
		}
		hr = runtime.pGetFactory(hClassName, __uuidof(TFactory), (void**)ppFactory);
		runtime.pDeleteString(hClassName);
		return hr;
	}

	// Waits for an asynchronous operation on this (multithreaded) apartment and takes its result.
	template <class TOperation, class TResult>
	HRESULT Await(TOperation* pOperation, TResult* pResult) {
		CComPtr<IAsyncInfo> pInfo;
		HRESULT hr = pOperation->QueryInterface(__uuidof(IAsyncInfo), (void**)&pInfo);
		if (FAILED(hr)) {
			return hr;
		}
		DWORD nStart = ::GetTickCount();
		for (;;) {
			AsyncStatus eStatus;
			hr = pInfo->get_Status(&eStatus);
			if (FAILED(hr)) {
				return hr;
			}
			if ((int)eStatus != 0) { // no longer started: completed, cancelled or failed
				break;
			}
			if (::GetTickCount() - nStart > TIMEOUT_MS) {
				pInfo->Cancel();
				return E_ABORT;
			}
			::Sleep(1);
		}
		return pOperation->GetResults(pResult);
	}

	struct SJob {
		BYTE* pGrey;
		int nWidth, nHeight;
		std::vector<FaceMath::SFace> faces;
		LONG nRefs;
	};

	void Release(SJob* pJob) {
		if (::InterlockedDecrement(&pJob->nRefs) == 0) {
			delete[] pJob->pGrey;
			delete pJob;
		}
	}

	void DetectGrey(SJob* pJob) {
		SRuntime runtime;
		if (!LoadRuntime(runtime)) {
			return;
		}
		CComPtr<IFaceDetectorStatics> pDetectorStatics;
		if (FAILED(GetFactory(runtime, RuntimeClass_Windows_Media_FaceAnalysis_FaceDetector, &pDetectorStatics))) {
			return;
		}
		boolean bSupported = false;
		if (FAILED(pDetectorStatics->get_IsSupported(&bSupported)) || !bSupported) {
			return;
		}

		// The grey pixels go into a Windows Runtime buffer, and a bitmap is made from it.
		UINT32 nSize = (UINT32)(pJob->nWidth * pJob->nHeight);
		CComPtr<IBufferFactory> pBufferFactory;
		CComPtr<IBuffer> pBuffer;
		if (FAILED(GetFactory(runtime, RuntimeClass_Windows_Storage_Streams_Buffer, &pBufferFactory)) ||
			FAILED(pBufferFactory->Create(nSize, &pBuffer)) || FAILED(pBuffer->put_Length(nSize))) {
			return;
		}
		CComPtr<::Windows::Storage::Streams::IBufferByteAccess> pByteAccess;
		byte* pBytes = NULL;
		if (FAILED(pBuffer->QueryInterface(__uuidof(::Windows::Storage::Streams::IBufferByteAccess), (void**)&pByteAccess)) ||
			FAILED(pByteAccess->Buffer(&pBytes)) || pBytes == NULL) {
			return;
		}
		memcpy(pBytes, pJob->pGrey, nSize);
		CComPtr<ISoftwareBitmapStatics> pBitmapStatics;
		CComPtr<ISoftwareBitmap> pBitmap;
		if (FAILED(GetFactory(runtime, RuntimeClass_Windows_Graphics_Imaging_SoftwareBitmap, &pBitmapStatics)) ||
			FAILED(pBitmapStatics->CreateCopyFromBuffer(pBuffer, BitmapPixelFormat_Gray8, pJob->nWidth, pJob->nHeight, &pBitmap))) {
			return;
		}

		CComPtr<__FIAsyncOperation_1_Windows__CMedia__CFaceAnalysis__CFaceDetector> pCreate;
		CComPtr<IFaceDetector> pDetector;
		if (FAILED(pDetectorStatics->CreateAsync(&pCreate)) || FAILED(Await(pCreate.p, &pDetector)) || pDetector == NULL) {
			return;
		}
		CComPtr<__FIAsyncOperation_1___FIVector_1_Windows__CMedia__CFaceAnalysis__CDetectedFace> pDetect;
		CComPtr<__FIVector_1_Windows__CMedia__CFaceAnalysis__CDetectedFace> pFaces;
		if (FAILED(pDetector->DetectFacesAsync(pBitmap, &pDetect)) || FAILED(Await(pDetect.p, &pFaces)) || pFaces == NULL) {
			return;
		}
		unsigned int nFaces = 0;
		pFaces->get_Size(&nFaces);
		for (unsigned int i = 0; i < nFaces; i++) {
			CComPtr<IDetectedFace> pFace;
			BitmapBounds box;
			if (SUCCEEDED(pFaces->GetAt(i, &pFace)) && pFace != NULL && SUCCEEDED(pFace->get_FaceBox(&box))) {
				FaceMath::SFace face = { (double)box.X, (double)box.Y, (double)box.Width, (double)box.Height };
				pJob->faces.push_back(face);
			}
		}
	}

	DWORD WINAPI DetectThread(void* pParameter) {
		SJob* pJob = (SJob*)pParameter;
		// The detector's asynchronous calls may only be waited for off the user interface
		// thread, in the multithreaded apartment this thread joins.
		if (SUCCEEDED(::CoInitializeEx(NULL, COINIT_MULTITHREADED))) {
			DetectGrey(pJob);
			::CoUninitialize();
		}
		Release(pJob);
		return 0;
	}
}

#endif // FACE_DETECT_WINRT

namespace FaceDetect {

std::vector<FaceMath::SFace> Detect(const void* pPixels, int nWidth, int nHeight, int nChannels, int nStride) {
	std::vector<FaceMath::SFace> faces;
	if (pPixels == NULL || nWidth <= 0 || nHeight <= 0 || (nChannels != 1 && nChannels != 3 && nChannels != 4)) {
		return faces;
	}
#ifndef FACE_DETECT_WINRT
	return AnimeFaceDetect::Detect(pPixels, nWidth, nHeight, nChannels, nStride);
#else

	// Shrink to grey. Nearest pixel is plenty for finding faces.
	double dScale = min(1.0, (double)MAX_DETECT_SIZE / max(nWidth, nHeight));
	int nGreyWidth = max(1, (int)(nWidth * dScale + 0.5));
	int nGreyHeight = max(1, (int)(nHeight * dScale + 0.5));
	SJob* pJob = new SJob();
	pJob->pGrey = new BYTE[nGreyWidth * nGreyHeight];
	pJob->nWidth = nGreyWidth;
	pJob->nHeight = nGreyHeight;
	pJob->nRefs = 2;
	const BYTE* pSource = (const BYTE*)pPixels;
	for (int y = 0; y < nGreyHeight; y++) {
		const BYTE* pRow = pSource + (size_t)min(nHeight - 1, (int)((y + 0.5) * nHeight / nGreyHeight)) * nStride;
		BYTE* pTarget = pJob->pGrey + (size_t)y * nGreyWidth;
		for (int x = 0; x < nGreyWidth; x++) {
			const BYTE* p = pRow + (size_t)min(nWidth - 1, (int)((x + 0.5) * nWidth / nGreyWidth)) * nChannels;
			pTarget[x] = (nChannels == 1) ? p[0] : (BYTE)((p[0] * 29 + p[1] * 150 + p[2] * 77) >> 8);
		}
	}

	// The photo detector runs on its own thread while the neural one runs on this one.
	HANDLE hThread = ::CreateThread(NULL, 0, DetectThread, pJob, 0, NULL);
	if (hThread == NULL) {
		Release(pJob);
	}
	std::vector<FaceMath::SFace> drawnFaces = AnimeFaceDetect::Detect(pPixels, nWidth, nHeight, nChannels, nStride);
	// A detector that does not answer in time is left to finish on its own; the job is
	// freed by whichever side lets go of it last.
	if (hThread != NULL) {
		if (::WaitForSingleObject(hThread, TIMEOUT_MS + 1000) == WAIT_OBJECT_0) {
			double dBackX = (double)nWidth / nGreyWidth;
			double dBackY = (double)nHeight / nGreyHeight;
			for (size_t i = 0; i < pJob->faces.size(); i++) {
				const FaceMath::SFace& face = pJob->faces[i];
				FaceMath::SFace scaled = { face.dX * dBackX, face.dY * dBackY, face.dWidth * dBackX, face.dHeight * dBackY };
				faces.push_back(scaled);
			}
		}
		::CloseHandle(hThread);
	}
	Release(pJob);
	// The neural boxes come first: they frame drawn faces better, and a face both found
	// is kept once.
	return FaceMath::MergeFaces(drawnFaces, faces);
#endif
}

namespace {
	struct SAsyncJob {
		HWND hWnd;
		UINT nMessage;
		WPARAM wParam;
		BYTE* pPixels;
		int nWidth, nHeight, nChannels, nStride;
	};

	DWORD WINAPI DetectAsyncThread(void* pParameter) {
		SAsyncJob* pJob = (SAsyncJob*)pParameter;
		std::vector<FaceMath::SFace>* pFaces = new std::vector<FaceMath::SFace>(
			Detect(pJob->pPixels, pJob->nWidth, pJob->nHeight, pJob->nChannels, pJob->nStride));
		// A window closed in the meantime takes no message, and then the faces go here.
		if (!::PostMessage(pJob->hWnd, pJob->nMessage, pJob->wParam, (LPARAM)pFaces)) {
			delete pFaces;
		}
		delete[] pJob->pPixels;
		delete pJob;
		return 0;
	}
}

void DetectAsync(HWND hWnd, UINT nMessage, WPARAM wParam, const void* pPixels, int nWidth, int nHeight, int nChannels, int nStride) {
	SAsyncJob* pJob = new SAsyncJob();
	pJob->hWnd = hWnd;
	pJob->nMessage = nMessage;
	pJob->wParam = wParam;
	pJob->nWidth = nWidth;
	pJob->nHeight = nHeight;
	pJob->nChannels = nChannels;
	pJob->nStride = nStride;
	pJob->pPixels = NULL;
	if (pPixels != NULL && nWidth > 0 && nHeight > 0 && nStride > 0) {
		size_t nSize = (size_t)nStride * nHeight;
		pJob->pPixels = new BYTE[nSize];
		memcpy(pJob->pPixels, pPixels, nSize);
	}
	HANDLE hThread = ::CreateThread(NULL, 0, DetectAsyncThread, pJob, 0, NULL);
	if (hThread == NULL) {
		DetectAsyncThread(pJob); // no thread to be had: search here, the answer is still posted
	} else {
		::CloseHandle(hThread);
	}
}

}
