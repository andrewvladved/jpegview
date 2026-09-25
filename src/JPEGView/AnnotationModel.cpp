#include "AnnotationModel.h"

CAnnotationModel::CAnnotationModel() {
	m_bDirty = false;
}

bool CAnnotationModel::IsDegenerate(const CAnnotation& annotation) {
	switch (annotation.eType) {
		case AT_Freehand:
			// A click without a drag leaves a single point: invisible, but it would
			// still mark the image dirty and trigger the save prompt.
			return annotation.points.size() < 2;
		case AT_Rectangle:
			return annotation.points.size() < 2;
		case AT_Text:
			return annotation.points.empty() || annotation.sText.IsEmpty();
	}
	return true;
}

void CAnnotationModel::Add(const CAnnotation& annotation) {
	if (IsDegenerate(annotation)) {
		return;
	}
	m_annotations.push_back(annotation);
	m_redoStack.clear();
	m_bDirty = true;
}

bool CAnnotationModel::Undo() {
	if (m_annotations.empty()) {
		return false;
	}
	m_redoStack.push_back(m_annotations.back());
	m_annotations.pop_back();
	// Undoing back to an empty model leaves nothing to save, so it is not dirty.
	m_bDirty = !m_annotations.empty();
	return true;
}

bool CAnnotationModel::Redo() {
	if (m_redoStack.empty()) {
		return false;
	}
	m_annotations.push_back(m_redoStack.back());
	m_redoStack.pop_back();
	m_bDirty = true;
	return true;
}

void CAnnotationModel::Clear() {
	bool bHadAnnotations = !m_annotations.empty();
	m_annotations.clear();
	m_redoStack.clear();
	if (bHadAnnotations) {
		m_bDirty = false;
	}
}
