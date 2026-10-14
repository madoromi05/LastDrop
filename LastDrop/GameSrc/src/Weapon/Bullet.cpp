#include "GameSrc/Weapon/Bullet.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Hit/HitWorld.h"
#include "GameSrc/Render/StaticMesh/StaticMeshData.h"

#include "GameLib/Math/Matrix34.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 弾の速さ（m/ティック）。7.62×51mm NATO（M80）の初速 約830m/秒。
	const float kSpeed = 830.f / GameTime::kTicksPerSecond;
	// 届く距離（m）。これだけ飛んだら消す。空へ撃った弾は何にも当たらないので、飛び続ける弾が溜まらないようにするための上限。
	const float kMaxRange = 300.f;

	// 弾の向きを作るときの仮の「横」を決める軸。飛ぶ向きとほぼ平行だと外積が 0 に近づくので、そのときはもう一方を使う。
	const Vector3 kWorldUp( 0.f, 1.f, 0.f );
	const Vector3 kWorldRight( 1.f, 0.f, 0.f );
	const float kParallelLimit = 0.99f; // 飛ぶ向きと上の内積がこれを超えたら、ほぼ真上・真下とみなす
}

Bullet::Bullet( const Vector3& origin, const Vector3& direction ) :
	mPosition( origin ),
	mDirection( direction ),
	mTraveled( 0.f ),
	mFinished( false ) {
}

bool Bullet::update( const HitWorld& world, const IHittable* shooter, HitWorld::Hit* hit ) {
	if ( mFinished ) {
		return false;
	}
	Vector3 step;
	step.setMul( mDirection, kSpeed );
	if ( world.raycast( mPosition, step, shooter, hit ) ) {
		mPosition = hit->point;
		mFinished = true;
		return true;
	}
	mPosition += step;
	mTraveled += kSpeed;
	if ( mTraveled >= kMaxRange ) {
		mFinished = true;
	}
	return false;
}

void Bullet::draw( const StaticMeshData& data ) const {
	// 弾頭のモデルは先が +Y なので、モデルの +Y を飛ぶ向きへ合わせる。弾は軸まわりに丸いので、残りの2軸は直交していればどこを向いてもよい。
	const Vector3& helper = ( Math::abs( mDirection.dot( kWorldUp ) ) > kParallelLimit ) ? kWorldRight : kWorldUp;
	Vector3 side;
	side.setCross( helper, mDirection );
	side.normalize();
	Vector3 front;
	front.setCross( side, mDirection );

	// 列が モデルの +X / +Y / +Z の行き先（ワールド）、最後の列が置き場所。
	Matrix34 world;
	world.m00 = side.x; world.m01 = mDirection.x; world.m02 = front.x; world.m03 = mPosition.x;
	world.m10 = side.y; world.m11 = mDirection.y; world.m12 = front.y; world.m13 = mPosition.y;
	world.m20 = side.z; world.m21 = mDirection.z; world.m22 = front.z; world.m23 = mPosition.z;
	data.draw( world );
}

bool Bullet::isFinished() const {
	return mFinished;
}
