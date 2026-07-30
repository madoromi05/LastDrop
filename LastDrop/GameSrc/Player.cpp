#include "Framework.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Math/Functions.h"
using namespace GameLib;

#include "Player.h"
#include "Arena.h"
#include "Target.h"
#include "Bullet.h"
#include "Pad.h"
#include "Library/GraphicsDatabase.h"
#include "Library/Model.h"
#include <cmath>

// ── 移動・接地 ──
const float Player::mMaxMoveSpeed    = 0.6f;
const int   Player::mMoveAccelFrames = 10;
const float Player::mTurnSpeedY      = 12.0f;
const float Player::mBodyRadius      = 0.8f;
const float Player::mChestHeight     = 1.2f;
const float Player::mGravity         = 0.08f;
// ── 回避 ──
const float Player::mDodgeSpeed          = 1.6f;
const int   Player::mDodgeFrames         = 10;
const int   Player::mDodgeIFrames        = 8;
const int   Player::mDodgeCooldownFrames = 25;
// ── 拘束ワイヤー ──
const float Player::mWirePullSpeed     = 2.5f;
const float Player::mWireStopRadius     = 100.0f;
const float Player::mWireMaxLength      = 200.0f;
const int   Player::mWireCooldownFrames = 15;
const int   Player::mWireMaxFrames      = 120;
const int   Player::mWireLingerFrames   = 18;
// ── 体力・射撃 ──
const int Player::mMaxHitPoint        = 100;
const int Player::mBulletNumber       = 16;
const int Player::mFireCooldownFrames = 6;
const int Player::mBulletDamage       = 1;
// ── カメラ ──
const float Player::mCameraDistanceZ       = 10.0f;
const float Player::mCameraDistanceY       = 4.0f;
const float Player::mCameraTargetDistanceZ = 20.0f;

Player::Player( const Vector3& position ) :
	mPosition( position ),
	mVelocity( 0.0f, 0.0f, 0.0f ),
	mAngleY( 0.0f ),
	mHitPoint( mMaxHitPoint ),
	mInvincibleCount( 0 ),
	mDodgeDir( 0.0f, 0.0f, 0.0f ),
	mDodgeCount( 0 ),
	mDodgeCooldown( 0 ),
	mWirePulling( false ),
	mAnchor( 0.0f, 0.0f, 0.0f ),
	mWireCount( 0 ),
	mWireCooldown( 0 ),
	mWireVisibleCount( 0 ),
	mWireDrawFrom( 0.0f, 0.0f, 0.0f ),
	mFireCooldown( 0 ) {
	mDatabase = NEW GraphicsDatabase( "Resource/robo.txt" );
	mModel = mDatabase->createModel( "robo" );
	mBullets.setSize( mBulletNumber );
}

Player::~Player() {
}

const Vector3* Player::getPosition() const { return &mPosition; }
float Player::getAngleY() const { return mAngleY; }
int  Player::getHitPoint() const { return mHitPoint; }
bool Player::isInvincible() const { return mInvincibleCount > 0; }
bool Player::isWirePulling() const { return mWirePulling; }
int  Player::getBulletNumber() const { return mBulletNumber; }

const Vector3* Player::getBulletPosition( int index ) const {
	if (mBullets[ index ].isEmpty()) {
		return 0;
	}
	return mBullets[ index ].getPosition();
}

