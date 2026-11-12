#ifndef INCLUDED_GAMELIB_VECTOR_H
#define INCLUDED_GAMELIB_VECTOR_H

namespace GameLib{

///低機能版std::vector
/*!
std::vectorを使わずこちらを使う利点はなにか？
実はあまりない。頻度の低い関数や、危険な関数を除いてあるので
使いやすく、学習には適しているだろう
あとはSTLでsize_tが返ってくることに閉口している人にはお勧めである。intでいいじゃん！

基本的にこのクラスは楽に書くためにあり、サイズ変更系の関数を
呼べば当然遅い。Arrayと違ってコピーもできるが、無思慮なコピーは破滅的な性能の劣化を
招くだろう
*/
template< class T > class Vector{
public:
	explicit Vector( int size = 0, int capacity = 0 );	///capacityを指定すると、その数までは増えても引っ越さない
	///丸ごとコピーする。遅いことは覚悟せよ
	Vector( const Vector& );							//コピーコンストラクタを禁止しない
	~Vector();
	//代入。丸ごとコピーする。遅いことは覚悟せよ
	void operator=( const Vector& );					//代入も禁止しない
	void push( const T& );								///追加
	T* push();											///場所だけ作る追加。ポインタを返すのでここに入れること
	void pop();											///最終要素を削除
	void clear();										///全削除
	int size() const;									///サイズ取得
	void setSize( int );								///サイズ変更。小さくしても引越しが起こらない
	T& operator[]( int );								///添え字(非const)
	const T& operator[]( int ) const;					///添え字(const)
	void moveTo( Vector* );								///中身を別のベクタに移す。自分は空になる。相手に入っていたものは消える
private:
	T* mElements;
	int mSize;
	int mCapacity;
};

} //namespace GameLib
#include "GameLib/Base/Impl/VectorImpl.h" //中身はこの中

#endif

