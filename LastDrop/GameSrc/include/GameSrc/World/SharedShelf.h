#ifndef INCLUDED_SHARED_SHELF_H
#define INCLUDED_SHARED_SHELF_H

#include "GameLib/GameLib.h"
#include <atomic>
#include <vector>

// 世界に 1 つの物（当たり判定の名簿・町の床の形・ゾンビ全員で共有するティックの周り）を置いておく棚
// （Game Programming Patterns の Service Locator）
// 部品がそれを付けるときの引数で受け取らなくてよくするため（付ける人が手元に持っていなくても付けられる）
// 持つのは GameObjectWorld、置くのはシーン（GameWorld）、引くのは部品（Component::shared）
// ・型ごとに 1 つだけ置ける。所有権は受け取らない（置いた人が、棚より長く生かす）
// ・物ごとに違う値（位置・どのモデルか・丈夫さ）は置かない（部品を付けるときの引数で渡す）
class SharedShelf{
public:
	// item を T の置き場へ置く。nullptr 不可。同じ型をもう置いていないこと
	template< class T >
	void provide( T* item ){
		ASSERT( item && "SharedShelf : item is null." );
		int id = typeId< T >();
		if ( id >= static_cast< int >( mItems.size() ) ){
			mItems.resize( id + 1, nullptr );
		}
		ASSERT( !mItems[ id ] && "SharedShelf : the same kind is already provided." );
		mItems[ id ] = item;
	}
	// 置いた T（置いていなければ nullptr）
	template< class T >
	T* find() const {
		int id = typeId< T >();
		return ( id < static_cast< int >( mItems.size() ) ) ? static_cast< T* >( mItems[ id ] ) : nullptr;
	}

private:
	// 型の番号（ComponentTypeId と同じ作り。番号は初めて使ったときに振るので、保存しないこと）
	// 数える所は atomic にする（別スレッドで同時に初めて使われても番号が重ならないように）
	static int nextTypeId(){
		static std::atomic< int > counter{ 0 };
		return counter.fetch_add( 1 );
	}
	template< class T >
	static int typeId(){
		static const int id = nextTypeId();
		return id;
	}

	std::vector< void* > mItems; // 型の番号で引く。借りているだけ
};

#endif
