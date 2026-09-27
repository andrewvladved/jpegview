#include "TestFramework.h"
#include "FaceMath.h"

static FaceMath::SFace Face(double dX, double dY, double dWidth, double dHeight) {
	FaceMath::SFace face = { dX, dY, dWidth, dHeight };
	return face;
}

static SIZE Size(int nWidth, int nHeight) {
	SIZE size = { nWidth, nHeight };
	return size;
}

// Where an image point lands on screen, the arithmetic CMainDlg draws with.
static double ScreenPos(double dPoint, int nImage, double dZoom, double dOffset, int nWindow) {
	return nWindow / 2.0 + dOffset + (dPoint - nImage / 2.0) * dZoom;
}

TEST(PickFaceFindsNothingWithoutFaces) {
	std::vector<FaceMath::SFace> faces;
	CHECK(FaceMath::PickFace(faces, true) == -1);
	CHECK(FaceMath::PickFace(faces, false) == -1);
}

TEST(PickFaceChoosesTheSmallestOrTheLargestByArea) {
	std::vector<FaceMath::SFace> faces;
	faces.push_back(Face(10, 10, 50, 50));   // 2500
	faces.push_back(Face(200, 30, 20, 30));  // 600
	faces.push_back(Face(400, 40, 90, 80));  // 7200
	faces.push_back(Face(600, 50, 40, 40));  // 1600
	CHECK(FaceMath::PickFace(faces, true) == 1);
	CHECK(FaceMath::PickFace(faces, false) == 2);
}

TEST(CenterOffsetBringsThePointToTheMiddleOfTheWindow) {
	// 1000 x 1000 at 2x is 2000 x 2000 on screen, far more than the 800 x 600 window.
	FaceMath::SOffset offset = FaceMath::CenterOffset(300, 400, Size(1000, 1000), 2.0, Size(800, 600));
	CHECK_NEAR(offset.dX, 400, 0.001);
	CHECK_NEAR(offset.dY, 200, 0.001);
	CHECK_NEAR(ScreenPos(300, 1000, 2.0, offset.dX, 800), 400, 0.001);
	CHECK_NEAR(ScreenPos(400, 1000, 2.0, offset.dY, 600), 300, 0.001);
}

TEST(CenterOffsetStopsAtTheEdgesOfTheImage) {
	// A face in the top left corner can only come as far as the image edge allows:
	// (2000 - 800) / 2 and (2000 - 600) / 2.
	FaceMath::SOffset offset = FaceMath::CenterOffset(10, 10, Size(1000, 1000), 2.0, Size(800, 600));
	CHECK_NEAR(offset.dX, 600, 0.001);
	CHECK_NEAR(offset.dY, 700, 0.001);
}

TEST(CenterOffsetIsZeroWhileTheImageFitsTheWindow) {
	// At fit to screen there is nothing to pan, so the image stays centred.
	FaceMath::SOffset offset = FaceMath::CenterOffset(100, 900, Size(1000, 1000), 0.6, Size(800, 600));
	CHECK_NEAR(offset.dX, 0, 0.001);
	CHECK_NEAR(offset.dY, 0, 0.001);
}

TEST(LimitOffsetKeepsWhatIsWithinTheEdges) {
	FaceMath::SOffset offset = { -150, 90 };
	FaceMath::SOffset limited = FaceMath::LimitOffset(offset, Size(1000, 500), 1.0, Size(800, 400));
	CHECK_NEAR(limited.dX, -100, 0.001);
	CHECK_NEAR(limited.dY, 50, 0.001);
}

TEST(AnchorOffsetStartsWhereTheRunStarts) {
	FaceMath::SOffset start = { 120, -40 };
	FaceMath::SOffset offset = FaceMath::AnchorOffset(300, 700, Size(1000, 1000), 2.0, start, 2.0, Size(800, 600));
	CHECK_NEAR(offset.dX, 120, 0.001);
	CHECK_NEAR(offset.dY, -40, 0.001);
}

TEST(AnchorOffsetKeepsTheFaceWhereItIsOnScreen) {
	// From fit to screen the face stays on its spot while the image grows around it.
	FaceMath::SOffset start = { 0, 0 };
	double dStartZoom = 0.6;
	double dPointX = 300, dPointY = 400;
	FaceMath::SOffset offset = FaceMath::AnchorOffset(dPointX, dPointY, Size(1000, 1000), dStartZoom, start, 3.0, Size(800, 600));
	CHECK_NEAR(ScreenPos(dPointX, 1000, 3.0, offset.dX, 800), ScreenPos(dPointX, 1000, dStartZoom, 0, 800), 0.001);
	CHECK_NEAR(ScreenPos(dPointY, 1000, 3.0, offset.dY, 600), ScreenPos(dPointY, 1000, dStartZoom, 0, 600), 0.001);
}

TEST(AnchorOffsetStopsAtTheEdgesWhenZoomingOut) {
	// Zooming out around a face near the corner would pull the image off the window
	// edge, so the edge wins: (1000 - 800) / 2 and (1000 - 600) / 2.
	FaceMath::SOffset start = FaceMath::CenterOffset(100, 100, Size(1000, 1000), 2.0, Size(800, 600));
	FaceMath::SOffset offset = FaceMath::AnchorOffset(100, 100, Size(1000, 1000), 2.0, start, 1.0, Size(800, 600));
	CHECK_NEAR(offset.dX, 100, 0.001);
	CHECK_NEAR(offset.dY, 200, 0.001);
}
