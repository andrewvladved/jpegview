// Finds faces in an image, drawn or photographed
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "FaceMath.h"

namespace FaceDetect {

	// Faces in the pixels, in their own coordinates, from the neural detector for drawn
	// faces and the one Windows 10 and later have built in for photographs. The pixels are
	// rows of nStride bytes with 1 (grey), 3 (BGR) or 4 (BGRA) bytes per pixel. Empty when
	// there is no face, and where neither detector is available - never an error.
	std::vector<FaceMath::SFace> Detect(const void* pPixels, int nWidth, int nHeight, int nChannels, int nStride);
}
