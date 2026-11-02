#include "GameSrc/Effect/BloodStainList.h"
#include "GameSrc/Core/GameTime.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

#include <algorithm>
#include <cmath>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// tools/blood_stain_gen.py で作る（TGA を作った後、tools/texture_to_dds.ps1 blood_stain で DDS にする）。
	const char* kTextureFile = "Resource/blood_stain.dds";
	const char* kNormalMapFile = "Resource/blood_stain_normal.dds";

	// 同時に残す数（BloodStain_Plan.txt Q4。ZombieHitFeedback_Plan.txt 4 章）。飛び散った血と滴の跡を合わせて数える。超えたら一番古い物から消す。
	const int kCapacity = 32;
	// 残す時間と、薄くし始めるまでの時間（ティック）。弾痕（10 秒）より長く残し、戦った跡を見せる。
	const int kLifeTicks = GameTime::secondsToTicks( 30.f );
	const int kFadeStartTicks = GameTime::secondsToTicks( 20.f );
	const int kFadeTicks = kLifeTicks - kFadeStartTicks; // 薄くしていく長さ

	// デカールの箱の厚さの半分（m）。弾痕と同じ（角の奥の別の面まで届かない薄さ）。
	const float kHalfDepth = 0.05f;

	// 絵の並び（tools/blood_stain_gen.py の main と合わせる）。2×2 で、左上・右上・左下が飛び散った血、右下が滴の跡。
	const int kCellsPerRow = 2;
	const float kCellUvSize = 1.f / kCellsPerRow;
	const int kSplatterCellNumber = 3;
	const int kDripCell = 3;

	// 向きをばらすとき、1 つごとに回す角度（度。黄金角。BulletHoleList と同じ）。
	const float kTurnPerStainDegree = 137.5f;
	const float kFullTurnDegree = 360.f;
	// 弾の向きを面に写した長さがこれより短ければ、面にほぼ垂直に撃ったとみなし、絵の向きをばらす（筋の向きが決まらないため）。
	const float kMinAlongSurface = 0.2f;

	// 絵の向きを作るときの仮の「上」（BulletHoleList と同じ）。
	const Vector3 kWorldUp( 0.f, 1.f, 0.f );
	const Vector3 kWorldRight( 1.f, 0.f, 0.f );
	const float kParallelLimit = 0.99f;

	DecalBatch::UvRect cellUvRect( int cell ) {
		DecalBatch::UvRect rect;
		rect.left = ( cell % kCellsPerRow ) * kCellUvSize;
		rect.top = ( cell / kCellsPerRow ) * kCellUvSize;
		rect.width = kCellUvSize;
		rect.height = kCellUvSize;
		return rect;
	}
}

BloodStainList::BloodStainList() :
	mNextTurnDegree( 0.f ),
	mNextSplatterCell( 0 ),
	mTexture( Graphics::Texture::create( kTextureFile, Graphics::TEXTURE_COLOR_SPACE_SRGB ) ),
	mNormalMap( Graphics::Texture::create( kNormalMapFile, Graphics::TEXTURE_COLOR_SPACE_LINEAR ) ), // 法線の向き（色ではない）
	mDecals( kCapacity ) {
	mStains.reserve( kCapacity );
}

void BloodStainList::addSplatter( const Vector3& point, const Vector3& normal, const Vector3& direction, float halfSize,
const IRayBlocker* attachedTo ) {
	// 弾の向きから面に垂直な分を引いて、面に沿う向きにする。
	Vector3 alongSurface = direction;
	alongSurface.madd( normal, -direction.dot( normal ) );
	Vector3 right;
	if ( alongSurface.length() >= kMinAlongSurface ) {
		alongSurface.normalize();
		right = alongSurface;
	} else {
		right = nextTurnedDirection( normal );
	}
	add( point, normal, right, halfSize, mNextSplatterCell, attachedTo );
	mNextSplatterCell = ( mNextSplatterCell + 1 ) % kSplatterCellNumber;
}

void BloodStainList::addDrip( const Vector3& point, const Vector3& normal, float halfSize, const IRayBlocker* attachedTo ) {
	add( point, normal, nextTurnedDirection( normal ), halfSize, kDripCell, attachedTo );
}

void BloodStainList::add( const Vector3& point, const Vector3& normal, const Vector3& rightDirection, float halfSize, int cell,
const IRayBlocker* attachedTo ) {
	ASSERT( attachedTo && "BloodStainList : attachedTo is null." );
	if ( static_cast< int >( mStains.size() ) >= kCapacity ) {
		mStains.erase( mStains.begin() ); // 一番古い物。数が少ないので、詰め直しの手間は気にしない
	}
	Stain stain;
	stain.right.setMul( rightDirection, halfSize );
	stain.up.setCross( normal, stain.right ); // normal は長さ1で right と直交するので、長さは right と同じ
	stain.center = point;
	stain.normal = normal;
	stain.attachedTo = attachedTo;
	stain.cell = cell;
	stain.age = 0;
	mStains.push_back( stain );
}

Vector3 BloodStainList::nextTurnedDirection( const Vector3& normal ) {
	// 面の上で直交する2本の向きを作り、法線まわりに回す（BulletHoleList::add と同じ）。
	const Vector3& helper = ( Math::abs( normal.dot( kWorldUp ) ) > kParallelLimit ) ? kWorldRight : kWorldUp;
	Vector3 tangent;
	tangent.setCross( helper, normal );
	tangent.normalize();
	Vector3 bitangent;
	bitangent.setCross( normal, tangent );
	Vector3 turned;
	turned.setMul( tangent, Math::cos( mNextTurnDegree ) );
	turned.madd( bitangent, Math::sin( mNextTurnDegree ) );
	mNextTurnDegree = std::fmod( mNextTurnDegree + kTurnPerStainDegree, kFullTurnDegree );
	return turned;
}

void BloodStainList::update() {
	for ( Stain& stain : mStains ) {
		++stain.age;
	}
	mStains.erase( std::remove_if( mStains.begin(), mStains.end(),
		[]( const Stain& stain ) { return stain.age >= kLifeTicks; } ), mStains.end() );
}

void BloodStainList::removeAttachedTo( const IRayBlocker* attachedTo ) {
	mStains.erase( std::remove_if( mStains.begin(), mStains.end(),
		[ attachedTo ]( const Stain& stain ) { return stain.attachedTo == attachedTo; } ), mStains.end() );
}

void BloodStainList::draw( const Matrix44& projectionView ) {
	// isReady を呼ぶと読み込みが進む（毎フレーム、2 枚とも呼ぶ）。読めなかった絵では描かない。
	const bool textureReady = mTexture.isReady() && !mTexture.isError();
	const bool normalMapReady = mNormalMap.isReady() && !mNormalMap.isError();
	if ( !textureReady || !normalMapReady || mStains.empty() || !decalsEnabled() ) {
		return;
	}
	for ( const Stain& stain : mStains ) {
		// kFadeStartTicks を過ぎたら、消えるまでに不透明度を 1 → 0 へ下げていく（BulletHoleList と同じ）。
		float opacity = ( stain.age > kFadeStartTicks ) ? static_cast< float >( kLifeTicks - stain.age ) / kFadeTicks : 1.f;
		mDecals.add( stain.center, stain.right, stain.up, stain.normal, kHalfDepth, opacity, cellUvRect( stain.cell ) );
	}
	mDecals.draw( mTexture, mNormalMap, projectionView );
}
