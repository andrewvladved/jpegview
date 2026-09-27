#include "FaceMath.h"

namespace FaceMath {

int PickFace(const std::vector<SFace>& faces, bool bSmallest) {
	return -1;
}

SOffset LimitOffset(SOffset offset, SIZE imageSize, double dZoom, SIZE windowSize) {
	return offset;
}

SOffset CenterOffset(double dPointX, double dPointY, SIZE imageSize, double dZoom, SIZE windowSize) {
	SOffset offset = { 0, 0 };
	return offset;
}

SOffset AnchorOffset(double dPointX, double dPointY, SIZE imageSize, double dStartZoom, SOffset startOffset,
	double dZoom, SIZE windowSize) {
	return startOffset;
}

}
