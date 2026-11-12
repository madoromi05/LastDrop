#ifndef INCLUDED_GAMELIB_TANK_H
#define INCLUDED_GAMELIB_TANK_H

#include "GameLib/Base/Array.h"

namespace GameLib{

///足し終わるまでアクセスせず、いくつ足すかがわからない、という用途のためのもの
/*!
水をためて一気に流すイメージでタンクとしたが、もっといい名前があれば募集
内部的には容量単位(blockSize)づつ内部でアロケートして継ぎ足していく

アクセスはgetCurrent()で取得、toNext()で次へ。rewind()で最初からアクセスしなおせる
*/
template< class T > class Tank{
public:
	explicit Tank( int blockSize = 16 );
	~Tank();
	void setBlockSize( int blockSize );	///再確保。中身がある状態で呼ぶとassert。clear()しろ
	int size() const;					///現在の要素数取得
	void clear();						///全解放
	T* add();							///末尾に空で追加。足したものへのポインタを返すので、初期設定はこれを使うこと
	void add( const T& );				///末尾に追加
	void toNext();						///次へ
	bool isEnd() const;					///終わりです
	T* get();							///現在の要素を取得(非const)
	const T* get() const;				///現在の要素を取得(const)
	void rewind();						///最初の要素にポインタを戻す
	void copyTo( T* ) const;			///単なる配列に変換。前もってsize()したサイズでnewした領域を渡すこと
	void copyTo( Array< T >* ) const;	///Arrayにコピー。空でないと怒られる
private:
	struct Block{
		Block* mNext; //次のノード
		T* mElements; //配列
	};
	void operator=( const Tank& ); //代入禁止
	Tank( const Tank& ); //コピーコンストラクタ禁止

	//定数
	int mBlockSize; //単位量
	int mSize;
	Block mHead; //ダミー

	Block* mLastBlock; //最終ノード
	int mLastPos; //最終ノード内の位置
	Block* mCurrentBlock; //現在のイテレータ
	int mCurrentPos; //ノード内位置
};

} //namespace GameLib
#include "GameLib/Base/Impl/TankImpl.h" //中身はこの中

#endif
