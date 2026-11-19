#include "GameSrc/Zombie/Damage/ZombieDismemberment.h"
#include "GameSrc/Zombie/Damage/ZombieSeverLayout.h"
#include "GameSrc/Math/PlacementMatrix.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	const float kCollapsedScale = 0.f; // 骨の原点へ頂点を潰す縮尺（FirstPersonBody の頭を消すのと同じ）
	// 切り口を丸めるときに残す縮尺。軸の向きは短く、横は首の太さくらいに残して、低いこぶにする
	const float kStumpLengthScale = 0.05f;
	const float kStumpWidthScale = 0.5f;
	// 切り口の手前の骨（首）を、自分の付け根から軸の向きへ縮める縮尺。首が長く突き出て見えないように短くする。太さは変えない
	const float kStumpBoneLengthScale = 0.3f;
	const float kKeepWidthScale = 1.f;
	const float kMinStumpAxisLength = 1e-4f; // 切り口の軸がこれより短いと向きが決まらないので、1点へ潰す（m）
}

ZombieDismemberment::ZombieDismemberment( const ZombieSeverLayout* layout ) :
	mLayout( layout ) {
	ASSERT( mLayout && "ZombieDismemberment : layout is null." );
	for ( bool& lost : mLost ) {
		lost = false;
	}
}

bool ZombieDismemberment::sever( BodyPart part ) {
	if ( !mLayout->canSever( part ) || isLost( part ) ) {
		return false;
	}
	for ( BodyPart lostPart : mLayout->cut( part ).parts ) {
		mLost[ static_cast< int >( lostPart ) ] = true;
	}
	mCutParts.push_back( part );
	mSeveredThisHit.push_back( part );
	return true;
}

void ZombieDismemberment::clearSeveredThisHit() {
	mSeveredThisHit.clear();
}

const std::vector< BodyPart >& ZombieDismemberment::severedThisHit() const {
	return mSeveredThisHit;
}

bool ZombieDismemberment::isLost( BodyPart part ) const {
	int index = static_cast< int >( part );
	ASSERT( index >= 0 && index < static_cast< int >( BodyPart::Count ) && "ZombieDismemberment : bad part." );
	return mLost[ index ];
}

bool ZombieDismemberment::hasLostFooting() const {
	for ( BodyPart part : mCutParts ) {
		if ( mLayout->cut( part ).losesFooting ) {
			return true;
		}
	}
	return false;
}

void ZombieDismemberment::apply( std::vector< Matrix34 >* pose ) {
	for ( BodyPart part : mCutParts ) {
		const ZombieSeverLayout::Cut& cut = mLayout->cut( part );
		// 関節の位置と切り口の軸は、潰す前に読む（関節の骨も潰す骨に入っている）
		Vector3 joint = PlacementMatrix::translationOf( ( *pose )[ cut.jointBone ] );
		Vector3 axis;
		if ( stumpAxisOf( cut, *pose, &axis ) ) {
			// 首を付け根から短く縮め、頭はその縮めた首の先で丸める
			Vector3 stump = PlacementMatrix::translationOf( ( *pose )[ cut.stumpBone ] );
			Vector3 toJoint;
			toJoint.setSub( joint, stump );
			Vector3 shortJoint;
			shortJoint.setMul( toJoint, kStumpBoneLengthScale );
			shortJoint += stump;
			( *pose )[ cut.stumpBone ] = scaledAlong( stump, axis, kStumpBoneLengthScale, kKeepWidthScale, ( *pose )[ cut.stumpBone ] );
			for ( int bone : cut.bones ) {
				( *pose )[ bone ] = roundedAt( shortJoint, axis, ( *pose )[ bone ] );
			}
			continue;
		}
		Matrix34 collapsed = collapsedAt( joint );
		for ( int bone : cut.bones ) {
			( *pose )[ bone ] = collapsed;
		}
	}
}

bool ZombieDismemberment::stumpAxisOf( const ZombieSeverLayout::Cut& cut, const std::vector< Matrix34 >& pose, Vector3* axis ) {
	ASSERT( axis && "ZombieDismemberment : axis is null." );
	if ( cut.stumpBone < 0 ) {
		return false;
	}
	axis->setSub( PlacementMatrix::translationOf( pose[ cut.jointBone ] ), PlacementMatrix::translationOf( pose[ cut.stumpBone ] ) );
	if ( axis->length() < kMinStumpAxisLength ) {
		return false;
	}
	axis->normalize();
	return true;
}

Matrix34 ZombieDismemberment::roundedAt( const Vector3& point, const Vector3& axis, const Matrix34& bonePose ) {
	return scaledAlong( point, axis, kStumpLengthScale, kStumpWidthScale, bonePose );
}

Matrix34 ZombieDismemberment::scaledAlong( const Vector3& point, const Vector3& axis, float lengthScale, float widthScale, const Matrix34& bonePose ) {
	// 縮める行列 A = 横の縮尺 * I + ( 軸の縮尺 - 横の縮尺 ) * axis axisᵀ。point を中心に bonePose の後へ掛ける
	const float n[ 3 ] = { axis.x, axis.y, axis.z };
	const float extra = lengthScale - widthScale;
	float a[ 3 ][ 3 ];
	for ( int row = 0; row < 3; ++row ) {
		for ( int column = 0; column < 3; ++column ) {
			a[ row ][ column ] = extra * n[ row ] * n[ column ] + ( row == column ? widthScale : 0.f );
		}
	}
	const float b[ 3 ][ 4 ] = {
		{ bonePose.m00, bonePose.m01, bonePose.m02, bonePose.m03 - point.x },
		{ bonePose.m10, bonePose.m11, bonePose.m12, bonePose.m13 - point.y },
		{ bonePose.m20, bonePose.m21, bonePose.m22, bonePose.m23 - point.z },
	};
	float m[ 3 ][ 4 ];
	for ( int row = 0; row < 3; ++row ) {
		for ( int column = 0; column < 4; ++column ) {
			m[ row ][ column ] = a[ row ][ 0 ] * b[ 0 ][ column ] + a[ row ][ 1 ] * b[ 1 ][ column ] + a[ row ][ 2 ] * b[ 2 ][ column ];
		}
	}
	Matrix34 rounded;
	rounded.m00 = m[ 0 ][ 0 ];	rounded.m01 = m[ 0 ][ 1 ];	rounded.m02 = m[ 0 ][ 2 ];	rounded.m03 = m[ 0 ][ 3 ] + point.x;
	rounded.m10 = m[ 1 ][ 0 ];	rounded.m11 = m[ 1 ][ 1 ];	rounded.m12 = m[ 1 ][ 2 ];	rounded.m13 = m[ 1 ][ 3 ] + point.y;
	rounded.m20 = m[ 2 ][ 0 ];	rounded.m21 = m[ 2 ][ 1 ];	rounded.m22 = m[ 2 ][ 2 ];	rounded.m23 = m[ 2 ][ 3 ] + point.z;
	return rounded;
}

Matrix34 ZombieDismemberment::collapsedAt( const Vector3& point ) {
	Matrix34 collapsed;
	collapsed.setScaling( kCollapsedScale );
	collapsed.m03 = point.x;
	collapsed.m13 = point.y;
	collapsed.m23 = point.z;
	return collapsed;
}
