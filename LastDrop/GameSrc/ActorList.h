#ifndef INCLUDED_ACTOR_LIST_H
#define INCLUDED_ACTOR_LIST_H

#include "IWorldMember.h"
#include "GameLib/GameLib.h"
#include <algorithm>
#include <memory>
#include <vector>

class CollisionWorld;
class HitWorld;

// 途中で増えたり消えたりするもの（ゾンビ・飛んだ部位・拾う物など）の持ち主。T は IWorldMember を実装すること。
// ・spawn：持って、当たり判定の名簿へ載せる
// ・destroyLater：消す印を付けるだけ（すぐには消さない）
// ・flush：印の付いたものを名簿から外して消す。ティックの最後に1回呼ぶ
// すぐに消さないのは、update の途中（ゾンビ同士の判定・名簿をたどっている最中など）で相手が消えて、
// 他の人が持っているポインタや名簿の中身が消えたものを指すのを防ぐため。消えるのは必ず flush の中だけ。
//
// 名簿（CollisionWorld・HitWorld）はこのリストより長く生きていること（シーンでは名簿を先に宣言する）。
// ComponentSystem を作るときは、このクラスをそれに置き換える（spawn / destroyLater / flush の約束は引き継ぐ）。
template< class T >
class ActorList{
public:
	typedef std::vector< std::unique_ptr< T > > Container;

	// collisionWorld / hitWorld: 載せる名簿。所有権は受け取らない。nullptr 不可。
	ActorList( CollisionWorld* collisionWorld, HitWorld* hitWorld ) :
	mCollisionWorld( collisionWorld ),
	mHitWorld( hitWorld ){
		ASSERT( mCollisionWorld && mHitWorld && "ActorList : world is null." );
	}
	// 残っているものも名簿から外してから消す（名簿の方が長生きなので、外さないと消えたものを指し続ける）。
	~ActorList(){
		for ( std::unique_ptr< T >& actor : mActors ){
			static_cast< IWorldMember* >( actor.get() )->removeFrom( mCollisionWorld, mHitWorld );
		}
	}
	ActorList( const ActorList& ) = delete;
	ActorList& operator=( const ActorList& ) = delete;

	// actor を持ち、名簿へ載せる。返すポインタは flush で消えるまで使える。actor: nullptr 不可。
	T* spawn( std::unique_ptr< T > actor ){
		ASSERT( actor && "ActorList : actor is null." );
		T* raw = actor.get();
		static_cast< IWorldMember* >( raw )->addTo( mCollisionWorld, mHitWorld );
		mActors.push_back( std::move( actor ) );
		return raw;
	}
	// 次の flush で消す印を付ける。同じものに何回付けてもよい。このリストのものでなければ何もしない。
	void destroyLater( const T* actor ){
		if ( std::find( mPendingDestroy.begin(), mPendingDestroy.end(), actor ) == mPendingDestroy.end() ){
			mPendingDestroy.push_back( actor );
		}
	}
	// 印の付いたものを名簿から外して消す。ティックの最後（全員の update が済んだ後）に1回呼ぶ。
	void flush(){
		for ( const T* doomed : mPendingDestroy ){
			auto it = std::find_if( mActors.begin(), mActors.end(),
				[ doomed ]( const std::unique_ptr< T >& a ){ return a.get() == doomed; } );
			if ( it == mActors.end() ){
				continue;
			}
			static_cast< IWorldMember* >( it->get() )->removeFrom( mCollisionWorld, mHitWorld );
			mActors.erase( it );
		}
		mPendingDestroy.clear();
	}
	// 基底クラスのポインタ（名簿から返ってきた IHittable* など）から、このリストのものを探す。無ければ nullptr。
	template< class Base >
	T* find( const Base* base ) const {
		for ( const std::unique_ptr< T >& actor : mActors ){
			if ( static_cast< const Base* >( actor.get() ) == base ){
				return actor.get();
			}
		}
		return nullptr;
	}

	// 全員をたどる（for ( auto& actor : list ) で使う）。たどっている間に spawn しないこと（並びが動く）。
	typename Container::iterator begin(){ return mActors.begin(); }
	typename Container::iterator end(){ return mActors.end(); }
	typename Container::const_iterator begin() const { return mActors.begin(); }
	typename Container::const_iterator end() const { return mActors.end(); }

private:
	CollisionWorld* mCollisionWorld;
	HitWorld* mHitWorld;
	Container mActors;
	std::vector< const T* > mPendingDestroy; // 次の flush で消すもの
};

#endif
