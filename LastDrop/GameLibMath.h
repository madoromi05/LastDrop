#ifndef INCLUDED_GAMELIB_MATH_H
#define INCLUDED_GAMELIB_MATH_H

// 度数法で動作する数学ライブラリ
namespace GameLib{

double sin( double degree );
double cos( double degree );
double tan( double degree );
double asin( double );
double acos( double );
double atan( double );
double atan2( double y, double x );
double sqrt( double );

}

#include "GameLib/GameLib.h"

#endif
