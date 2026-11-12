#ifndef INCLUDED_GAME_OBJECT_WORLD_H
#define INCLUDED_GAME_OBJECT_WORLD_H

#include "GameSrc/World/GameObject.h"
#include "GameSrc/World/SharedShelf.h"
#include "GameSrc/World/UpdatePhase.h"
#include "GameLib/GameLib.h"
#include <memory>
#include <unordered_map>
#include <vector>

class CollisionWorld;
class HitWorld;

// 世界に置く物（GameObject）の持ち主（SummaryPlan/個別仕様/設計と分割/ComponentSystem2_Plan.txt。画像の obj_manager）
// ・provide：世界に 1 つの物を共有の棚へ置く（部品が Component::shared で引く）
//   載せる名簿（CollisionWorld・HitWorld）は作るときに自分で置く
// ・create：物を作って持つ（まだ動かない）。部品を addComponent で付ける
// ・spawn：全部の部品の awake → 全部の start を呼び、名簿へ載る部品（IWorldMember）を当たり判定の名簿へ載せ、段の並びへ入れて動かし始める
// ・update( 段 )：その段の部品の update を、並びに載った順に呼ぶ（spawn した順。spawn の後に付けた部品は、付けた時に最後へ入る）（段の順は GameWorld::update が決める）
// ・destroyLater：消す印を付けるだけ。removeComponentLater（部品を外す印）も同じ
// ・flush：印の付いた部品・物の onDestroy を呼び、名簿から外して消す。ティックの最後に1回呼ぶ
//   update の途中（ゾンビ同士の判定・名簿をたどっている最中など）で相手が消えて、ほかの人の持つポインタや名簿の中身が
//   消えた物を指すのを防ぐため。消えるのは必ず flush の中だけ
// ・find：名簿から返ってきた基底のポインタ（IHittable* など）が、どの物の部品かを探す
// ・forEach< T >：T の部品を持つ物を全部、並びに載った順に（spawn した順。spawn の後に付けた部品は付けた時）。速くするため、部品の種類ごとに「その部品を持つ物の並び」も持つ
// 名簿（CollisionWorld・HitWorld）はこの持ち主より長く生きていること（シーンでは名簿を先に宣言する）
class GameObjectWorld{
public:
	GameObjectWorld( CollisionWorld* collisionWorld, HitWorld* hitWorld );	// collisionWorld / hitWorld: 載せる名簿。所有権は受け取らない。nullptr 不可
	// 残っている物も onDestroy を呼び、名簿から外してから消す（名簿の方が長生きなので、外さないと消えた物を指し続ける）
	~GameObjectWorld();
	GameObjectWorld( const GameObjectWorld& ) = delete;
	GameObjectWorld& operator=( const GameObjectWorld& ) = delete;

	// 世界に 1 つの item を共有の棚へ置く（同じ型は 1 つだけ。所有権は受け取らない。この持ち主より長く生きていること）
	// 引く部品を付けた物を spawn する前に置くこと
	template< class T >
	void provide( T* item ){
		mShelf.provide( item );
	}
	// 棚に置いた T（置いていなければ nullptr）
	template< class T >
	T* shared() const {
		return mShelf.find< T >();
	}

	// 物を作って持つ（画像の obj_manager.Register( "AK47" )）。返すポインタは flush で消えるまで使える
	// name: 確認用の名前（写して持つ。nullptr 不可）。部品を付け終えたら spawn する
	GameObject* create( GameObjectTag tag, const char* name );
	void spawn( GameObject* object );				// create した物を動かし始める。updatePhase の段の並びに、spawn した順で入る。たどっている最中には呼ばない（並びが動く）
	// phase の段の部品の update を、spawn した順に呼ぶ。たどっている間に destroyLater・removeComponentLater はしてよい（印だけ）
	// spawn・部品を付けることはしないこと
	void update( UpdatePhase phase );
	void destroyLater( const GameObject* object );	// 次の flush で消す印を付ける。同じ物に何回付けてもよい。この持ち主の物でなければ何もしない
	// 印の付いた部品を外し、物を消す。ティックの最後（全員の update が済んだ後）に1回呼ぶ
	// onDestroy の中で destroyLater・removeComponentLater してよい（同じ flush の中で、印が無くなるまで片付ける）
	// onDestroy の中で spawn・部品を付けることはしないこと（ASSERT で止める）
	void flush();

