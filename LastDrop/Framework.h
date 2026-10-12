#ifndef INCLUDED_GAMELIB_FRAMEWORK_H
#define INCLUDED_GAMELIB_FRAMEWORK_H

namespace GameLib{

class Framework{
public:
	Framework();
	void update();
	static Framework instance();
	int width() const;
	int height() const;
	void requestEnd();
	bool isEndRequested() const;
	void sleep( int milliSeconds ) const;
	unsigned time() const;
	int frameRate() const;
	void setFrameRate( int );
	int previousFrameInterval() const;
	void drawDebugString( int column, int row, const char* string, unsigned color = 0xffffffff );

	void start( void* windowHandle );
	void preUpdate();
	void postUpdate();
	static void create();
	static void destroy();
};
}

#include "GameLib/GameLib.h"

#endif
