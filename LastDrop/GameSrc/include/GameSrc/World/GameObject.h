#ifndef INCLUDED_GAME_OBJECT_H
#define INCLUDED_GAME_OBJECT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/World/ComponentTypeId.h"
#include "GameSrc/World/GameObjectTag.h"
#include "GameSrc/World/IWorldMember.h"
#include "GameLib/GameLib.h"
#include <memory>
#include <type_traits>
#include <vector>

// 途中で増えたり消えたりする物 1 つ（Unity の GameObject と同じ考え方。SummaryPlan/個別仕様/ComponentSystem_Plan.txt）。
// 何の物かを表す種類（タグ）と、付けた部品（Component）を持つ。振る舞いは部品が持ち、物そのものは何もしない。
// ・部品は物を作るときにまとめて付け、途中で付け外ししない（GameObjectWorld へ渡す前に付け終える）
// ・同じ種類の部品は 1 つまで
// ・持ち主は GameObjectWorld だけ。ほかの人は生のポインタを借りる（消えるのは GameObjectWorld::flush の中だけ）
class GameObject{
public:
	explicit GameObject( GameObjectTag tag ) : mTag( tag ){}
	GameObject( const GameObject& ) = delete;
	GameObject& operator=( const GameObject& ) = delete;

	// 部品を付けて、借りるポインタを返す。component: nullptr 不可。同じ種類の部品はもう付いていないこと。
	// 当たり判定の名簿へ載る部品（IWorldMember）なら覚えておき、GameObjectWorld が spawn・flush で載せる・外す。
	template< class T >
	T* addComponent( std::unique_ptr< T > component ){
		static_assert( std::is_base_of< Component, T >::value, "GameObject : T must be a Component." );
		ASSERT( component && "GameObject : component is null." );
		ASSERT( !this->component< T >() && "GameObject : the same kind of component is already attached." );
		T* raw = component.get();
		static_cast< Component* >( raw )->mGameObject = this;
		Entry entry;
		entry.typeId = componentTypeId< T >();
		entry.member = worldMemberOf( raw );
		entry.component = std::move( component );
		mComponents.push_back( std::move( entry ) );
		return raw;
	}

	// T の部品（付いていなければ nullptr）。
	template< class T >
	T* component() const {
		int id = componentTypeId< T >();
		for ( const Entry& entry : mComponents ){
			if ( entry.typeId == id ){
				return static_cast< T* >( entry.component.get() );
			}
		}
		return nullptr;
	}

	GameObjectTag tag() const { return mTag; }

	// ── GameObjectWorld が使う ──
	int componentNumber() const { return static_cast< int >( mComponents.size() ); }
	int componentTypeIdAt( int index ) const { return mComponents[ index ].typeId; }
	// index 番目の部品が名簿へ載る部品（IWorldMember）なら、その窓口。載らない部品なら nullptr。
	IWorldMember* worldMemberAt( int index ) const { return mComponents[ index ].member; }

private:
	struct Entry{
		int typeId;
		std::unique_ptr< Component > component;
		IWorldMember* member; // 名簿へ載る部品なら、その窓口（component と同じ物を指す）。載らないなら nullptr
	};

	// T が IWorldMember も実装していれば、その窓口を返す（していなければ nullptr）。
	template< class T >
	static IWorldMember* worldMemberOf( T* component ){
		if constexpr ( std::is_base_of< IWorldMember, T >::value ){
			return static_cast< IWorldMember* >( component );
		} else {
			return nullptr;
		}
	}

	GameObjectTag mTag;
	std::vector< Entry > mComponents; // 付けた順
};

#endif
