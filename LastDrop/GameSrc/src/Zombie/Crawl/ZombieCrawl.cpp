#include "GameSrc/Zombie/Crawl/ZombieCrawl.h"
#include "GameSrc/Math/AxisRotation.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Math/PlacementMatrix.h"
#include "GameSrc/Math/ScalarMath.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"
#include "GameLib/Math/Matrix34.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// ── 往復の時間割（SummaryPlan/個別仕様/ゾンビ/ZombieCrawl_Plan.txt の 0・4 章）──
	// 1 往復の長さ（秒）。前半で手を前へ運び、後半で引く。左右は半周ずらすので、引くのは 1 往復に 2 回。
	const float kStrokeSeconds = 1.25f;
	const float kPhasePerTick = 1.f / ( kStrokeSeconds * GameTime::kTicksPerSecond );
	const float kReachShare = 0.5f; // 1 往復のうち、手を前へ運んでいる割合（残りが引く）
	// 1 回引くと進む長さ（m）。1 往復で 2 回引くので、平均 0.25 × 2 / 1.25 = 0.4m/秒。
	const float kPullDistance = 0.25f;
	// 倒れた姿勢から決めた形へ移る長さ（秒）。
	const float kFadeInSeconds = 0.4f;
	const float kWeightPerTick = 1.f / ( kFadeInSeconds * GameTime::kTicksPerSecond );
	// 攻撃するとき・やめるときに、腕を伸ばす ⇔ 戻すのにかける長さ（秒）。
	const float kReachSeconds = 0.25f;
	const float kReachPerTick = 1.f / ( kReachSeconds * GameTime::kTicksPerSecond );

	// ── 胸 ──
	// 胸を反らせる角度（度。Spine1 で曲げる）。前腕で上体を支えた形にする（反らせすぎると手が地面に届かない）。
	const float kChestLiftDegree = 10.f;

	// ── 頭（2026-10-23 ユーザー指定「角度を変えて Player を見るように」）──
	// 顔を向けられる範囲（度）。体の前から左右へ、水平から上・下へ。これを越える向きは、範囲の端で止める。
	const float kMaxLookYawDegree = 70.f;
	const float kMaxLookUpDegree = 60.f;
	const float kMaxLookDownDegree = 10.f;
	// 相手が居ないときに見る向き（体の前から上へ起こす角度。度）。うつ伏せで地面を見ないように、少し上を見る。
	const float kIdleLookUpDegree = 20.f;
	// 顔の向きを回す速さ（度/ティック）。1 秒に 240°（相手が見えた瞬間に首が跳ねないように）。
	const float kLookTurnPerTick = 240.f / GameTime::kTicksPerSecond;

	// ── 手（2026-10-23 ユーザー指定「浮かせるのではなく地面に対して這う」）──
	// 手を突く所（肩の真下の地面から。m）。前へ伸ばし切った所と、外側へずらす量。引き終わりは、伸ばし切った所から
	// 1 回に進む長さだけ後ろ（地面に付けたまま、体が進む分だけ手が後ろへずれる）。
	const float kHandReachForward = 0.45f;
	const float kHandPulledForward = kHandReachForward - kPullDistance;
	const float kHandOutward = 0.12f;
	// 手首の中心・中指の付け根を置く高さ（m。地面から）。手の厚みの半分ほどで、地面に付いて見える。
	const float kWristGroundHeight = 0.045f;
	const float kKnuckleGroundHeight = 0.03f;
	// 手を前へ運ぶ途中に地面から浮かせる高さ（m。運ぶ真ん中で一番高い）。地面をこする程度。
	const float kHandCarryLift = 0.04f;
	// 攻撃するとき：手を伸ばす所（肩の真下の地面から前へ）と、手首の高さ（足首を引っかく）。
	const float kReachForward = 0.6f;
	const float kReachWristHeight = 0.12f;
	// 肘を向ける向き（外側・上・前 の重み）。前腕で這うときは、肘が外側の下へ張り出す。
	const Vector3 kElbowPole( 0.7f, -0.7f, -0.2f );
	// 肘の中心をこれより下へ行かせない高さ（m。地面から）。
	const float kElbowMinHeight = 0.05f;
	// 手首から中指の付け根の向きの、上下の成分の範囲（手のひらが地面へ刺さらない・反り返らない）。
	const float kHandMinPitch = -0.6f;
	const float kHandMaxPitch = 0.3f;

	// ── 脚（2026-10-23 ユーザー指定「足も地面に対して這うように」）──
	// 反対の側の腕を伸ばし切ったときに、太ももを外へ開く角度と、膝で足先を内へ曲げる角度（度）。どちらも真上の軸のまわり
	// （高さを変えない＝脚は地面に付いたまま、地面に沿って曲がる）。腕で引く間に 0 へ戻る（地面を蹴る）。
	const float kThighSpreadDegree = 25.f;
	const float kKneeBendDegree = 45.f;

	// これより短い向き・外積（長さ）は決まらないとみなす。
	const float kTinyLength = 0.001f;
	const float kHalfTurnDegree = 180.f;

	float distanceBetween( const Vector3& a, const Vector3& b ) {
		Vector3 d;
		d.setSub( b, a );
		return d.length();
	}

	// 往復の位置 phase で、腕をどこまで伸ばしているか（0 = 引き終わり、1 = 伸ばし切り）。前半で伸ばし、後半で引く。
	float strokeExtend( float phase ) {
		if ( phase < kReachShare ) {
			return ScalarMath::smoothStep( phase / kReachShare );
		}
		return 1.f - ScalarMath::smoothStep( ( phase - kReachShare ) / ( 1.f - kReachShare ) );
	}

	// 往復の位置 phase までに、その腕で引いた割合（引き始め 0 → 引き終わり 1。伸ばしている間は 0）。
	// 引く速さは引き始めと終わりで 0、真ん中で一番速い（ぐっと引いて止まる）ので、割合は (1 - cos) / 2 で増える。
	float pulledShare( float phase ) {
		if ( phase < kReachShare ) {
			return 0.f;
		}
		float t = ( phase - kReachShare ) / ( 1.f - kReachShare );
		return ( 1.f - Math::cos( kHalfTurnDegree * t ) ) * 0.5f;
	}

	// v を長さ 1 にする。短すぎれば fallback を返す。
	Vector3 normalizedOr( const Vector3& v, const Vector3& fallback ) {
		float length = v.length();
		if ( length < kTinyLength ) {
			return fallback;
		}
		Vector3 r = v;
		r *= 1.f / length;
		return r;
	}

	// pivot を中心に、axis（長さ1でなくてよい）のまわりへ degree 度、followers を回す。軸が決まらなければ回さない。
	void rotateBones( const Vector3& pivot, const Vector3& axis, float degree, const std::vector< int >& followers,
		std::vector< Matrix34 >* pose ) {
		float length = axis.length();
		if ( length < kTinyLength ) {
			return;
		}
		Vector3 unitAxis = axis;
		unitAxis *= 1.f / length;
		Matrix34 rotation = AxisRotation::about( pivot, unitAxis, degree );
		for ( int b : followers ) {
			Matrix34 bent;
			bent.setMul( rotation, ( *pose )[ b ] );
			( *pose )[ b ] = bent;
		}
	}

	// pivot を中心に、from の向きが to の向き（どちらも長さ1）へ weight の割合だけ近づくよう、followers を回す。
	void turnBones( const Vector3& pivot, const Vector3& from, const Vector3& to, float weight,
		const std::vector< int >& followers, std::vector< Matrix34 >* pose ) {
		// from × to の軸で正に回すと、from が to へ近づく（ZombieFlinch と同じ）。
		Vector3 axis;
		axis.setCross( from, to );
		float sine = axis.length();
		if ( sine < kTinyLength ) {
			return; // もう同じ向き（または真逆で軸が決まらない）
		}
		float degree = Math::atan2( sine, from.dot( to ) ) * weight;
		rotateBones( pivot, axis, degree, followers, pose );
	}

	// pivot の骨の原点を中心に、pivot → tip の向きが target の向きへ weight の割合だけ近づくよう、followers を回す。
	void aimBone( int pivot, int tip, const std::vector< int >& followers, const Vector3& target, float weight,
		std::vector< Matrix34 >* pose ) {
		Vector3 center = PlacementMatrix::translationOf( ( *pose )[ pivot ] );
		Vector3 direction;
		direction.setSub( PlacementMatrix::translationOf( ( *pose )[ tip ] ), center );
		if ( direction.length() < kTinyLength ) {
			return;
		}
		direction.normalize();
		turnBones( center, direction, target, weight, followers, pose );
	}

	// pivot → tip の向き（長さ1。決まらなければ長さ 0）。
	Vector3 boneDirection( int pivot, int tip, const std::vector< Matrix34 >& pose ) {
		Vector3 direction;
		direction.setSub( PlacementMatrix::translationOf( pose[ tip ] ), PlacementMatrix::translationOf( pose[ pivot ] ) );
		return normalizedOr( direction, Vector3( 0.f, 0.f, 0.f ) );
	}

	// 骨 b から骨 other への向きを水平にし、forward に直交させたもの（外側の向き）。決まらなければ長さ 0。
	Vector3 outwardFrom( int b, int other, const Vector3& forward, const std::vector< Matrix34 >& pose ) {
		Vector3 outward;
		outward.setSub( PlacementMatrix::translationOf( pose[ b ] ), PlacementMatrix::translationOf( pose[ other ] ) );
		outward.y = 0.f;
		outward.msub( forward, outward.dot( forward ) );
		return normalizedOr( outward, Vector3( 0.f, 0.f, 0.f ) );
	}

	// 外側・上・前 の重みで書いた向きを、骨格の根の空間の向きへ（長さはそのまま）。
	Vector3 bodyVector( const Vector3& weights, const Vector3& outward, const Vector3& up, const Vector3& forward ) {
		Vector3 v( 0.f, 0.f, 0.f );
		v.madd( outward, weights.x );
		v.madd( up, weights.y );
		v.madd( forward, weights.z );
		return v;
	}

	// 長さ upper・lower の 2 本の骨で、shoulder から target へ届く肘の位置（2 本の骨の IK）。
	// 肘は pole の側へ曲げる。届かなければ target の方へまっすぐ伸ばす（近すぎれば折りたたむ）。
	Vector3 solveElbow( const Vector3& shoulder, const Vector3& target, float upper, float lower, const Vector3& pole,
		const Vector3& fallbackDirection ) {
		Vector3 toTarget;
		toTarget.setSub( target, shoulder );
		Vector3 direction = normalizedOr( toTarget, fallbackDirection );
		float reach = ScalarMath::clamp( toTarget.length(), Math::abs( upper - lower ) + kTinyLength, upper + lower - kTinyLength );
		// 肩から、肘を target の方へ射影した所までの長さと、そこから肘までの高さ（余弦定理）。
		float along = ( upper * upper - lower * lower + reach * reach ) / ( 2.f * reach );
		float height = Math::sqrt( Math::max( 0.f, upper * upper - along * along ) );
		Vector3 bend = pole;
		bend.msub( direction, bend.dot( direction ) );
		bend = normalizedOr( bend, Vector3( 0.f, -1.f, 0.f ) );
		Vector3 elbow = shoulder;
		elbow.madd( direction, along );
		elbow.madd( bend, height );
		return elbow;
	}

	// current（長さ1）を desired（長さ1）へ、最大 maxDegree 度だけ回した向き。
	Vector3 rotateToward( const Vector3& current, const Vector3& desired, float maxDegree ) {
		Vector3 axis;
		axis.setCross( current, desired );
		float sine = axis.length();
		float degree = Math::atan2( sine, current.dot( desired ) );
		if ( degree <= maxDegree || sine < kTinyLength ) {
			return desired;
		}
		axis *= 1.f / sine;
		Matrix34 rotation = AxisRotation::about( Vector3( 0.f, 0.f, 0.f ), axis, maxDegree );
		Vector3 turned;
		rotation.mul33( &turned, current );
		return normalizedOr( turned, desired );
	}

	ZombieCrawlLayout::Side otherSide( ZombieCrawlLayout::Side side ) {
		return ( side == ZombieCrawlLayout::SIDE_LEFT ) ? ZombieCrawlLayout::SIDE_RIGHT : ZombieCrawlLayout::SIDE_LEFT;
	}
}

