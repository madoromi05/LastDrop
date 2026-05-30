#pragma once
#include "Framework.h"

namespace GameLib{

class DebugScreen{
public:
	static DebugScreen instance(){ return DebugScreen(); }
	void draw( int col, int row, const char* s, unsigned color = 0xffffffff ){
		Framework::instance().drawDebugString( col, row, s, color );
	}
};

} //namespace GameLib
