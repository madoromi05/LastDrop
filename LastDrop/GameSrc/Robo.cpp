#include "Framework.h"
#include "GameLib/Input/Manager.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Math/Functions.h"
using namespace GameLib;

#include "Robo.h"
#include "Bullet.h"
#include "Pad.h"
#include "Library/GraphicsDatabase.h"
#include "Library/Model.h"
#include <sstream>
using namespace std;

// 本来はパラメータをファイルに出力すべきだろうか？
const int Robo::mJumpUpTime = 20;			// 上昇にかける時間。
const int Robo::mJumpStayTime = 60;			// 上昇から下降に切り替わるまでの滞空時間。
const int Robo::mJumpFallTime = 40;			// 下降にかける時間。
const int Robo::mMoveAccelEndCount = 30;	// 移動を始めてから最大速度になるまでの時間（フレーム数）。
const float Robo::mMaxMoveSpeed = 0.5;		// 最大移動速度。
const float Robo::mJumpHeight = 20.0;		// 最大跳躍高さ。
const int Robo::mCameraDelayCount = 10;		// ジャンプ開始から何フレームで敵の方向を向き終えるか。
const float Robo::mCameraDistanceZ = 10.0;	// カメラとのZ方向の距離。
const float Robo::mCameraDistanceY = 4.0;	// カメラの高さ。
const float Robo::mCameraTargetDistanceZ = 20.0; // カメラの注視点を前方にどれだけ伸ばすか。
const float Robo::mTurnSpeed = 1.0;			// 旋回速度。
const int Robo::mMaxHitPoint = 100;			// 最大ヒットポイント。
const int Robo::mMaxEnergy = 100;			// 最大武器エネルギー。
const int Robo::mEnergyPerBullet = 27;		// 弾1発の消費エネルギー。
const int Robo::mEnergyCharge = 1;			// 1フレームあたりのエネルギー回復量。
const float Robo::mLockOnAngleIn = 10.0;	// ロックオンする内側角度。
const float Robo::mLockOnAngleOut = 30.0;	// ロックオンが外れる外側角度。

Robo::Robo( int id ) :
	mPosition( 0.0, 0.0, 0.0 ),
	mAngleY( 0.0 ),
	mId( id ),
	mBulletNumber( 100 ), // 決め打ち。
	mCameraCount( 0 ),
	mCount( 0 ),
	mVelocity( 0.0, 0.0, 0.0 ),
	mMode( MODE_ON_LAND ),
	mHitPoint( mMaxHitPoint ),
	mEnergy( mMaxEnergy ),
	mLockOn( true ) { // 正面を向いて開始するので、最初はロックオンできている状態にしておく。
	mDatabase = NEW GraphicsDatabase( "Resource/robo.txt" );
	mModel = mDatabase->createModel( "robo" );
	mBullets.setSize( mBulletNumber );
}

Robo::~Robo() {
}

void Robo::setPosition( const Vector3& a ) {
	mPosition = a;
}

void Robo::setAngleY( float a ) {
	mAngleY = a;
}

const Vector3* Robo::getBulletPosition( int i ) const {
	if (mBullets[i].isEmpty()) {
		return 0;
	}
	else {
		return mBullets[i].getPosition();
	}
}