ZombieCrawl::ZombieCrawl( const ZombieCrawlLayout* layout ) :
	mLayout( layout ),
	mStarted( false ),
	mWeight( 0.f ),
	mPhase( 0.f ),
	mReach( 0.f ),
	mHasLookTarget( false ),
	mLookTarget( 0.f, 0.f, 0.f ),
	mHasLookDirection( false ),
	mLookDirection( 0.f, 0.f, 1.f ) {
	ASSERT( mLayout && "ZombieCrawl : layout is null." );
	for ( float& pulled : mPulled ) {
		pulled = 0.f;
	}
}

void ZombieCrawl::start() {
	mStarted = true;
}

bool ZombieCrawl::isStarted() const {
	return mStarted;
}

void ZombieCrawl::update( float pace, bool reaching ) {
	for ( float& pulled : mPulled ) {
		pulled = 0.f;
	}
	if ( !mStarted ) {
		return;
	}
	mWeight = Math::min( 1.f, mWeight + kWeightPerTick );
	mReach = reaching ? Math::min( 1.f, mReach + kReachPerTick ) : Math::max( 0.f, mReach - kReachPerTick );
	// 往復を進め、腕ごとに、この間に引いた長さを求める（引き終わりを越えて 1 周したときは、越える前と後を足す）。
	float step = kPhasePerTick * pace;
	for ( int s = 0; s < ZombieCrawlLayout::SIDE_NUMBER; ++s ) {
		float before = armPhase( static_cast< ZombieCrawlLayout::Side >( s ) );
		float after = before + step;
		float share = ( after >= 1.f ) ?
			( 1.f - pulledShare( before ) ) + pulledShare( after - 1.f ) :
			pulledShare( after ) - pulledShare( before );
		mPulled[ s ] = kPullDistance * share;
	}
	mPhase += step;
	if ( mPhase >= 1.f ) {
		mPhase -= 1.f;
	}
}

