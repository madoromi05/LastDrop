#ifndef INCLUDED_GAME_OBJECT_H
#define INCLUDED_GAME_OBJECT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/World/ComponentTypeId.h"
#include "GameSrc/World/GameObjectTag.h"
#include "GameSrc/World/IWorldMember.h"
#include "GameLib/GameLib.h"
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

class GameObjectWorld;

// 世界に置く物 1 つ（Unity の GameObject と同じ考え方。SummaryPlan/個別仕様/ComponentSystem2_Plan.txt・ComponentSystem3_Plan.txt）。
// 何の物かを表す種類（タグ）と名前と、付けた部品（Component）を持つ。振る舞いは部品が持ち、物そのものは何もしない。
// ・作るのは GameObjectWorld::create。部品を addComponent で付け終えてから GameObjectWorld::spawn で動かし始める
// ・同じ種類の部品は 1 つまで。付ける順は自由（Component の awake・start の約束）
// ・途中で付けてもよい（すぐ awake・start して動き出す。GameObjectWorld がたどっている最中は付けない）。
//   外すのは removeComponentLater で印を付けるだけで、実際に外れるのは GameObjectWorld::flush の中（消える物と同じ約束）。
//   ほかの部品が requireComponent した部品は外せない（外すとその部品が成り立たないため）
// ・持ち主は GameObjectWorld だけ。ほかの人は生のポインタを借りる（消えるのは flush の中だけ）。
//   消えたかを知りたいときは GameObjectHandle で持つ
// ・名前は確認用（F6 のエディタの物の一覧・デバッグ出力）。ゲームの判断にはタグを使う（★6）
class GameObject{
public:
	// world: 持ち主。id: 持ち主が振る番号（GameObjectHandle が引く）。作るのは GameObjectWorld::create だけ。
	GameObject( GameObjectWorld* world, GameObjectTag tag, const char* name, unsigned id );
	GameObject( const GameObject& ) = delete;
	GameObject& operator=( const GameObject& ) = delete;

	// T の部品を args で作って付け、借りるポインタを返す（画像の AddComponent< ModelComponent >()）。同じ種類の部品はもう付いていないこと。
	// spawn した後に付けたなら、すぐ awake・start し、名簿・段の並びへ載せる。
	template< class T, class... Args >
	T* addComponent( Args&&... args ){
		static_assert( std::is_base_of< Component, T >::value, "GameObject : T must be a Component." );
		ASSERT( !getComponent< T >() && "GameObject : the same kind of component is already attached." );
		std::unique_ptr< T > component = std::make_unique< T >( std::forward< Args >( args )... );
		T* raw = component.get();
		static_cast< Component* >( raw )->mGameObject = this;
		Entry entry;
		entry.typeId = componentTypeId< T >();
		entry.member = worldMemberOf( raw );
		entry.component = std::move( component );
		entry.required = false;
		entry.removing = false;
		mComponents.push_back( std::move( entry ) );
		if ( mSpawned ){
			attachLate( static_cast< int >( mComponents.size() ) - 1 );
		}
		return raw;
	}

	// T の部品（付いていなければ nullptr。外す印が付いていても、外れるまでは返す）。
	template< class T >
	T* getComponent() const {
		int id = componentTypeId< T >();
		for ( const Entry& entry : mComponents ){
			if ( entry.typeId == id ){
				return static_cast< T* >( entry.component.get() );
			}
		}
		return nullptr;
	}

	// ほかの部品が要る T の部品（無ければ ASSERT で止める）。要る印を付け、removeComponentLater で外せなくする。
	// 印は、要った側の部品を外しても残す（どの部品が要ったかまでは覚えない）。
	template< class T >
	T* requireComponent(){
		int id = componentTypeId< T >();
		for ( Entry& entry : mComponents ){
			if ( entry.typeId == id ){
				entry.required = true;
				return static_cast< T* >( entry.component.get() );
			}
		}
		ASSERT( false && "GameObject : a required component is not attached." );
		return nullptr;
	}

	// 次の GameObjectWorld::flush で T の部品を外す印を付ける（付いていなければ何もしない）。外す前に onDestroy を呼ぶ。
	// ほかの部品が requireComponent した部品は外せない（ASSERT で止める）。外せるのは、隣の部品が getComponent で
	// 使う所ごとに引いている任意の部品だけ（覚えたポインタが残らないので、外れた後も落ちない）。
	template< class T >
	void removeComponentLater(){
		int id = componentTypeId< T >();
		for ( Entry& entry : mComponents ){
			if ( entry.typeId == id ){
				ASSERT( !entry.required && "GameObject : a component required by another component cannot be removed." );
				entry.removing = true;
				requestRemoval();
				return;
			}
		}
	}

	GameObjectTag tag() const { return mTag; }
	const std::string& name() const { return mName; }
	unsigned id() const { return mId; }
	bool isSpawned() const { return mSpawned; }
	GameObjectWorld* world() const { return mWorld; }

	// ── GameObjectWorld が使う ──
	int componentNumber() const { return static_cast< int >( mComponents.size() ); }
	Component* componentAt( int index ) const { return mComponents[ index ].component.get(); }
	int componentTypeIdAt( int index ) const { return mComponents[ index ].typeId; }
	// index 番目の部品が名簿へ載る部品（IWorldMember）なら、その窓口。載らない部品なら nullptr。
	IWorldMember* worldMemberAt( int index ) const { return mComponents[ index ].member; }
	bool isRemovingAt( int index ) const { return mComponents[ index ].removing; }
	// index 番目の部品を外して捨てる（onDestroy と名簿から外すのは GameObjectWorld が済ませてから呼ぶ）。
	void eraseAt( int index );
	void markSpawned(){ mSpawned = true; }

private:
	struct Entry{
		int typeId;
		std::unique_ptr< Component > component;
		IWorldMember* member; // 名簿へ載る部品なら、その窓口（component と同じ物を指す）。載らないなら nullptr
		bool required;        // ほかの部品が requireComponent した（外せない）
		bool removing;        // 次の flush で外す
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
	// spawn した後に付けた index 番目の部品を、持ち主に start させ・載せてもらう。
	void attachLate( int index );
	// 外す印を付けたことを持ち主に知らせる（flush で外してもらう）。
	void requestRemoval();

	GameObjectWorld* mWorld;
	GameObjectTag mTag;
	std::string mName;
	unsigned mId;
	bool mSpawned;
	std::vector< Entry > mComponents; // 付けた順
};

template< class T >
T* Component::getComponent() const {
	return mGameObject->getComponent< T >();
}

template< class T >
T* Component::requireComponent() const {
	return mGameObject->requireComponent< T >();
}

#endif
