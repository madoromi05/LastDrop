#ifndef INCLUDED_GAMELIB_THREADING_SEMAPHORE_H
#define INCLUDED_GAMELIB_THREADING_SEMAPHORE_H

namespace GameLib{
namespace Threading{

///決まった人数だけ同時にロックできるMutex
class Semaphore{
public:
	static Semaphore create( int startCount, int maxCount = 0 );	///引数の初期値で生成。maxCountは0だとstartCountになる
	void decrease();												///カウントを1減らそうとする。すでに0だと眠らされる
	void increase( int value = 1 );									///カウントを増やす。引数を省略すれば1づつ

	Semaphore();
	~Semaphore();
	Semaphore( const Semaphore& );
	operator void*() const;
	Semaphore& operator=( const Semaphore& );
	void release();
	bool operator==( const Semaphore& ) const;
	bool operator!=( const Semaphore& ) const;
private:
	class Impl;
	Impl* mImpl;
};

} //namespace Threading
} //namespace GameLib

#endif
