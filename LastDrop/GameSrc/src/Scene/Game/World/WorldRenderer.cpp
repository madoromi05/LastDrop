#include "GameSrc/Scene/Game/World/WorldRenderer.h"
#include "GameSrc/Scene/Game/World/GameWorld.h"
#include "GameSrc/Scene/Game/World/EffectManager.h"
#include "GameSrc/Component/SkinnedModelComponent.h"
#include "GameSrc/Component/StaticModelComponent.h"
#include "GameSrc/Component/HitboxComponent.h"
#include "GameSrc/Player/Body/HoldComponent.h"
#include "GameSrc/Effect/MuzzleFlashComponent.h"
#include "GameSrc/Zombie/Component/SeveredLimbComponent.h"
#include "GameSrc/Render/Culling/DrawCulling.h"

#include "GameLib/GameLib.h"
#include "GameLib/Profiling/Profiler.h"

#include <algorithm>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 自キャラが手に持っている銃の、銃口が光る部品と銃口の位置・向き。素手（銃口が無い）なら nullptr
	MuzzleFlashComponent* heldMuzzleFlash( const GameObject* player, Vector3* muzzlePosition, Vector3* muzzleDirection ) {
		if ( !player ) {
			return nullptr;
		}
		const HoldComponent* hold = player->getComponent< HoldComponent >();
		GameObject* weapon = hold ? hold->heldObject() : nullptr; // 持つ部品が無ければ素手
		MuzzleFlashComponent* flash = weapon ? weapon->getComponent< MuzzleFlashComponent >() : nullptr;
		if ( !flash || !hold->muzzle( muzzlePosition, muzzleDirection ) ) {
			return nullptr;
		}
		return flash;
	}
}

WorldRenderer::WorldRenderer( const GameWorld* world, EffectManager* effects ) :
	mWorld( world ),
	mEffects( effects ),
	mBatches( false ) {
	ASSERT( world && effects && "WorldRenderer : an argument is null." );
}

void WorldRenderer::drawStatic( const DrawCulling& culling, const Vector3& eye ) {
	// 建物は目に近い順。手前の建物が奥の建物を隠すので、奥の建物の画素の多くが色の計算の前に捨てられる
	mBuildingDrawOrder.clear();
	for ( const Building& building : mWorld->town().buildings() ) {
		Vector3 toBuilding;
		toBuilding.setSub( building.position(), eye );
		mBuildingDrawOrder.emplace_back( toBuilding.squareLength(), &building );
	}
	std::sort( mBuildingDrawOrder.begin(), mBuildingDrawOrder.end(),
		[]( const std::pair< float, const Building* >& a, const std::pair< float, const Building* >& b ) { return a.first < b.first; } );
	// まとめて描くときも、形の中では近い順に詰めるので手前から描かれる（違う形どうしの順は混ざらない）
	beginBatch();
	PROFILE_COUNT( "Draw.instancing", mBatches ? 1.0 : 0.0 ); // CSV で F6 の Instancing の入/切のフレームを分けるため（目から見る絵で 1 回だけ数える）
	for ( const auto& entry : mBuildingDrawOrder ) {
		addBuilding( *entry.second, culling );
	}
	endBatch();
	Profiling::gpuMark( "Buildings" );
	// 置物（車・街灯・消火栓・ゴミ箱・落ちているガソリン。形ごとにまとめ、形の中では作った順＝車が先）
	drawProps( culling );
	Profiling::gpuMark( "Props" );
	// 地面は画面の下半分を覆うが、その多くは建物・キャラ・車に隠れるので最後。まとめて描くので映る範囲だけで間引く
	mWorld->town().ground().draw( culling.frustum() );
	Profiling::gpuMark( "Ground" );
}

void WorldRenderer::drawDecals( const Matrix44& projectionView ) {
	mEffects->drawDecals( projectionView );
}

void WorldRenderer::drawFirstPerson() const {
	// 一人称の腕と銃は画面の大きな所を占め、いつも一番手前にあるので、動く物の中では最初に描く
	drawPlayer();
	mWorld->bullets().draw();
	Profiling::gpuMark( "Player" );
}

void WorldRenderer::drawPlayer() const {
	const GameObject* player = mWorld->player();
	if ( !player ) {
		return;
	}
	player->getComponent< SkinnedModelComponent >()->draw();
	if ( const HoldComponent* hold = player->getComponent< HoldComponent >() ) {
		hold->draw(); // 素手なら何も描かない
	}
}

