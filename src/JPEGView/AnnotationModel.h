#pragma once

#include "AnnotationTypes.h"

// Holds the annotations of the currently displayed image, with undo/redo.
// Append-only: an element cannot be moved or edited once added, only undone.
// Knows nothing about windows, painting or the main dialog, so it is unit-testable.
class CAnnotationModel {
public:
	CAnnotationModel();

	// Adds an annotation. Degenerate elements are silently ignored: a freehand stroke
	// with fewer than two points, a rectangle without two distinct corners, empty text.
	// Adding discards the redo stack.
	void Add(const CAnnotation& annotation);

	bool Undo(); // returns false if there was nothing to undo
	bool Redo(); // returns false if there was nothing to redo
	void Clear();

	bool IsEmpty() const { return m_annotations.empty(); }
	size_t Count() const { return m_annotations.size(); }
	const std::vector<CAnnotation>& Annotations() const { return m_annotations; }

	// True when the annotations differ from what was last saved into the image.
	bool IsDirty() const { return m_bDirty; }
	void MarkClean() { m_bDirty = false; }

private:
	static bool IsDegenerate(const CAnnotation& annotation);

	std::vector<CAnnotation> m_annotations;
	std::vector<CAnnotation> m_redoStack;
	bool m_bDirty;
};