void Player::update( const Arena& arena, Target* target ) {
	if (mHitPoint <= 0) { // 敗北後は動かない。
		return;
	}
	// 移動はワールド固定軸（＝固定カメラ相対）で扱う。向きに追従させると対象を
	// 中心に周回してしまうため、移動と向きを切り離す。
	// カメラは -Z 側からプレイヤーを見るため、画面右はワールド -X になる。
	// よって右方向は (-1,0,0) にして画面の左右と一致させる。W=奥(+Z)、D=画面右。
	const Vector3 forward( 0.0f, 0.0f, 1.0f );
	const Vector3 right( -1.0f, 0.0f, 0.0f );

	// 状態の優先順位：ワイヤー引き寄せ ＞ 回避 ＞ 新規アクション開始／通常移動。
	if (mWirePulling) {
		updateWirePull();
	}
	else if (mDodgeCount > 0) {
		mVelocity.x = mDodgeDir.x * mDodgeSpeed; // 回避中は水平方向へ一定速で滑る。
		mVelocity.z = mDodgeDir.z * mDodgeSpeed;
		--mDodgeCount;
		if (mDodgeCount == 0) {
			mDodgeCooldown = mDodgeCooldownFrames;
			mVelocity.x *= 0.2f; // 回避終了時に少しだけ余韻を残して止める。
			mVelocity.z *= 0.2f;
		}
	}
	else {
		// 新しいアクションを起こせる状態。ワイヤー→回避の順で判定する。
		if (InputWire() && mWireCooldown == 0) {
			startWire( arena, *target );
		}
		else if (InputDodge() && mDodgeCooldown == 0) {
			startDodge( forward, right );
		}
		else {
			updateMove( forward, right );
		}
	}

	// 垂直方向：引き寄せ中は updateWirePull が全成分を制御するので触らない。
	// それ以外は重力で床へ引き戻す（ワイヤーで浮いた後の着地用）。
	if (!mWirePulling) {
		mVelocity.y -= mGravity;
	}

	// 位置更新と地形での押し戻し。
	mPosition += mVelocity;
	bool onGround = arena.clampInside( &mPosition, mBodyRadius );
	if (onGround && mVelocity.y < 0.0f) {
		mVelocity.y = 0.0f; // 着地したら落下速度を消す。
	}
	// 対象（ダミー）を通り抜けないように、重なったぶんだけ水平に押し出す。
	{
		const Vector3& tp = *target->getPosition();
		float dx = mPosition.x - tp.x;
		float dz = mPosition.z - tp.z;
		float distSq = dx * dx + dz * dz;
		float minDist = mBodyRadius + target->getRadius();
		if (distSq < minDist * minDist && distSq > 0.0001f) {
			float dist = GameLib::Math::sqrt( distSq );
			float push = ( minDist - dist ) / dist;
			mPosition.x += dx * push;
			mPosition.z += dz * push;
		}
	}

	// 向きの更新（移動方向を向く。静止時は対象を向いて構える）。
	updateFacing( *target );

	// 射撃と弾の処理（対象を弱化させる）。
	updateAttack( target );

	// タイマーを進める。
	if (mInvincibleCount > 0) { --mInvincibleCount; }
	if (mWireCooldown > 0)    { --mWireCooldown; }
	if (mDodgeCooldown > 0)   { --mDodgeCooldown; }
	if (!mWirePulling && mWireVisibleCount > 0) { --mWireVisibleCount; } // 残像を消していく。

	// 見た目へ反映。
	mModel->setAngle( Vector3( 0.0f, mAngleY, 0.0f ) );
	mModel->setPosition( mPosition );
}

void Player::updateFacing( const Target& target ) {
	// 動いていれば進行方向を、止まっていれば対象を向く。
	// ＝ワイヤー／射撃は「向いている方向」へ出るので、移動で壁を狙い、静止で敵を狙える。
	float desired;
	float hSpeedSq = mVelocity.x * mVelocity.x + mVelocity.z * mVelocity.z;
	if (hSpeedSq > 0.01f) {
		desired = GameLib::Math::atan2( mVelocity.x, mVelocity.z );
	}
	else {
		Vector3 to;
		to.setSub( *target.getPosition(), mPosition );
		if (to.x == 0.0f && to.z == 0.0f) {
			return; // 同位置なら向きを変えない。
		}
		desired = GameLib::Math::atan2( to.x, to.z );
	}
	float delta = desired - mAngleY;
	while (delta >  180.0f) { delta -= 360.0f; } // 近い方向へ回り込む。
	while (delta < -180.0f) { delta += 360.0f; }
	if (delta >  mTurnSpeedY) { delta =  mTurnSpeedY; } // 旋回速度で制限する。
	if (delta < -mTurnSpeedY) { delta = -mTurnSpeedY; }
	mAngleY += delta;
}