void ZombieCrawl::setLookTarget( bool hasTarget, const Vector3& rootPoint ) {
	mHasLookTarget = hasTarget;
	mLookTarget = rootPoint;
}

float ZombieCrawl::pullDistance( bool hasLeftArm, bool hasRightArm ) const {
	float distance = 0.f;
	if ( hasLeftArm ) {
		distance += mPulled[ ZombieCrawlLayout::SIDE_LEFT ];
	}
	if ( hasRightArm ) {
		distance += mPulled[ ZombieCrawlLayout::SIDE_RIGHT ];
	}
	return distance;
}

float ZombieCrawl::armPhase( ZombieCrawlLayout::Side side ) const {
	if ( side == ZombieCrawlLayout::SIDE_LEFT ) {
		return mPhase;
	}
	float phase = mPhase + 0.5f; // 右腕は半周ずらす（左が伸ばしている間に右が引く）
	return ( phase >= 1.f ) ? phase - 1.f : phase;
}

void ZombieCrawl::apply( std::vector< Matrix34 >* pose ) {
	if ( !mStarted || !mLayout->isValid() || mWeight <= 0.f ) {
		return;
	}
	// 体の向きは、倒れた姿勢の腰 → 首を水平にした向き（倒れ方で足元の向きから少しずれている）。胸を反らせる前に測る。
	Vector3 forward;
	forward.setSub( PlacementMatrix::translationOf( ( *pose )[ mLayout->neck() ] ), PlacementMatrix::translationOf( ( *pose )[ mLayout->hips() ] ) );
	forward.y = 0.f;
	if ( forward.length() < kTinyLength ) {
		return; // 立っている（腰の真上に首がある）。引きずる姿勢ではない
	}
	forward.normalize();
	const Vector3 up( 0.f, 1.f, 0.f );
	// 胸を先に反らせる（頭・腕は、反らせた後の首・肩の位置から形を決める）。
	liftChest( up, pose );
	turnHead( forward, up, pose );
	placeHand( ZombieCrawlLayout::SIDE_LEFT, forward, up, pose );
	placeHand( ZombieCrawlLayout::SIDE_RIGHT, forward, up, pose );
	sweepLeg( ZombieCrawlLayout::SIDE_LEFT, forward, up, pose );
	sweepLeg( ZombieCrawlLayout::SIDE_RIGHT, forward, up, pose );
}