void Robo::update( Robo* enemy ) {
	// 撃破済みなら何もしない。
	if (mHitPoint <= 0) {
		return;
	}
	// AIの思考。プレイヤーの場合は入力をそのまま返すだけ。
	bool iJump = false;
	bool iFire = false;
	bool iLeft = false;
	bool iRight = false;
	bool iUp = false;
	bool iDown = false;
	think( &iJump, &iFire, &iLeft, &iRight, &iUp, &iDown );
	// 以降はこの入力を使って処理する。
	const Vector3& enemyPos = *enemy->getPosition();
	++mCount;
	// ジャンプ計算で使う一時変数。
	float t;
	// 状態は素直にステートマシンで分けてみよう。コードの重複は増えるが、
	// 「1ブロックの中だけ見れば何をしているか分かる」構成になる。
	// 各状態でやることを一通り見ていこう。
	switch (mMode) {
	case MODE_JUMP_UP:
		// カメラの向きが追いつくまでの間、なめらかに回転させる。
		if (mCameraCount < mCameraDelayCount) {
			mAngleY += mAngleVelocityY;
			++mCameraCount;
		}
		// 上昇中の速度。
		t = mJumpHeight / static_cast< float >( mJumpUpTime );
		mVelocity.y = t;
		if (!iJump) { // ジャンプ入力が離されたので落下に変更する。
			mMode = MODE_JUMP_FALL;
			mCount = 0;
		}
		else if (mCount >= mJumpUpTime) { // 上昇終了。
			mMode = MODE_JUMP_STAY;
			mCount = 0;
		}
		mVelocity.x = mVelocity.z = 0.0; // X,Z方向の移動は停止する。
		break;
	case MODE_JUMP_STAY:
		// カメラの向きが追いつくまでの間、なめらかに回転させる。
		if (mCameraCount < mCameraDelayCount) {
			mAngleY += mAngleVelocityY;
			++mCameraCount;
		}
		mVelocity.y = 0.0;
		if (!iJump) { // ジャンプ入力が離されたので落下に変更する。
			mMode = MODE_JUMP_FALL;
			mCount = 0;
		}
		else if (mCount >= mJumpStayTime) { // 滞空時間が終わったので落下を開始する。
			mMode = MODE_JUMP_FALL;
			mCount = 0;
		}
		break;
	case MODE_JUMP_FALL:
		// カメラの向きが追いつくまでの間、なめらかに回転させる。
		if (mCameraCount < mCameraDelayCount) {
			mAngleY += mAngleVelocityY;
			++mCameraCount;
		}
		// 下降中の速度。
		t = mJumpHeight / static_cast< float >( mJumpFallTime );
		mVelocity.y = -t;
		// 着地判定は最終的には衝突判定で行うので、ここでは判定しない。
		break;
	case MODE_ON_LAND:
		if (iJump) {
			mMode = MODE_JUMP_UP;
			mCount = 0;
			mCameraCount = 0;

			// 敵の方向に向ける。
			Vector3 dir;
			dir.setSub( enemyPos, mPosition ); // 自分から敵への方向ベクトル。
			// Y軸の角度はatan2(x, z)で求める。
			t = GameLib::Math::atan2( dir.x, dir.z );
			// 角度の差が180度を超える場合は±360度して、近い方向に回り込ませる。
			if (t - mAngleY > 180.f) {
				t -= 360.f;
			}
			else if (mAngleY - t > 180.f) {
				t += 360.f;
			}
			mAngleVelocityY = ( t - mAngleY ) / static_cast< float >( mCameraDelayCount );
		}
		else {
			move( iLeft, iRight, iUp, iDown ); // コードが長くなるので関数に分離する。
		}
		mVelocity.y = 0.f;
		break;
	}
	// 位置を更新する。地面との衝突判定は次の処理で行う。
	mPosition += mVelocity;
	if (mPosition.y < 0.f) {
		mPosition.y = 0.f;
		mMode = MODE_ON_LAND;
	}
	// 弾の発射処理。
	if (iFire) {
		// 上昇中・下降中は発射できない。
		if (( mMode != MODE_JUMP_FALL ) && ( mMode != MODE_JUMP_UP )) {
			// エネルギーは足りているか？
			if (mEnergy >= mEnergyPerBullet) {
				// 空いている弾スロットを探す。
				for (int i = 0; i < mBulletNumber; ++i) {
					if (mBullets[i].isEmpty()) {
						const char* name = ( mId == 0 ) ? "bulletB" : "bulletR";
						mBullets[i].create(
							mDatabase,
							name,
							mPosition,
							15.f,
							mAngleY,
							mLockOn );
						mEnergy -= mEnergyPerBullet;
						break;
					}
				}
			}
		}
	}
	// 弾の更新処理。
	for (int i = 0; i < mBulletNumber; ++i) {
		if (!mBullets[i].isEmpty()) {
			mBullets[i].update( enemyPos );
			// 衝突判定。
			Vector3 t;
			t.setSub( *mBullets[i].getPosition(), enemyPos );
			if (t.squareLength() < 4.f) {
				enemy->setDamage( 1 ); // ひとまず1点固定で与えてみる。
				mBullets[i].die(); // 弾を消す。
			}
		}
	}
	// エネルギーの自動回復。
	mEnergy += mEnergyCharge;
	if (mEnergy > mMaxEnergy) {
		mEnergy = mMaxEnergy;
	}
	// ロックオン判定。
	// まずは角度を求めてみよう。
	// 角度はどうやって求めるか？内積を使う。
	Vector3 toEnemy;
	toEnemy.setSub( enemyPos, mPosition );
	Vector3 myDir( 0.f, 0.f, -1.f );
	Matrix34 m;
	m.setRotationY( mAngleY + 180.f );
	m.mul( &myDir, myDir );
	float len = toEnemy.length();
	if (len > 0.f) {
		toEnemy *= 1.f / len; // 長さを1にする（正規化）。
		float dotProduct = toEnemy.dot( myDir );
		if (dotProduct > 1.f) { // 誤差で1を超えることがあるのでクランプする。
			dotProduct = 1.f;
		}
		else if (dotProduct < -1.f) {
			dotProduct = -1.f;
		}
		// 角度に変換すると、
		float angle = GameLib::Math::acos( dotProduct );
		if (mLockOn) {
			// ロックオン中なら、外れる角度まで離れたかどうかを判定する。
			if (angle > mLockOnAngleOut) {
				mLockOn = false;
			}
		}
		else {
			// ロックオンが外れている場合、戻すかどうかを判定する。
			if (angle < mLockOnAngleIn) {
				mLockOn = true;
			}
		}
		// 最終的な状態をModelに反映する。
	}
	mModel->setAngle( Vector3( 0.f, mAngleY, 0.f ) );
	mModel->setPosition( mPosition );
}

