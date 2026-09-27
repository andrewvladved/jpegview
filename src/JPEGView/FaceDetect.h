// Finds faces in an image with the face detector Windows 10 and later have built in
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "FaceMath.h"

namespace FaceDetect {

	// Faces in the pixels, in their own coordinates. The pixels are rows of nStride bytes
	// with 1 (grey), 3 (BGR) or 4 (BGRA) bytes per pixel. Empty when there is no face, and
	// on systems without the detector (anything before Windows 10) - never an error.
	std::vector<FaceMath::SFace> Detect(const void* pPixels, int nWidth, int nHeight, int nChannels, int nStride);
}
