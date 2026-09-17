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
// ── 立体機動ワイヤー（正面1アンカーのバネ-ダンパ振り子）──
const int   Player::kWireCount;         // 本数（宣言のみ。値はヘッダのクラス内初期化子）。
const float Player::mWireMaxLength      = 80.0f; // 届く最大距離（＝伸びの絶対上限）。
const float Player::mWireMinLength      = 6.0f;  // 引き寄せで縮められる自然長の下限。
const float Player::mWireReelSpeed      = 0.2f;  // 1フレームで自然長を縮める量（引き寄せ）。
const float Player::mWireSpring         = 0.01f; // 超過1mあたりの内向き復元加速（要実機調整）。
const float Player::mWireDamper         = 0.3f; // 外向き速度成分の毎フレーム減衰割合（要実機調整）。
const float Player::mWireObstacleMargin = 0.5f;  // 遮蔽判定でアンカー手前に残す余白。
const float Player::mWireAirControl     = 0.04f; // スイング中の左右空中制御の加速。
const int   Player::mWireCooldownFrames = 15;    // 離脱後の再射出までの待ち。
const int   Player::mWireLingerFrames   = 12;    // 離脱後にワイヤーを残して見せるフレーム数。
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
	mWireSwinging( false ),
	mWireCooldown( 0 ),
	mWireVisibleCount( 0 ),
	mLandOnContact( false ),
	mFireCooldown( 0 ) {
	mDatabase = NEW GraphicsDatabase( "Resource/robo.txt" );
	mModel = mDatabase->createModel( "robo" );
	mBullets.setSize( mBulletNumber );
	for (int i = 0; i < kWireCount; ++i) { // 配列メンバは本体で初期化する。
		mWireActive[ i ] = false;
		mAnchor[ i ].set( 0.0f, 0.0f, 0.0f );
		mWireLength[ i ] = 0.0f;
		mWireDrawFrom[ i ].set( 0.0f, 0.0f, 0.0f );
	}
}

Player::~Player() {
}