void ZombieCrawl::liftChest( const Vector3& up, std::vector< Matrix34 >* pose ) const {
	// Spine1 → Spine2 の向きを上へ倒す軸で回す（背骨 × 上 の軸で正に回すと、背骨の先が上がる）。
	Vector3 spineDirection = boneDirection( mLayout->spine(), mLayout->chest(), *pose );
	Vector3 chestAxis;
	chestAxis.setCross( spineDirection, up );
	rotateBones( PlacementMatrix::translationOf( ( *pose )[ mLayout->spine() ] ), chestAxis, kChestLiftDegree * mWeight,
		mLayout->chestFollowers(), pose );
}

void ZombieCrawl::turnHead( const Vector3& forward, const Vector3& up, std::vector< Matrix34 >* pose ) {
	if ( mLayout->chin() < 0 ) {
		return;
	}
	// 顔の向き：頭からあごの先への向きから、首 → 頭の向きの成分を除いたもの（頭の骨の軸の取り方に頼らない）。
	Vector3 headPosition = PlacementMatrix::translationOf( ( *pose )[ mLayout->head() ] );
	Vector3 headAxis = boneDirection( mLayout->neck(), mLayout->head(), *pose );
	Vector3 face;
	face.setSub( PlacementMatrix::translationOf( ( *pose )[ mLayout->chin() ] ), headPosition );
	face.msub( headAxis, face.dot( headAxis ) );
	face = normalizedOr( face, forward );

	// 向けたい向き：相手が居れば頭から相手へ、居なければ前の少し上。どちらも、体の前から決めた範囲の内へ収める。
	Vector3 desired;
	if ( mHasLookTarget ) {
		desired.setSub( mLookTarget, headPosition );
	} else {
		desired.set( 0.f, 0.f, 0.f );
		desired.madd( forward, Math::cos( kIdleLookUpDegree ) );
		desired.madd( up, Math::sin( kIdleLookUpDegree ) );
	}
	Vector3 side;
	side.setCross( up, forward );
	float yaw = ScalarMath::clamp( Math::atan2( desired.dot( side ), desired.dot( forward ) ), -kMaxLookYawDegree, kMaxLookYawDegree );
	float flat = Math::sqrt( desired.x * desired.x + desired.z * desired.z );
	float pitch = ScalarMath::clamp( Math::atan2( desired.y, flat ), -kMaxLookDownDegree, kMaxLookUpDegree );
	Vector3 flatDirection( 0.f, 0.f, 0.f );
	flatDirection.madd( forward, Math::cos( yaw ) );
	flatDirection.madd( side, Math::sin( yaw ) );
	desired.set( 0.f, 0.f, 0.f );
	desired.madd( flatDirection, Math::cos( pitch ) );
	desired.madd( up, Math::sin( pitch ) );

	// 今向けている向きを少しずつ回し、顔をその向きへ合わせる（首で回す）。
	if ( !mHasLookDirection ) {
		mLookDirection = face;
		mHasLookDirection = true;
	}
	mLookDirection = rotateToward( mLookDirection, desired, kLookTurnPerTick );
	turnBones( PlacementMatrix::translationOf( ( *pose )[ mLayout->neck() ] ), face, mLookDirection, mWeight, mLayout->headFollowers(), pose );
}

