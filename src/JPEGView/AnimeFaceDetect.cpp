#include <windows.h>
#include <atlbase.h>
#include <atlstr.h>
#include "onnxruntime/include/onnxruntime_c_api.h"
#include "AnimeFaceDetect.h"

// The detector is a YOLOv8 model trained on drawn faces (deepghs/anime_face_detection,
// MIT), run by onnxruntime (MIT). Both sit next to JPEGView.exe and are loaded on first
// use by full path, so neither is imported and a missing one only means no face.
namespace {

	const wchar_t* RUNTIME_DLL = L"onnxruntime.dll";
	const wchar_t* MODEL_FILE = L"face_detect.onnx";
	const int SQUARE = 640; // the size the model was trained at
	const float PAD_VALUE = 114.0f / 255; // the grey the training letterboxed with
	const double THRESHOLD = 0.278; // the model's own best F1 threshold
	const double FALLBACK = 0.12; // a single less certain face still counts
	const double MAX_IOU = 0.5;

	struct SRuntime {
		bool bTried;
		const OrtApi* pApi;
		OrtEnv* pEnv;
		OrtSession* pSession;
	};

	SRuntime g_runtime = { false, NULL, NULL, NULL };

	// True when the call succeeded; a failure's status is released.
	bool Ok(OrtStatus* pStatus) {
		if (pStatus == NULL) {
			return true;
		}
		g_runtime.pApi->ReleaseStatus(pStatus);
		return false;
	}

	CStringW ExeDir() {
		wchar_t sPath[MAX_PATH];
		DWORD nLength = ::GetModuleFileNameW(NULL, sPath, MAX_PATH);
		CStringW sDir(sPath, (int)nLength);
		int nSlash = sDir.ReverseFind(L'\\');
		return (nSlash >= 0) ? sDir.Left(nSlash + 1) : CStringW();
	}

