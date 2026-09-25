#include "StdAfx.h"
#include "AnnotationModel.h"

// RED STUB - compiles, returns wrong answers on purpose so the tests fail against
// real assertions rather than against a missing header. Replaced in the GREEN pass.

CAnnotationModel::CAnnotationModel() {
	m_bDirty = false;
}

bool CAnnotationModel::IsDegenerate(const CAnnotation& annotation) {
	return true;
}

void CAnnotationModel::Add(const CAnnotation& annotation) {
}

bool CAnnotationModel::Undo() {
	return false;
}

bool CAnnotationModel::Redo() {
	return false;
}

void CAnnotationModel::Clear() {
}