void ZombieCrawl::placeHand( ZombieCrawlLayout::Side side, const Vector3& forward, const Vector3& up,
	std::vector< Matrix34 >* pose ) const {
	const ZombieCrawlLayout::Arm& arm = mLayout->arm( side );
	// 外側は、反対の肩からこの肩への向き（水平で、体の向きに直交させる）。骨格の左右の決まりに頼らない。
	Vector3 outward = outwardFrom( arm.shoulder, mLayout->arm( otherSide( side ) ).shoulder, forward, *pose );
	if ( outward.length() < kTinyLength ) {
		return;
	}
	Vector3 shoulder = PlacementMatrix::translationOf( ( *pose )[ arm.shoulder ] );
	Vector3 elbow = PlacementMatrix::translationOf( ( *pose )[ arm.elbow ] );
	Vector3 wrist = PlacementMatrix::translationOf( ( *pose )[ arm.hand ] );
	float upperLength = distanceBetween( shoulder, elbow );
	float lowerLength = distanceBetween( elbow, wrist );

	// 手首を置く所（肩の真下の地面から、前・外側へ）。
	//   前半：引き終わりの所から伸ばし切りの所へ、地面すれすれに運ぶ（真ん中で少し浮く）
	//   後半：地面に付けたまま、引いた割合だけ後ろへずらす（体が前へ進む分と同じ。地面の同じ所に手を突いて見える）
	float phase = armPhase( side );
	float forwardAmount;
	float height = kWristGroundHeight;
	if ( phase < kReachShare ) {
		float t = phase / kReachShare;
		forwardAmount = ScalarMath::lerp( kHandPulledForward, kHandReachForward, ScalarMath::smoothStep( t ) );
		height += kHandCarryLift * Math::sin( kHalfTurnDegree * t );
	} else {
		forwardAmount = ScalarMath::lerp( kHandReachForward, kHandPulledForward, pulledShare( phase ) );
	}
	// 攻撃するときは、前へ伸ばして足首の高さで止める。
	forwardAmount = ScalarMath::lerp( forwardAmount, kReachForward, mReach );
	height = ScalarMath::lerp( height, kReachWristHeight, mReach );
	Vector3 target( shoulder.x, 0.f, shoulder.z );
	target.madd( forward, forwardAmount );
	target.madd( outward, kHandOutward );
	target.y = height;

	// 肘は外側の下へ張り出す。地面より下へは行かせない（行くなら、肩からの長さを保って持ち上げる）。
	Vector3 elbowTarget = solveElbow( shoulder, target, upperLength, lowerLength, bodyVector( kElbowPole, outward, up, forward ),
		forward );
	if ( elbowTarget.y < kElbowMinHeight ) {
		elbowTarget.y = kElbowMinHeight;
		Vector3 fromShoulder;
		fromShoulder.setSub( elbowTarget, shoulder );
		elbowTarget = shoulder;
		elbowTarget.madd( normalizedOr( fromShoulder, forward ), upperLength );
	}
	Vector3 upperTarget;
	upperTarget.setSub( elbowTarget, shoulder );
	aimBone( arm.shoulder, arm.elbow, arm.upperFollowers, normalizedOr( upperTarget, forward ), mWeight, pose );
	// 回った後の肘から、手首を置く所へ前腕を向ける。
	Vector3 lowerTarget;
	lowerTarget.setSub( target, PlacementMatrix::translationOf( ( *pose )[ arm.elbow ] ) );
	aimBone( arm.elbow, arm.hand, arm.lowerFollowers, normalizedOr( lowerTarget, forward ), mWeight, pose );

	// 手：前腕の向きを水平にした向きで、中指の付け根を地面の高さへ（手のひらを地面に付ける）。
	Vector3 placedWrist = PlacementMatrix::translationOf( ( *pose )[ arm.hand ] );
	float handLength = distanceBetween( placedWrist, PlacementMatrix::translationOf( ( *pose )[ arm.knuckle ] ) );
	if ( handLength < kTinyLength ) {
		return;
	}
	Vector3 handFlat( lowerTarget.x, 0.f, lowerTarget.z );
	handFlat = normalizedOr( handFlat, forward );
	float pitch = ScalarMath::clamp( ( kKnuckleGroundHeight - placedWrist.y ) / handLength, kHandMinPitch, kHandMaxPitch );
	Vector3 handTarget = handFlat;
	handTarget *= Math::sqrt( 1.f - pitch * pitch );
	handTarget.y = pitch;
	aimBone( arm.hand, arm.knuckle, arm.handFollowers, handTarget, mWeight, pose );
}