void Robo::think(
	bool* jump,
	bool* fire,
	bool* left,
	bool* right,
	bool* up,
	bool* down ) const {
	Pad* pad = Pad::instance();
	if (mId == 0) { // プレイヤー操作。
		*jump = pad->isOn( Pad::JUMP, mId );
		*fire = pad->isTriggered( Pad::FIRE, mId );
		*left = pad->isOn( Pad::LEFT, mId );
		*right = pad->isOn( Pad::RIGHT, mId );
		*up = pad->isOn( Pad::UP, mId );
		*down = pad->isOn( Pad::DOWN, mId );
	}
	else { // AI操作。
		// ジャンプ：
		// ロックオンされていなければジャンプする。
		*jump = !mLockOn;
		// 弾は撃てるだけ撃つ。
		*fire = true;
		// 移動はしない。
		// 移動を実装すると管理する変数が増えて面倒なので、今回はしない。
		*left = *right = *up = *down = false;
	}
}

void Robo::move( bool left, bool right, bool up, bool down ) {
	// 移動方向を決める。まずは単純に入力方向のベクトルを求める。
	Vector3 inputDir( 0.f, 0.f, 0.f );
	if (up) {
		inputDir.z = -1.f;
	}
	if (down) {
		inputDir.z = 1.f;
	}
	if (left) {
		inputDir.x = -1.f;
	}
	if (right) {
		inputDir.x = 1.f;
	}
	// ロボットの向きに合わせて回転させる。
	Matrix34 m;
	m.setRotationY( mAngleY + 180.f );
	m.mul( &inputDir, inputDir );

	// 停止状態からは単純に加速する。徐々に速度を上げていく。
	if (mVelocity.x == 0.f && mVelocity.z == 0.f) {
		// mMoveAccelEndCountフレームで最大速度になるように、1フレームあたりの加速度を求める。
		float accel = mMaxMoveSpeed / static_cast< float >( mMoveAccelEndCount );
		mVelocity.setMul( inputDir, accel );
	}
	else { // 既に移動している場合。
		if (inputDir.x == 0.f && inputDir.z == 0.f) { // 移動入力なし。
			mVelocity.set( 0.f, 0.f, 0.f ); // 移動を止める。
		}
		else {
			// 既に移動していて、かつ入力もある場合の処理。
			// 45度くらいの方向転換なら、そのままなめらかに方向を変えたい。
			// しかし「今の速度と方向が完全に逆」のケースもあるので、その場合の処理も必要になる。

			// 90度以上の方向転換なら速度を0にする。
			// 急停止するゲームもあるが、すぐに切り返したいなら慣性はむしろ邪魔になる。
			float dp = mVelocity.dot( inputDir );
			if (dp <= 0.f) {
				mVelocity.set( 0.f, 0.f, 0.f );
			}
			else { // 90度以内のターンなら、現在の速度と入力方向の内積はマイナスにならない。
				// 現在の移動速度から、新しい方向の成分だけを取り出す。
				// つまり処理としては、移動方向の単位ベクトルとの内積を取り、その単位ベクトルにかけ戻す。
				// 移動方向の単位ベクトルをE、現在の速度ベクトルをVとすると、新しい速度ベクトルV'は次の式になる。
				//V' = dot(V,E) * E
				// ここで、Eは移動方向ベクトルMを使ってE = M/|M|として求められるので、
				//V' = dot(V,M) * M / ( |M|^2 )
				// つまり、正規化前の方向ベクトルのままで計算できる。|M|や|M|^2の計算を省略できて楽になる。
				float moveSquareLength = inputDir.x * inputDir.x + inputDir.z * inputDir.z;
				dp = mVelocity.dot( inputDir );
				mVelocity.setMul( inputDir, dp / moveSquareLength );
			}
			// 加速する。
			// 加速度は最大速度 ÷ 加速時間で求める。
			float accel = mMaxMoveSpeed / static_cast< float >( mMoveAccelEndCount );
			mVelocity.madd( inputDir, accel );
			// 最大速度でクランプする。
			float speed = mVelocity.length();
			if (speed > mMaxMoveSpeed) {
				mVelocity *= mMaxMoveSpeed / speed;
			}
		}
	}
}

