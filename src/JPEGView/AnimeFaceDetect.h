// Finds faces in drawn images - anime, illustrations, pixel art - with a neural detector
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "FaceMath.h"

namespace AnimeFaceDetect {

	// Faces in the pixels, in their own coordinates: every one the detector is sure of, or
	// else the single best less certain one. The pixels are rows of nStride bytes with 1 (grey), 3 (BGR) or 4 (BGRA) bytes per pixel.
	// Empty when onnxruntime.dll or the model is missing next to JPEGView.exe, or does not
	// load on this system - never an error.
	std::vector<FaceMath::SFace> Detect(const void* pPixels, int nWidth, int nHeight, int nChannels, int nStride);
}
