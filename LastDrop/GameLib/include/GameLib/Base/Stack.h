#ifndef INCLUDED_GAMELIB_STACK_H
#define INCLUDED_GAMELIB_STACK_H

namespace GameLib{

///素直な固定サイズスタック
template< class T > class Stack{
public:
	Stack();						///デフォルト。サイズ、最大容量は0
	///最大容量を指定
	explicit Stack( int capacity );
	~Stack();
	void setCapacity( int size );	///再確保。中身がある状態で呼ぶとassert。clear()しろ
	int capacity() const;			///最大容量取得
	int size() const;				///現在の要素数取得
	void clear();					///全解放
	void push( const T& );			///末尾に追加
	void push();					///末尾に追加(デフォルトコンストラクタ呼び出し)
	void pop();						///末尾を削除
	void pop( T* out );				///末尾を削除(捨てる要素のコピーを受け取る)
	const T* get() const;			///末尾を取得(const)
	T* get();						///末尾を取得(非const)
private:
	void operator=( const Stack& ); //代入禁止
	Stack( const Stack& ); //コピーコンストラクタ禁止

	T* mElements;
	int mSize;
	int mCapacity;
	int mBack;
};

} //namespace GameLib
#include "GameLib/Base/Impl/StackImpl.h" //中身はこの中

#endif

