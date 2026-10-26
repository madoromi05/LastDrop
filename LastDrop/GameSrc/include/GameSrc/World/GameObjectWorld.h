#ifndef INCLUDED_GAME_OBJECT_WORLD_H
#define INCLUDED_GAME_OBJECT_WORLD_H

#include "GameSrc/World/GameObject.h"
#include "GameLib/GameLib.h"
#include <memory>
#include <vector>

class CollisionWorld;
class HitWorld;

// 途中で増えたり消えたりする物（GameObject）の持ち主（SummaryPlan/個別仕様/ComponentSystem_Plan.txt。前の ActorList の代わり）。
// ActorList の約束をそのまま引き継ぐ：
// ・spawn：持って、名簿へ載る部品（IWorldMember）を当たり判定の名簿へ載せる
// ・destroyLater：消す印を付けるだけ（すぐには消さない）
// ・flush：印の付いた物を名簿から外して消す。ティックの最後に1回呼ぶ
//   update の途中（ゾンビ同士の判定・名簿をたどっている最中など）で相手が消えて、ほかの人の持つポインタや名簿の中身が
//   消えた物を指すのを防ぐため。消えるのは必ず flush の中だけ。
// ・find：名簿から返ってきた基底のポインタ（IHittable* など）が、どの物の部品かを探す
// 新しく足した物：forEach<T>（T の部品を持つ物を全部、作った順に）・count<T>・タグで数える／探す。
// forEach を速くするため、部品の種類ごとに「その部品を持つ物の並び」も持つ（作った順のまま。flush で外す）。
// 名簿（CollisionWorld・HitWorld）はこの持ち主より長く生きていること（シーンでは名簿を先に宣言する）。
class GameObjectWorld{
public:
	// collisionWorld / hitWorld: 載せる名簿。所有権は受け取らない。nullptr 不可。
	GameObjectWorld( CollisionWorld* collisionWorld, HitWorld* hitWorld );
	// 残っている物も名簿から外してから消す（名簿の方が長生きなので、外さないと消えた物を指し続ける）。
	~GameObjectWorld();
	GameObjectWorld( const GameObjectWorld& ) = delete;
	GameObjectWorld& operator=( const GameObjectWorld& ) = delete;

	// object を持ち、名簿へ載せる。返すポインタは flush で消えるまで使える。object: nullptr 不可。部品は付け終えていること。
	// forEach でたどっている最中には呼ばない（並びが動く）。
	GameObject* spawn( std::unique_ptr< GameObject > object );
	// 部品 1 つだけの物を作って spawn し、その部品を返す（今の物はどれも部品 1 つ）。
	template< class T >
	T* spawn( GameObjectTag tag, std::unique_ptr< T > component ){
		std::unique_ptr< GameObject > object = std::make_unique< GameObject >( tag );
		T* raw = object->addComponent( std::move( component ) );
		spawn( std::move( object ) );
		return raw;
	}
	// 次の flush で消す印を付ける。同じ物に何回付けてもよい。この持ち主の物でなければ何もしない。
	void destroyLater( const GameObject* object );
	// 印の付いた物を名簿から外して消す。ティックの最後（全員の update が済んだ後）に1回呼ぶ。
	void flush();

	// T の部品を持つ物を、作った順に全部たどり、function( T& ) を呼ぶ。
	// たどっている間に destroyLater はしてよい（印だけ）。spawn はしないこと。
	template< class T, class Function >
	void forEach( Function function ){
		IterationGuard guard( this );
		const std::vector< GameObject* >* objects = objectsWith( componentTypeId< T >() );
		if ( !objects ){
			return;
		}
		for ( GameObject* object : *objects ){
			function( *object->component< T >() );
		}
	}
	// 読むだけの forEach（function( const T& )）。
	template< class T, class Function >
	void forEach( Function function ) const {
		const std::vector< GameObject* >* objects = objectsWith( componentTypeId< T >() );
		if ( !objects ){
			return;
		}
		for ( const GameObject* object : *objects ){
			function( static_cast< const T& >( *object->component< T >() ) );
		}
	}
	// T の部品を持つ物の数（消す印が付いていて、まだ flush していない物も数える）。
	template< class T >
	int count() const {
		const std::vector< GameObject* >* objects = objectsWith( componentTypeId< T >() );
		return objects ? static_cast< int >( objects->size() ) : 0;
	}
	// 基底のポインタ（名簿から返ってきた IHittable* など）が、どの物の T の部品かを探す。無ければ nullptr。
	template< class T, class Base >
	T* find( const Base* base ) const {
		const std::vector< GameObject* >* objects = objectsWith( componentTypeId< T >() );
		if ( !objects || !base ){
			return nullptr;
		}
		for ( GameObject* object : *objects ){
			T* component = object->component< T >();
			if ( static_cast< const Base* >( component ) == base ){
				return component;
			}
		}
		return nullptr;
	}
	// tag の物の数（消す印が付いていて、まだ flush していない物も数える）。
	int countTagged( GameObjectTag tag ) const;
	// tag の物のうち、一番先に作った物（無ければ nullptr）。数の上限を超えたときに古い物から消すのに使う。
	GameObject* oldestTagged( GameObjectTag tag ) const;

private:
	// forEach でたどっている間か（たどっている最中の spawn を見つける）。
	class IterationGuard{
	public:
		explicit IterationGuard( GameObjectWorld* world ) : mWorld( world ){ ++mWorld->mIterationDepth; }
		~IterationGuard(){ --mWorld->mIterationDepth; }
		IterationGuard( const IterationGuard& ) = delete;
		IterationGuard& operator=( const IterationGuard& ) = delete;
	private:
		GameObjectWorld* mWorld;
	};

	// typeId の部品を持つ物の並び（その種類の物を一度も持っていなければ nullptr）。
	const std::vector< GameObject* >* objectsWith( int typeId ) const;
	// object を名簿から外し、種類ごとの並びから外す（消すのは呼び出し側）。
	void detach( GameObject* object );

	CollisionWorld* mCollisionWorld;
	HitWorld* mHitWorld;
	std::vector< std::unique_ptr< GameObject > > mObjects;   // 作った順
	std::vector< std::vector< GameObject* > > mObjectsByType; // 部品の種類の番号で引く。中は作った順
	std::vector< const GameObject* > mPendingDestroy;        // 次の flush で消す物
	int mIterationDepth;                                     // forEach の入れ子の深さ（0 ならたどっていない）
};

#endif