	bool LoadRuntime() {
		if (g_runtime.bTried) {
			return g_runtime.pSession != NULL;
		}
		g_runtime.bTried = true;
		CStringW sDir = ExeDir();
		if (::GetFileAttributesW(sDir + MODEL_FILE) == INVALID_FILE_ATTRIBUTES) {
			return false;
		}
		// By full path: Windows 11 has an older onnxruntime.dll of its own in System32. A
		// system too old for this one must fail quietly, not with a message box.
		UINT nOldMode = ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
		HMODULE hRuntime = ::LoadLibraryExW(sDir + RUNTIME_DLL, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
		::SetErrorMode(nOldMode);
		if (hRuntime == NULL) {
			return false;
		}
		typedef const OrtApiBase* (ORT_API_CALL *PGetApiBase)();
		PGetApiBase pGetApiBase = (PGetApiBase)::GetProcAddress(hRuntime, "OrtGetApiBase");
		if (pGetApiBase == NULL || pGetApiBase() == NULL) {
			return false;
		}
		g_runtime.pApi = pGetApiBase()->GetApi(ORT_API_VERSION);
		if (g_runtime.pApi == NULL) {
			return false;
		}
		const OrtApi* pApi = g_runtime.pApi;
		OrtSessionOptions* pOptions = NULL;
		if (!Ok(pApi->CreateEnv(ORT_LOGGING_LEVEL_ERROR, "JPEGView", &g_runtime.pEnv)) ||
			!Ok(pApi->CreateSessionOptions(&pOptions))) {
			return false;
		}
		Ok(pApi->SetSessionGraphOptimizationLevel(pOptions, ORT_ENABLE_ALL));
		CStringW sModel = sDir + MODEL_FILE;
		if (!Ok(pApi->CreateSession(g_runtime.pEnv, sModel, pOptions, &g_runtime.pSession))) {
			g_runtime.pSession = NULL;
		}
		pApi->ReleaseSessionOptions(pOptions);
		return g_runtime.pSession != NULL;
	}

	// The image letterboxed into the square as the model takes it: RGB planes of 0..1,
	// each target pixel the average of the source pixels it covers.
	void Letterbox(const BYTE* pPixels, int nWidth, int nHeight, int nChannels, int nStride,
		const FaceMath::SLetterbox& letterbox, float* pInput) {
		const int nPlane = SQUARE * SQUARE;
		for (int i = 0; i < 3 * nPlane; i++) {
			pInput[i] = PAD_VALUE;
		}
		for (int ty = 0; ty < letterbox.nHeight; ty++) {
			int y0 = (int)((__int64)ty * nHeight / letterbox.nHeight);
			int y1 = max(y0 + 1, (int)((__int64)(ty + 1) * nHeight / letterbox.nHeight));
			for (int tx = 0; tx < letterbox.nWidth; tx++) {
				int x0 = (int)((__int64)tx * nWidth / letterbox.nWidth);
				int x1 = max(x0 + 1, (int)((__int64)(tx + 1) * nWidth / letterbox.nWidth));
				unsigned int nSum[3] = { 0, 0, 0 };
				for (int y = y0; y < y1; y++) {
					const BYTE* p = pPixels + (size_t)y * nStride + (size_t)x0 * nChannels;
					for (int x = x0; x < x1; x++, p += nChannels) {
						if (nChannels == 1) {
							nSum[0] += p[0]; nSum[1] += p[0]; nSum[2] += p[0];
						} else {
							nSum[0] += p[2]; nSum[1] += p[1]; nSum[2] += p[0]; // BGR to RGB
						}
					}
				}
				float fCount = 255.0f * (y1 - y0) * (x1 - x0);
				int nIndex = (ty + letterbox.nPadY) * SQUARE + tx + letterbox.nPadX;
				pInput[nIndex] = nSum[0] / fCount;
				pInput[nPlane + nIndex] = nSum[1] / fCount;
				pInput[2 * nPlane + nIndex] = nSum[2] / fCount;
			}
		}
	}
}

namespace AnimeFaceDetect {

std::vector<FaceMath::SFace> Detect(const void* pPixels, int nWidth, int nHeight, int nChannels, int nStride) {
	std::vector<FaceMath::SScoredFace> faces;
	if (pPixels == NULL || nWidth <= 0 || nHeight <= 0 || (nChannels != 1 && nChannels != 3 && nChannels != 4)) {
		return std::vector<FaceMath::SFace>();
	}
	if (!LoadRuntime()) {
		return std::vector<FaceMath::SFace>();
	}
	const OrtApi* pApi = g_runtime.pApi;
	FaceMath::SLetterbox letterbox = FaceMath::Letterbox(nWidth, nHeight, SQUARE);
	std::vector<float> input((size_t)3 * SQUARE * SQUARE);
	Letterbox((const BYTE*)pPixels, nWidth, nHeight, nChannels, nStride, letterbox, &input[0]);

	OrtMemoryInfo* pMemory = NULL;
	OrtValue* pInput = NULL;
	OrtValue* pOutput = NULL;
	const int64_t shape[4] = { 1, 3, SQUARE, SQUARE };
	const char* sInputName = "images";
	const char* sOutputName = "output0";
	if (Ok(pApi->CreateCpuMemoryInfo(OrtArenaAllocator, OrtMemTypeDefault, &pMemory)) &&
		Ok(pApi->CreateTensorWithDataAsOrtValue(pMemory, &input[0], input.size() * sizeof(float), shape, 4,
			ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, &pInput)) &&
		Ok(pApi->Run(g_runtime.pSession, NULL, &sInputName, &pInput, 1, &sOutputName, 1, &pOutput))) {
		// The output is 1 x 5 x anchors: centre x, centre y, width, height, score.
		OrtTensorTypeAndShapeInfo* pInfo = NULL;
		size_t nDims = 0;
		int64_t dims[3] = { 0, 0, 0 };
		float* pData = NULL;
		if (Ok(pApi->GetTensorTypeAndShape(pOutput, &pInfo))) {
			if (Ok(pApi->GetDimensionsCount(pInfo, &nDims)) && nDims == 3) {
				Ok(pApi->GetDimensions(pInfo, dims, 3));
			}
			pApi->ReleaseTensorTypeAndShapeInfo(pInfo);
		}
		if (nDims == 3 && dims[0] == 1 && dims[1] == 5 && dims[2] > 0 && Ok(pApi->GetTensorMutableData(pOutput, (void**)&pData))) {
			faces = FaceMath::SuppressOverlaps(FaceMath::DecodeDetections(pData, (int)dims[2], letterbox, FALLBACK), MAX_IOU);
		}
	}
	if (pOutput != NULL) pApi->ReleaseValue(pOutput);
	if (pInput != NULL) pApi->ReleaseValue(pInput);
	if (pMemory != NULL) pApi->ReleaseMemoryInfo(pMemory);
	return FaceMath::SelectFaces(faces, THRESHOLD, FALLBACK);
}

}
