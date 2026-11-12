#ifndef INCLUDED_GAMELIB_MATH_RANDOM_H
#define INCLUDED_GAMELIB_MATH_RANDOM_H

namespace GameLib{
namespace Math{

///乱数生成クラス
class Random{
public:
	static Random create( int seed = -1 );
	void setSeed( int );				///種再設定
	int getInt();						///-0x80000000 - 0x7fffffff
	int getInt( int a );				///a-1まで [0,a)
	int getInt( int a, int b );			///aからb-1まで [a,b)
	float getFloat();					///0-1だが、1は含まない ( 23bit )
	float getFloat( float a );			///aからbまで。ただしbは含まない。[a, b) ( 23bit )
	float getFloat( float a, float b );	///aからbまで。ただしbは含まない。[a, b) ( 23bit )

	//以下知らなくていい
	Random();
	Random( const Random& );
	~Random();
	operator void*() const;
	Random& operator=( const Random& );
	void release();
	bool operator==( const Random& ) const;
	bool operator!=( const Random& ) const;
	bool operator<( const Random& ) const;
private:
	class Impl;
	Impl* mImpl;
};

} //namespace Math
} //namespace GameLib

#endif