	// T の部品を持つ物を、並びに載った順（spawn した順。spawn の後に付けた部品は付けた時）に全部たどり、function( T& ) を呼ぶ
	// たどっている間に destroyLater はしてよい（印だけ）。spawn はしないこと
	template< class T, class Function >
	void forEach( Function function ){
		IterationGuard guard( this );
		const std::vector< GameObject* >* objects = objectsWith( componentTypeId< T >() );
		if ( !objects ){
			return;
		}
		for ( GameObject* object : *objects ){
			function( *object->getComponent< T >() );
		}
	}
	// 読むだけの forEach（function( const T& )）
	template< class T, class Function >
	void forEach( Function function ) const {
		const std::vector< GameObject* >* objects = objectsWith( componentTypeId< T >() );
		if ( !objects ){
			return;
		}
		for ( const GameObject* object : *objects ){
			function( static_cast< const T& >( *object->getComponent< T >() ) );
		}
	}
	// spawn した物を全部、作った順にたどり、function( const GameObject& ) を呼ぶ（F6 のエディタの物の一覧）
	template< class Function >
	void forEachObject( Function function ) const {
		for ( const std::unique_ptr< GameObject >& object : mObjects ){
			if ( object->isSpawned() ){
				function( static_cast< const GameObject& >( *object ) );
			}
		}
	}
	// T の部品を持つ物の数（消す印が付いていて、まだ flush していない物も数える）
	template< class T >
	int count() const {
		const std::vector< GameObject* >* objects = objectsWith( componentTypeId< T >() );
		return objects ? static_cast< int >( objects->size() ) : 0;
	}
	// 基底のポインタ（名簿から返ってきた IHittable* など）が、どの物の T の部品かを探す。無ければ nullptr
	template< class T, class Base >
	T* find( const Base* base ) const {
		const std::vector< GameObject* >* objects = objectsWith( componentTypeId< T >() );
		if ( !objects || !base ){
			return nullptr;
		}
		for ( GameObject* object : *objects ){
			T* component = object->getComponent< T >();
			if ( static_cast< const Base* >( component ) == base ){
				return component;
			}
		}
		return nullptr;
	}
	int countTagged( GameObjectTag tag ) const;				// tag の、spawn した物の数（消す印が付いていて、まだ flush していない物も数える）
	GameObject* oldestTagged( GameObjectTag tag ) const;	// tag の、spawn した物のうち一番先に作った物（無ければ nullptr）。数の上限を超えたときに古い物から消すのに使う
	GameObject* findById( unsigned id ) const;				// 番号の物（消えた・この持ち主の物でなければ nullptr）。GameObjectHandle が使う

private:
	friend class GameObject;							// 下の 2 つは GameObject（addComponent・removeComponentLater）だけが呼ぶ
	void attachLate( GameObject* object, int index );	// spawn した後に付けた部品（object の index 番目）を awake・start させ、載せる
	void requestRemoval( GameObject* object );			// object に外す印の付いた部品がある（flush で外す）

	// たどっている間か（たどっている最中の spawn・部品を付けることを見つける）
	class IterationGuard{
	public:
		explicit IterationGuard( GameObjectWorld* world ) : mWorld( world ){ ++mWorld->mIterationDepth; }
		~IterationGuard(){ --mWorld->mIterationDepth; }
		IterationGuard( const IterationGuard& ) = delete;
		IterationGuard& operator=( const IterationGuard& ) = delete;
	private:
		GameObjectWorld* mWorld;
	};

	const std::vector< GameObject* >* objectsWith( int typeId ) const;	// typeId の部品を持つ物の並び（その種類の物を一度も持っていなければ nullptr）
	void attach( GameObject* object, int index );						// object の index 番目の部品を、名簿・種類ごとの並び・段の並びへ入れる（start は呼ぶ側が済ませる）
	void detach( GameObject* object, int index );						// object の index 番目の部品の onDestroy を呼び、名簿・種類ごとの並び・段の並びから外す（捨てるのは呼ぶ側）
	void detachAll( GameObject* object );								// object の全部の部品を、付けたのと逆の順に detach する
	void removeMarkedComponents();										// 外す印の付いた部品を外す（flush から）

	CollisionWorld* mCollisionWorld;
	HitWorld* mHitWorld;
	SharedShelf mShelf;                                      // 世界に 1 つの物（部品が shared で引く）
	std::vector< std::unique_ptr< GameObject > > mObjects;   // 作った順
	std::unordered_map< unsigned, GameObject* > mObjectsById; // GameObjectHandle が引く
	std::vector< std::vector< GameObject* > > mObjectsByType; // 部品の種類の番号で引く。中は並びに載った順（spawn した物だけ。spawn の後に付けた部品は付けた時に最後へ入る）
	std::vector< Component* > mPhaseComponents[ kUpdatePhaseNumber ]; // 段ごとの部品。中は spawn した順（None の段は使わない）
	std::vector< const GameObject* > mPendingDestroy;        // 次の flush で消す物
	std::vector< GameObject* > mPendingRemoval;              // 次の flush で外す部品を持つ物
	unsigned mNextId;                                        // 次に作る物の番号（使い回さない）
	int mIterationDepth;                                     // たどっている入れ子の深さ（0 ならたどっていない）
};

template< class T >
T* Component::shared() const {
	T* item = mGameObject->world()->shared< T >();
	ASSERT( item && "Component : a shared item is not provided." );
	return item;
}

#endif
