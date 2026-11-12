#include "GameSrc/Weapon/Bullet/BulletHoleList.h"
#include "GameSrc/Core/GameTime.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

#include <algorithm>
#include <cmath>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// tools/texgen/bullet_hole_gen.py で作る（TGA を作った後、tools/convert/texture_to_dds.ps1 で DDS にする）
	const char* kTextureFile = "Resource/Texture/Effect/bullet_hole.dds";
	const char* kNormalMapFile = "Resource/Texture/Effect/bullet_hole_normal.dds";

	const int kCapacity = 16;								// 同時に残す数。超えたら一番古い物から消す
	// 残す時間と、薄くし始めるまでの時間（ティック）。5 秒を過ぎたら少しずつ薄くし、10 秒で消す
	const int kLifeTicks = GameTime::secondsToTicks( 10.f );
	const int kFadeStartTicks = GameTime::secondsToTicks( 5.f );
	const int kFadeTicks = kLifeTicks - kFadeStartTicks;	// 薄くしていく長さ

	const float kHalfSize = 0.06f;	// 絵の幅の半分（m）。穴そのもの（7.62mm）では小さくて見えないので、周りの欠けを含めた大きさにする。見た目を確かめる前の初期値
	// デカールの箱の厚さの半分（m。面の前後にこれだけ届く）
	// 薄くして、角の奥の別の面まで届かないようにする。面から浮かせる量（板のときの 1cm）は要らない（面そのものに書くため）
	const float kHalfDepth = 0.05f;
	// 1 つごとに絵を回す角度（度。黄金角）。何枚並んでも向きがそろいにくく、同じ絵の繰り返しが目立たない
	const float kTurnPerHoleDegree = 137.5f;
	const float kFullTurnDegree = 360.f;

	// 絵の向きを作るときの仮の「上」。法線とほぼ平行（床・天井）なら外積が 0 に近づくので、もう一方を使う（Bullet::draw と同じ）
	const Vector3 kWorldUp( 0.f, 1.f, 0.f );
	const Vector3 kWorldRight( 1.f, 0.f, 0.f );
	const float kParallelLimit = 0.99f; // 法線と上の内積がこれを超えたら、ほぼ真上・真下とみなす
}

BulletHoleList::BulletHoleList() :
	mNextTurnDegree( 0.f ),
	mTexture( Graphics::Texture::create( kTextureFile, Graphics::TEXTURE_COLOR_SPACE_SRGB ) ),
	mNormalMap( Graphics::Texture::create( kNormalMapFile, Graphics::TEXTURE_COLOR_SPACE_LINEAR ) ), // 法線の向き（色ではない）
	mDecals( kCapacity ) {
	mHoles.reserve( kCapacity );
}

void BulletHoleList::add( const Vector3& point, const Vector3& normal, const IRayBlocker* attachedTo ) {
	ASSERT( attachedTo && "BulletHoleList : attachedTo is null." );
	if ( static_cast< int >( mHoles.size() ) >= kCapacity ) {
		mHoles.erase( mHoles.begin() ); // 一番古い物。数が少ないので、詰め直しの手間は気にしない
	}
	// 面の上で直交する2本の向きを作る
	const Vector3& helper = ( Math::abs( normal.dot( kWorldUp ) ) > kParallelLimit ) ? kWorldRight : kWorldUp;
	Vector3 tangent;
	tangent.setCross( helper, normal );
	tangent.normalize();
	Vector3 bitangent;
	bitangent.setCross( normal, tangent );

	// 法線まわりに回してから、絵の大きさにする
	Hole hole;
	hole.right.setMul( tangent, Math::cos( mNextTurnDegree ) * kHalfSize );
	hole.right.madd( bitangent, Math::sin( mNextTurnDegree ) * kHalfSize );
	hole.up.setCross( normal, hole.right ); // normal は長さ1で right と直交するので、長さは right と同じ
	hole.center = point;
	hole.normal = normal;
	hole.attachedTo = attachedTo;
	hole.age = 0;
	mHoles.push_back( hole );
	mNextTurnDegree = std::fmod( mNextTurnDegree + kTurnPerHoleDegree, kFullTurnDegree );
}

void BulletHoleList::update() {
	for ( Hole& hole : mHoles ) {
		++hole.age;
	}
	mHoles.erase( std::remove_if( mHoles.begin(), mHoles.end(),
		[]( const Hole& hole ) { return hole.age >= kLifeTicks; } ), mHoles.end() );
}

void BulletHoleList::removeAttachedTo( const IRayBlocker* attachedTo ) {
	mHoles.erase( std::remove_if( mHoles.begin(), mHoles.end(),
		[ attachedTo ]( const Hole& hole ) { return hole.attachedTo == attachedTo; } ), mHoles.end() );
}

void BulletHoleList::draw( const Matrix44& projectionView ) {
	// isReady を呼ぶと読み込みが進む（毎フレーム、2 枚とも呼ぶ）。読めなかった絵では描かない
	const bool textureReady = mTexture.isReady() && !mTexture.isError();
	const bool normalMapReady = mNormalMap.isReady() && !mNormalMap.isError();
	if ( !textureReady || !normalMapReady || mHoles.empty() || !decalsEnabled() ) {
		return;
	}
	for ( const Hole& hole : mHoles ) {
		addDecal( hole );
	}
	mDecals.draw( mTexture, mNormalMap, projectionView );
}

void BulletHoleList::addDecal( const Hole& hole ) {
	// kFadeStartTicks を過ぎたら、消えるまでに不透明度を 1 → 0 へ下げていく（絵のアルファに掛かる）
	float opacity = ( hole.age > kFadeStartTicks ) ? static_cast< float >( kLifeTicks - hole.age ) / kFadeTicks : 1.f;
	mDecals.add( hole.center, hole.right, hole.up, hole.normal, kHalfDepth, opacity );
}