void Robo::draw() const {
	// ロボット本体の描画。
	mModel->draw();
	// 弾の描画。
	for (int i = 0; i < mBulletNumber; ++i) {
		if (!mBullets[i].isEmpty()) {
			mBullets[i].draw();
		}
	}
}

const Vector3* Robo::getPosition() const {
	return &mPosition;
}

void Robo::setDamage( int d ) {
	mHitPoint -= d;
}

int Robo::getHitPoint() const {
	return mHitPoint;
}

int Robo::getEnergy() const {
	return mEnergy;
}

bool Robo::getLockOn() const {
	return mLockOn;
}

int Robo::getBulletNumber() const {
	return mBulletNumber;
}

void Robo::getView( Matrix34* vm, Vector3* pos ) const {
	// まずは前方方向のベクトルを作成する。
	Vector3 d( 0.f, 0.f, 1.f );
	Matrix34 m;
	m.setRotationY( mAngleY );
	m.mul( &d, d );
	// 注視点は、前方にmCameraTargetDistanceZだけ伸ばした位置にする。
	Vector3 t;
	t.setMul( d, mCameraTargetDistanceZ );
	// ジャンプ中は注視点の高さも動かしたいところだが、ちゃんと計算するのは大変なので、
	// 適当な係数（0.12）で近似してしまう。
	t.y -= mPosition.y * 0.12f; // 高さ方向の補正用の、適当な係数。
	// カメラ位置は、後方にmCameraDistanceZだけ伸ばした位置にする。
	Vector3 p;
	p.setMul( d, -mCameraDistanceZ );
	// Y軸にmCameraDistanceYだけ加算する。
	p.y += mCameraDistanceY;
	// ジャンプ中はカメラ位置の高さも合わせて動かしたいが、こちらも適当な係数で近似する。
	p.y += mPosition.y * 0.12f; // 高さ方向の補正用の、適当な係数。
	// ロボットの位置を加算する。
	t += mPosition;
	p += mPosition;
	// ビュー行列を作成する。
	vm->setViewTransform( p, t, Vector3( 0.f, 1.f, 0.f ) );
	*pos = p;
}