void Player::updateMove( const Vector3& forward, const Vector3& right ) {
	// 入力を前方・右方に展開して水平の移動方向を作る。
	Vector3 dir( 0.0f, 0.0f, 0.0f );
	if (InputUp())    { dir += forward; }
	if (InputDown())  { dir -= forward; }
	if (InputRight()) { dir += right; }
	if (InputLeft())  { dir -= right; }
	dir.y = 0.0f;

	float lenSq = dir.x * dir.x + dir.z * dir.z;
	if (lenSq == 0.0f) { // 入力なし。素直に止める（切り返しを軽くする）。
		mVelocity.x = 0.0f;
		mVelocity.z = 0.0f;
		return;
	}
	float inv = 1.0f / GameLib::Math::sqrt( lenSq );
	dir.x *= inv;
	dir.z *= inv;

	// 現在の水平速度を目標方向へ加速し、最大速度でクランプする。
	float accel = mMaxMoveSpeed / static_cast< float >( mMoveAccelFrames );
	Vector3 hv( mVelocity.x, 0.0f, mVelocity.z );
	hv.madd( dir, accel );
	float speed = GameLib::Math::sqrt( hv.x * hv.x + hv.z * hv.z );
	if (speed > mMaxMoveSpeed) {
		float k = mMaxMoveSpeed / speed;
		hv.x *= k;
		hv.z *= k;
	}
	mVelocity.x = hv.x;
	mVelocity.z = hv.z;
}

void Player::startDodge( const Vector3& forward, const Vector3& right ) {
	// 入力方向へ回避する。入力が無ければ正面方向へ。
	Vector3 dir( 0.0f, 0.0f, 0.0f );
	if (InputUp())    { dir += forward; }
	if (InputDown())  { dir -= forward; }
	if (InputRight()) { dir += right; }
	if (InputLeft())  { dir -= right; }
	dir.y = 0.0f;
	float lenSq = dir.x * dir.x + dir.z * dir.z;
	if (lenSq == 0.0f) {
		dir = forward;
		lenSq = dir.x * dir.x + dir.z * dir.z;
	}
	float inv = 1.0f / GameLib::Math::sqrt( lenSq );
	mDodgeDir.set( dir.x * inv, 0.0f, dir.z * inv );
	mDodgeCount = mDodgeFrames;
	mInvincibleCount = mDodgeIFrames; // 回避中は無敵。
}

void Player::startWire( const Arena& arena, const Target& target ) {
	// 射出原点は胸の高さ。狙いは正面（水平）方向。
	Vector3 origin = mPosition;
	origin.y += mChestHeight;
	Matrix34 rot;
	rot.setRotationY( mAngleY );
	Vector3 dir;
	rot.mul( &dir, Vector3( 0.0f, 0.0f, 1.0f ) ); // 前方（y成分は0）。

	// 候補1：対象（球）に当たるか。半直線と球の交差判定。
	bool  hasTarget = false;
	float targetDist = 0.0f;
	Vector3 targetAnchor( 0.0f, 0.0f, 0.0f );
	{
		Vector3 w;
		w.setSub( *target.getPosition(), origin );
		float proj = w.dot( dir ); // 最接近点までの距離。
		if (proj > 0.0f && proj <= mWireMaxLength) {
			float perpSq = w.squareLength() - proj * proj;
			float r = target.getRadius();
			if (perpSq <= r * r) {
				float thc = GameLib::Math::sqrt( r * r - perpSq );
				targetDist = proj - thc; // 手前側の交点。
				if (targetDist < 0.0f) { targetDist = 0.0f; }
				targetAnchor.setMadd( origin, dir, targetDist );
				hasTarget = true;
			}
		}
	}

	// 候補2：壁・床・天井に当たるか。
	bool  hasWall = false;
	float wallDist = 0.0f;
	Vector3 wallAnchor( 0.0f, 0.0f, 0.0f );
	if (arena.raycast( origin, dir, mWireMaxLength, &wallAnchor )) {
		Vector3 d;
		d.setSub( wallAnchor, origin );
		wallDist = d.length();
		hasWall = true;
	}

	// 手前のアンカーを採用する。当たらなければ何もしない（クールタイムも消費しない）。
	if (hasTarget && (!hasWall || targetDist <= wallDist)) {
		mAnchor = targetAnchor;
	}
	else if (hasWall) {
		mAnchor = wallAnchor;
	}
	else {
		return;
	}
	mWirePulling = true;
	mWireCount = 0;
}

