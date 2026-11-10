#ifndef INCLUDED_SELECTION_MARKS_H
#define INCLUDED_SELECTION_MARKS_H

#include "GameSrc/UI/UiRect.h"

class UiRenderer;

namespace SelectionMarks {
	void draw( UiRenderer& renderer, const UiRect& row );
	float labelLeft( const UiRect& row );
}

#endif