void WorldRenderer::drawCharacters( const DrawCulling& culling ) const {
	drawZombies( culling );
	drawSeveredLimbs( culling );
	Profiling::gpuMark( "Zombies" );
}

void WorldRenderer::drawSeveredLimbs( const DrawCulling& culling ) const {
	mWorld->objects().forEach< SeveredLimbComponent >( [ &culling ]( const SeveredLimbComponent& limb ) { limb.draw( culling ); } );
}

void WorldRenderer::drawShadowCasters( const DrawCulling& culling, bool includesPlayer ) {
	// 深度だけなので、drawStatic のような近い順の並べ替えはしない（速さにほとんど効かない）
	// 自キャラは部位の箱で間引く（月の遠い枚には映らないことが多い）
	const IHittable* playerTarget = mWorld->playerTarget();
	if ( includesPlayer && playerTarget && culling.showsPosed( playerTarget->hitboxes(), DrawCulling::Target::Large ) ) {
		drawPlayer();
	}
	drawZombies( culling );
	drawSeveredLimbs( culling );
	beginBatch();
	for ( const Building& building : mWorld->town().buildings() ) {
		addBuilding( building, culling );
	}
	endBatch();
	drawProps( culling );
}

void WorldRenderer::beginBatch() {
	mBatches = instancingEnabled() && mInstancing.isReady();
	mStaticBatch.clear();
}

int WorldRenderer::buildingIndex( const Building& building ) const {
	return static_cast< int >( &building - mWorld->town().buildings().data() );
}

void WorldRenderer::addBuilding( const Building& building, const DrawCulling& culling ) {
	if ( !culling.showsBuildingIndex( buildingIndex( building ) ) ) {
		return;
	}
	if ( !mBatches ) {
		building.draw( culling ); // 1 軒ずつ（F6 で Instancing を切った見比べ・シェーダが無いとき）
		return;
	}
	if ( building.shows( culling ) ) {
		mStaticBatch.add( building.mesh(), building.shapeFor( culling ) ); // 形 × 段ごとにまとまる
	}
}

void WorldRenderer::drawProps( const DrawCulling& culling ) {
	beginBatch();
	mWorld->objects().forEach< StaticModelComponent >( [ this, &culling ]( const StaticModelComponent& model ) { addProp( model, culling ); } );
	endBatch();
}

void WorldRenderer::addProp( const StaticModelComponent& model, const DrawCulling& culling ) {
	if ( !mBatches ) {
		model.draw( culling ); // 1 つずつ（F6 で Instancing を切った見比べ・シェーダが無いとき）
		return;
	}
	if ( model.shows( culling ) ) {
		mStaticBatch.add( model.mesh(), model.shapeFor( culling ) ); // 形 × 段ごとにまとまる（灯った街灯と消えた街灯は別の形）
	}
}

void WorldRenderer::endBatch() {
	if ( mBatches ) {
		mStaticBatch.draw( &mInstancing );
	}
}

void WorldRenderer::drawZombies( const DrawCulling& culling ) const {
	// 部位の箱（毎ティック今のポーズへ置き直している）を囲む箱で間引く
	mWorld->objects().forEach< ZombieBrainComponent >( [ &culling ]( const ZombieBrainComponent& zombie ) {
		const GameObject* object = zombie.gameObject();
		if ( culling.showsPosed( object->getComponent< HitboxComponent >()->hitboxes(), DrawCulling::Target::Zombie ) ) {
			object->getComponent< SkinnedModelComponent >()->draw();
		}
	} );
}

void WorldRenderer::drawImpactDust( const Matrix44& projectionView, const Vector3& viewDirection, const Vector3& eye ) {
	mEffects->drawImpactDust( projectionView, viewDirection, eye );
}

void WorldRenderer::drawBloodSpray( const Matrix44& projectionView, const Vector3& viewDirection, const Vector3& eye ) {
	mEffects->drawBloodSpray( projectionView, viewDirection, eye );
}

void WorldRenderer::drawMuzzleFlash( const Matrix44& projectionView, const Vector3& viewDirection ) const {
	Vector3 muzzlePosition;
	Vector3 muzzleDirection;
	// 撃った後に銃を手放したら、銃口が無いので描かない
	MuzzleFlashComponent* flash = heldMuzzleFlash( mWorld->player(), &muzzlePosition, &muzzleDirection );
	if ( !flash ) {
		return;
	}
	flash->flash().draw( projectionView, viewDirection, muzzlePosition, muzzleDirection,
		mWorld->player()->getComponent< HoldComponent >()->aimWeight() );
}