void Player::updateWirePull() {
	Vector3 diff;
	diff.setSub( mAnchor, mPosition );
	float dist = diff.length();
	++mWireCount;
	// 到達、または安全弁で引き寄せを終了する。
	if (dist <= mWireStopRadius || mWireCount >= mWireMaxFrames) {
		mWirePulling = false;
		mWireCooldown = mWireCooldownFrames;
		mVelocity *= 0.25f; // 着地の余韻を残しつつ止める。
		// 引き寄せは一瞬なので、終了後もしばらくワイヤーを残して見せる（視認性）。
		mWireVisibleCount = mWireLingerFrames;
		mWireDrawFrom.set( mPosition.x, mPosition.y + mChestHeight, mPosition.z );
		return;
	}
	// アンカーへ直線で引き寄せる（振り子スイングは無し）。
	float inv = 1.0f / dist;
	mVelocity.set( diff.x * inv * mWirePullSpeed,
		diff.y * inv * mWirePullSpeed,
		diff.z * inv * mWirePullSpeed );
}

void Player::updateAttack( Target* target ) {
	if (mFireCooldown > 0) { --mFireCooldown; }
	// 引き寄せ中は撃たない（機動に専念）。
	if (InputFire() && mFireCooldown == 0 && !mWirePulling) {
		for (int i = 0; i < mBulletNumber; ++i) {
			if (mBullets[ i ].isEmpty()) {
				Vector3 muzzle = mPosition;
				muzzle.y += mChestHeight;
				mBullets[ i ].create( mDatabase, "bulletB", muzzle, 0.0f, mAngleY, true );
				mFireCooldown = mFireCooldownFrames;
				break;
			}
		}
	}
	// 弾の更新と対象への当たり判定。
	const Vector3& targetPos = *target->getPosition();
	float hitRange = target->getRadius();
	for (int i = 0; i < mBulletNumber; ++i) {
		if (mBullets[ i ].isEmpty()) { continue; }
		mBullets[ i ].update( targetPos );
		Vector3 d;
		d.setSub( *mBullets[ i ].getPosition(), targetPos );
		if (d.squareLength() < hitRange * hitRange) {
			target->setDamage( mBulletDamage );
			mBullets[ i ].die();
		}
	}
}

void Player::setDamage( int damage ) {
	if (mInvincibleCount > 0) { // 回避の無敵フレーム中は無効。
		return;
	}
	mHitPoint -= damage;
	if (mHitPoint < 0) {
		mHitPoint = 0;
	}
}

bool Player::getWireEndpoints( Vector3* from, Vector3* to ) const {
	if (mWirePulling) { // 引き寄せ中は手元＝現在の胸の位置。
		from->set( mPosition.x, mPosition.y + mChestHeight, mPosition.z );
		*to = mAnchor;
		return true;
	}
	if (mWireVisibleCount > 0) { // 終了後の残像は、終了時点の手元位置で固定して見せる。
		*from = mWireDrawFrom;
		*to = mAnchor;
		return true;
	}
	return false;
}

void Player::draw() const {
	mModel->draw();
	for (int i = 0; i < mBulletNumber; ++i) {
		if (!mBullets[ i ].isEmpty()) {
			mBullets[ i ].draw();
		}
	}
}

void Player::getView( Matrix34* viewMatrix, Vector3* eye ) const {
	// カメラは向きに追従させず、ワールド固定の角度（+Z奥向き）でプレイヤーを追う。
	// 追従回転させると移動が周回に見えてしまうため、ここは固定にする。
	// 注視点：プレイヤーの少し前方（+Z）。
	Vector3 t( mPosition.x, mPosition.y + mPosition.y * 0.12f, mPosition.z + mCameraTargetDistanceZ );
	// カメラ位置：後方(-Z)かつ少し上。
	Vector3 p( mPosition.x, mPosition.y + mCameraDistanceY + mPosition.y * 0.12f, mPosition.z - mCameraDistanceZ );
	viewMatrix->setViewTransform( p, t, Vector3( 0.0f, 1.0f, 0.0f ) );
	*eye = p;
}