void ZombieCrawl::sweepLeg( ZombieCrawlLayout::Side side, const Vector3& forward, const Vector3& up,
	std::vector< Matrix34 >* pose ) const {
	const ZombieCrawlLayout::Leg& leg = mLayout->leg( side );
	// 反対の側の腕が伸びていくのに合わせて開いて曲げ、その腕で引く間に伸ばす（左腕で引くとき右脚で地面を蹴る）。
	// 攻撃している間は伸ばして止める。失った脚は、この後の部位欠損で潰れるので気にしない。
	float bend = strokeExtend( armPhase( otherSide( side ) ) ) * ( 1.f - mReach ) * mWeight;
	if ( bend <= 0.f ) {
		return;
	}
	Vector3 outward = outwardFrom( leg.hip, mLayout->leg( otherSide( side ) ).hip, forward, *pose );
	if ( outward.length() < kTinyLength ) {
		return;
	}
	// どちらも真上の軸のまわりで回す（高さが変わらないので、地面に付いた脚は地面に沿って動く）。
	// 太もも：膝を外へ出す向きへ（水平にした太ももの向き × 外側 が上か下かで回す向きが決まる）。
	Vector3 thighFlat = boneDirection( leg.hip, leg.knee, *pose );
	thighFlat.y = 0.f;
	Vector3 thighAxis;
	thighAxis.setCross( thighFlat, outward );
	float thighSign = ( thighAxis.dot( up ) >= 0.f ) ? 1.f : -1.f;
	rotateBones( PlacementMatrix::translationOf( ( *pose )[ leg.hip ] ), up, thighSign * kThighSpreadDegree * bend, leg.thighFollowers, pose );
	// すね：足先を内へ（膝が外へ出た分、足先は体の真ん中の方へ曲がる。蛙のように地面を蹴る形）。
	Vector3 shinFlat = boneDirection( leg.knee, leg.ankle, *pose );
	shinFlat.y = 0.f;
	Vector3 inward = outward;
	inward *= -1.f;
	Vector3 shinAxis;
	shinAxis.setCross( shinFlat, inward );
	float shinSign = ( shinAxis.dot( up ) >= 0.f ) ? 1.f : -1.f;
	rotateBones( PlacementMatrix::translationOf( ( *pose )[ leg.knee ] ), up, shinSign * kKneeBendDegree * bend, leg.shinFollowers, pose );
}
