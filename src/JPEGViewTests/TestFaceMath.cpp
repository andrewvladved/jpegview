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

static FaceMath::SScoredFace Scored(double dX, double dY, double dWidth, double dHeight, double dScore) {
	FaceMath::SScoredFace face = { { dX, dY, dWidth, dHeight }, dScore };
	return face;
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

TEST(LetterboxFitsATallImageIntoTheSquare) {
	// 768 x 1365 into 640: the height fills it, the width is centred.
	FaceMath::SLetterbox letterbox = FaceMath::Letterbox(768, 1365, 640);
	CHECK_NEAR(letterbox.dScale, 640.0 / 1365, 0.000001);
	CHECK(letterbox.nWidth == 360);
	CHECK(letterbox.nHeight == 640);
	CHECK(letterbox.nPadX == 140);
	CHECK(letterbox.nPadY == 0);
}

TEST(LetterboxFitsAWideImageIntoTheSquare) {
	FaceMath::SLetterbox letterbox = FaceMath::Letterbox(2400, 1500, 640);
	CHECK(letterbox.nWidth == 640);
	CHECK(letterbox.nHeight == 400);
	CHECK(letterbox.nPadX == 0);
	CHECK(letterbox.nPadY == 120);
}

TEST(DecodeDetectionsMapsBoxesBackToTheImage) {
	// Three anchors, row after row: centre x, centre y, width, height, score.
	float output[5 * 3] = {
		240, 100, 400,
		120, 200, 300,
		 40,  20,  60,
		 60,  30,  80,
		0.9f, 0.1f, 0.5f,
	};
	// A 1000 x 2000 image: scale 0.32, 320 x 640 in the square, padded 160 on the left.
	FaceMath::SLetterbox letterbox = FaceMath::Letterbox(1000, 2000, 640);
	std::vector<FaceMath::SScoredFace> faces = FaceMath::DecodeDetections(output, 3, letterbox, 0.3);
	CHECK(faces.size() == 2);
	if (faces.size() == 2) {
		// (240 - 20 - 160) / 0.32, (120 - 30) / 0.32, 40 / 0.32, 60 / 0.32
		CHECK_NEAR(faces[0].face.dX, 187.5, 0.01);
		CHECK_NEAR(faces[0].face.dY, 281.25, 0.01);
		CHECK_NEAR(faces[0].face.dWidth, 125, 0.01);
		CHECK_NEAR(faces[0].face.dHeight, 187.5, 0.01);
		CHECK_NEAR(faces[0].dScore, 0.9, 0.0001);
		CHECK_NEAR(faces[1].dScore, 0.5, 0.0001);
	}
}

TEST(SuppressOverlapsKeepsTheBestOfOverlappingBoxes) {
	std::vector<FaceMath::SScoredFace> faces;
	faces.push_back(Scored(100, 100, 50, 50, 0.6));
	faces.push_back(Scored(104, 102, 50, 50, 0.9)); // nearly the same box, more certain
	faces.push_back(Scored(400, 100, 50, 50, 0.4)); // elsewhere
	std::vector<FaceMath::SScoredFace> kept = FaceMath::SuppressOverlaps(faces, 0.5);
	CHECK(kept.size() == 2);
	if (kept.size() == 2) {
		CHECK_NEAR(kept[0].dScore, 0.9, 0.0001);
		CHECK_NEAR(kept[1].dScore, 0.4, 0.0001);
	}
}

TEST(SelectFacesTakesEveryCertainFace) {
	std::vector<FaceMath::SScoredFace> faces;
	faces.push_back(Scored(0, 0, 10, 10, 0.8));
	faces.push_back(Scored(50, 0, 10, 10, 0.2));
	faces.push_back(Scored(90, 0, 10, 10, 0.5));
	std::vector<FaceMath::SFace> selected = FaceMath::SelectFaces(faces, 0.278, 0.12);
	CHECK(selected.size() == 2);
	if (selected.size() == 2) {
		CHECK_NEAR(selected[0].dX, 0, 0.001);
		CHECK_NEAR(selected[1].dX, 90, 0.001);
	}
}

TEST(SelectFacesFallsBackToTheBestLessCertainFace) {
	std::vector<FaceMath::SScoredFace> faces;
	faces.push_back(Scored(0, 0, 10, 10, 0.13));
	faces.push_back(Scored(50, 0, 10, 10, 0.2));
	faces.push_back(Scored(90, 0, 10, 10, 0.05));
	std::vector<FaceMath::SFace> selected = FaceMath::SelectFaces(faces, 0.278, 0.12);
	CHECK(selected.size() == 1);
	if (selected.size() == 1) {
		CHECK_NEAR(selected[0].dX, 50, 0.001);
	}
}

TEST(SelectFacesFindsNothingBelowTheFallback) {
	std::vector<FaceMath::SScoredFace> faces;
	faces.push_back(Scored(0, 0, 10, 10, 0.11));
	CHECK(FaceMath::SelectFaces(faces, 0.278, 0.12).empty());
}

TEST(MergeFacesAddsOnlyFacesNotFoundYet) {
	std::vector<FaceMath::SFace> first, second;
	first.push_back(Face(347, 181, 122, 118));
	second.push_back(Face(399, 217, 56, 56)); // the same face, a tighter box inside the first
	second.push_back(Face(10, 10, 40, 40)); // another face
	std::vector<FaceMath::SFace> merged = FaceMath::MergeFaces(first, second);
	CHECK(merged.size() == 2);
	if (merged.size() == 2) {
		CHECK_NEAR(merged[0].dX, 347, 0.001);
		CHECK_NEAR(merged[1].dX, 10, 0.001);
	}
}
