#ifndef INCLUDED_GAMELIB_QUEUE_H
#define INCLUDED_GAMELIB_QUEUE_H

namespace GameLib{

///キュー。要素数が限界を超えなければ循環するのでいつまでも使える
template< class T > class Queue{
public:
	Queue();						///デフォルト。サイズ、最大容量は0
	///最大容量を指定
	explicit Queue( int capacity );
	~Queue();
	void setCapacity( int size );	///再確保。中身がある状態で呼ぶとassert。clear()しろ
	int capacity() const;			///最大容量取得
	int size() const;				///現在の要素数取得
	void clear();					///全解放
	T* pushBack();					///末尾に空で追加。足したものへのポインタを返すので、初期設定はこれを使うこと
	void pushBack( const T& );		///末尾に追加
	void popFront();				///先頭を削除
	void popFront( T* out );		///先頭を削除(捨てるもののコピーを取得)
	const T* front() const;			///先頭を取得(const)
	T* front();						///先頭を取得(非const)
private:
	void operator=( const Queue& ); //代入禁止
	Queue( const Queue& ); //コピーコンストラクタ禁止

	T* mElements;
	int mSize;
	int mCapacity;
	int mFront;
};

} //namespace GameLib
#include "GameLib/Base/Impl/QueueImpl.h" //中身はこの中

#endif

