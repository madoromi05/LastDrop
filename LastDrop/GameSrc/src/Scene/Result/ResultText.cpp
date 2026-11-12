#include "GameSrc/Scene/Result/ResultText.h"

#include "GameLib/GameLib.h"
#include <cmath>
#include <cstdio>

namespace{
	const int kSecondsPerMinute = 60;
	const int kTextSize = 16;	// 「分:秒」と終わりの 0 が入る大きさ（int の分が何桁でも足りる）
	const char* kTimeFormat = "%d:%02d";
}

std::string ResultText::time( float seconds ) {
	ASSERT( seconds >= 0.f && "ResultText : seconds is negative." );
	const int whole = static_cast< int >( std::floor( seconds ) );
	char text[ kTextSize ];
	std::snprintf( text, sizeof( text ), kTimeFormat, whole / kSecondsPerMinute, whole % kSecondsPerMinute );
	return text;
}