const Vector3* Player::getPosition() const { return &mPosition; }
float Player::getAngleY() const { return mAngleY; }
int  Player::getHitPoint() const { return mHitPoint; }
bool Player::isInvincible() const { return mInvincibleCount > 0; }
bool Player::isSwinging() const { return mWireSwinging; }
void Player::setLandOnContact( bool land ) { mLandOnContact = land; }
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

	// 保持式：スイング中に Wire を離したら離脱する（先に判定する）。
	if (mWireSwinging && InputWireRelease()) {
		releaseWire();
	}

	// 状態の優先順位：スイング ＞ 回避 ＞ 新規アクション開始／通常移動。
	if (mWireSwinging) {
		updateWireSwing( right, arena ); // 重力＋空中制御＋引き寄せ＋巻き付き＋バネ-ダンパ。
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
		// （離脱直後は mWireCooldown>0 なので同フレームの再射出は起きない。）
		if (InputWire() && mWireCooldown == 0) {
			startWireSwing( arena, *target );
		}
		else if (InputDodge() && mDodgeCooldown == 0) {
			startDodge( forward, right );
		}
		else {
			updateMove( forward, right );
		}
	}

	// 重力：スイング中は updateWireSwing 側で加える（拘束と適用順を揃えるため）。
	if (!mWireSwinging) {
		mVelocity.y -= mGravity;
	}

	// 位置更新。
	mPosition += mVelocity;

	// スイング中は絶対上限で暴走を止める（バネ拘束は速度側の updateWireSwing が担う）。
	if (mWireSwinging) {
		applyWireConstraint();
	}

	// 地形での押し戻し。
	bool onGround = arena.clampInside( &mPosition, mBodyRadius );
	if (onGround && mVelocity.y < 0.0f) {
		mVelocity.y = 0.0f; // 着地したら落下速度を消す。
	}
	// 接地したら、設定が「着地」のときだけワイヤーを切る（「継続」なら張ったまま滑る）。
	if (onGround && mWireSwinging && mLandOnContact) {
		releaseWire();
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
	if (!mWireSwinging && mWireVisibleCount > 0) { // 残像を消していく。
		--mWireVisibleCount;
		if (mWireVisibleCount == 0) { // 消え切ったら描画対象から外す。
			for (int i = 0; i < kWireCount; ++i) { mWireActive[ i ] = false; }
		}
	}

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

void Player::startWireSwing( const Arena& arena, const Target& target ) {
	// 射出原点は胸の高さ。向いている正面（レティクル方向）へ1本だけ飛ばす。
	Vector3 origin = mPosition;
	origin.y += mChestHeight;

	Matrix34 rot;
	rot.setRotationY( mAngleY );
	Vector3 dir;
	rot.mul( &dir, Vector3( 0.0f, 0.0f, 1.0f ) ); // 正面（y成分は0）。

	Vector3 anchor;
	float   length;
	if (!castWire( arena, target, origin, dir, &anchor, &length )) {
		mWireActive[ 0 ] = false;
		return; // 当たらなければ不発（クールタイムも消費しない）。
	}
	mAnchor[ 0 ] = anchor;
	mWireLength[ 0 ] = length; // 装着時の距離を自然長にする（＝最初はたるまず張った状態）。
	mWireActive[ 0 ] = true;
	mWireSwinging = true;
	mWireVisibleCount = 0;
}

bool Player::castWire( const Arena& arena, const Target& target,
	const Vector3& origin, const Vector3& dir,
	Vector3* anchor, float* length ) const {
	// 候補1：対象（球）に当たるか。半直線と球の交差判定。
	bool  hasTarget = false;
	float targetDist = 0.0f;
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

	// 手前のアンカーを採用する。当たらなければ false（不発）。
	if (hasTarget && (!hasWall || targetDist <= wallDist)) {
		anchor->setMadd( origin, dir, targetDist );
		*length = targetDist;
		return true;
	}
	if (hasWall) {
		*anchor = wallAnchor;
		*length = wallDist;
		return true;
	}
	return false;
}

void Player::updateWireSwing( const Vector3& right, const Arena& arena ) {
	if (!mWireActive[ 0 ]) { return; }

	// 重力で振り子を駆動する（拘束は速度側のバネ-ダンパで解く）。
	mVelocity.y -= mGravity;

	// A/D：スイング面の左右へ微加速（空中制御）。半径方向の余分はダンパで除かれる。
	if (InputRight()) { mVelocity.madd( right,  mWireAirControl ); }
	if (InputLeft())  { mVelocity.madd( right, -mWireAirControl ); }

	// 引き寄せ：ボタン保持中は自然長を下限まで縮める（＝バネで手前へ引き込まれる）。
	if (InputWireReel()) {
		mWireLength[ 0 ] -= mWireReelSpeed;
		if (mWireLength[ 0 ] < mWireMinLength) { mWireLength[ 0 ] = mWireMinLength; }
	}

	// 遮蔽物への巻き付き：手元→アンカー間に地形があればアンカーをその手前へ更新する。
	Vector3 hand( mPosition.x, mPosition.y + mChestHeight, mPosition.z );
	{
		Vector3 toAnchor;
		toAnchor.setSub( mAnchor[ 0 ], hand );
		float span = toAnchor.length();
		if (span > mWireObstacleMargin) {
			float inv = 1.0f / span;
			Vector3 dir( toAnchor.x * inv, toAnchor.y * inv, toAnchor.z * inv );
			Vector3 hit;
			if (arena.raycast( hand, dir, span - mWireObstacleMargin, &hit )) {
				mAnchor[ 0 ] = hit; // 角で折れる＝新しい支点。
				Vector3 rebent;
				rebent.setSub( mPosition, hit );
				float rebentLen = rebent.length();
				if (rebentLen < mWireLength[ 0 ]) { mWireLength[ 0 ] = rebentLen; }
			}
		}
	}

	// バネ-ダンパ拘束：自然長を超えて伸びたぶんだけ手前へ引き戻す（SpringJoint相当）。
	Vector3 rope;
	rope.setSub( mPosition, mAnchor[ 0 ] ); // アンカー→自分。
	float dist = rope.length();
	if (dist > mWireLength[ 0 ] && dist > 0.0001f) {
		float inv = 1.0f / dist;
		Vector3 n( rope.x * inv, rope.y * inv, rope.z * inv ); // 半径方向の単位ベクトル。
		float stretch = dist - mWireLength[ 0 ];
		mVelocity.madd( n, -mWireSpring * stretch );      // 超過に比例した内向きの復元力。
		float vr = mVelocity.dot( n );
		if (vr > 0.0f) { mVelocity.madd( n, -mWireDamper * vr ); } // 外向き速度を減衰（暴れ止め）。
	}
}

void Player::applyWireConstraint() {
	// 安全ガード：バネだけでは伸びきる暴走を防ぐため、絶対上限で球面へ固定する。
	if (!mWireActive[ 0 ]) { return; }
	Vector3 rope;
	rope.setSub( mPosition, mAnchor[ 0 ] ); // アンカー→自分。
	float dist = rope.length();
	if (dist <= mWireMaxLength || dist < 0.0001f) {
		return; // 上限内なら何もしない（バネに任せる）。
	}
	float inv = 1.0f / dist;
	Vector3 n( rope.x * inv, rope.y * inv, rope.z * inv );
	mPosition.setMadd( mAnchor[ 0 ], n, mWireMaxLength ); // 最大距離の球面へ引き戻す。
	float vr = mVelocity.dot( n );
	if (vr > 0.0f) { mVelocity.madd( n, -vr ); } // 外向き速度を消す。
}

void Player::releaseWire() {
	mWireSwinging = false;
	mWireCooldown = mWireCooldownFrames;
	// 速度はそのまま維持する（溜まった接線速度で飛び出す＝立体機動の飛翔感）。
	// 離脱後もしばらくワイヤーを残して見せる。手元位置を固定する。
	mWireVisibleCount = mWireLingerFrames;
	Vector3 hand( mPosition.x, mPosition.y + mChestHeight, mPosition.z );
	for (int i = 0; i < kWireCount; ++i) {
		mWireDrawFrom[ i ] = hand; // 実際に描くのは mWireActive な本だけ。
	}
}

void Player::updateAttack( Target* target ) {
	if (mFireCooldown > 0) { --mFireCooldown; }
	// スイング中は撃たない（機動に専念）。
	if (InputFire() && mFireCooldown == 0 && !mWireSwinging) {
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

bool Player::getWireEndpoint( int index, Vector3* from, Vector3* to ) const {
	if (index < 0 || index >= kWireCount) { return false; }
	if (!mWireActive[ index ]) { return false; } // その本は張られていない。
	if (mWireSwinging) { // スイング中は手元＝現在の胸の位置。
		from->set( mPosition.x, mPosition.y + mChestHeight, mPosition.z );
	}
	else { // 離脱後の残像は、離脱時点の手元位置で固定して見せる。
		*from = mWireDrawFrom[ index ];
	}
	*to = mAnchor[ index ];
	return true;
}

float Player::getWireStretch( int index ) const {
	// 手元〜アンカーの実距離が自然長をどれだけ超えているか（メートル）を返す（色演出用）。
	// 記事の ApplyStringColor と同じく、正規化せず絶対の超過量をテンションとして渡す。
	if (index < 0 || index >= kWireCount) { return 0.0f; }
	if (!mWireActive[ index ]) { return 0.0f; }
	float length = mWireLength[ index ];
	if (length < 0.0001f) { return 0.0f; }
	Vector3 from = mWireSwinging
		? Vector3( mPosition.x, mPosition.y + mChestHeight, mPosition.z )
		: mWireDrawFrom[ index ];
	Vector3 rope;
	rope.setSub( mAnchor[ index ], from );
	float stretch = rope.length() - length; // 自然長超過（m）。
	return stretch > 0.0f ? stretch : 0.0f;  // たるみ時は0。
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
