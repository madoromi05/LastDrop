#include "GameSrc/Player/PlayerCharacter.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Math/AngleMath.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshData.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshInstance.h"
#include "GameSrc/Render/StaticMesh/StaticMeshData.h"
#include "GameSrc/Player/HeldItem.h"
#include "GameSrc/Player/HeldMagazine.h"
#include "GameSrc/Player/FirstPersonBody.h"
#include "GameSrc/Collision/CollisionWorld.h"
#include "GameSrc/Hit/HitWorld.h"
#include "GameSrc/Interaction/Inventory.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"
#include "GameLib/Math/Functions.h"

#include <algorithm>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 速さは「毎秒」で決め、1フレーム（1ティック。GameTime.h）ぶんに直して使う（update は1ティックに1回）。
	// 歩き・走りの速さ（m/フレーム）は、アニメの足運びに合わせる（合わないと足が地面を滑って見える）。
	// 値は元アニメ（In Place にする前）が1周で進む距離から測った。
	const float kWalkSpeed      = 1.4f / GameTime::kTicksPerSecond; // Walking（素手）
	const float kRunSpeed       = 3.2f / GameTime::kTicksPerSecond; // Slow Run（素手）
	const float kRifleWalkSpeed = 1.0f / GameTime::kTicksPerSecond; // walking（Shooter Pack）: 1.36m / 1.37秒
	const float kRifleRunSpeed  = 3.1f / GameTime::kTicksPerSecond; // rifle run（Shooter Pack）: 2.30m / 0.73秒
	const float kStopSpeed      = 0.f;
	// 加速・減速（m/フレーム²）。毎秒²の値をフレーム²に直す。
	// 止まる→走るは約0.4秒、走る→止まるは約0.27秒。減速を強めにしているのは、止まりたいときに滑り続けると操作が重く感じるため。
	const float kAcceleration = 8.f / ( GameTime::kTicksPerSecond * GameTime::kTicksPerSecond );
	const float kDeceleration = 12.f / ( GameTime::kTicksPerSecond * GameTime::kTicksPerSecond );
	// アニメを切り替えるときに前後のポーズを混ぜる長さ（フレーム）。0.2秒。
	const float kCrossFadeFrames = 0.2f * GameTime::kTicksPerSecond;
	const float kNoWeight   = 0.f; // 補正の効き具合（両手持ち）
	const float kFullWeight = 1.f;

	const float kMinInputLength = 0.001f; // これより短い入力（逆方向キーの同時押しなど）は静止とみなす
	// ゾンビに離されてから、またつかまれるようになるまで（秒。SummaryPlan/個別仕様/ZombieAttack_Plan.txt の 3 章）。
	const float kGrabCooldownSeconds = 1.f;
	const int kGrabCooldownTicks = GameTime::secondsToTicks( kGrabCooldownSeconds );
	// 骨の名前の頭の共通部分（移動の当たり判定・部位ごとの当たり判定で骨を引くのに使う）。
	const char* kBoneNamePrefix = "mixamorig:";
	const float kFootprintPadding = 0.f; // 移動の当たり判定は体ぴったり（狭い所を通れるように）
	// 目から、移動の当たり判定の前の辺までの余白（m）。これより近くに壁が来ないので、視点が壁を越えて建物の中が見えることが無い。
	// カメラは目の前 5cm（FirstPersonCamera の kNearClip）より近くを描かず、その面の隅は目から約 7.7cm 離れている
	// （縦の視野角 60 度・16:9）。斜めに壁を見ても隅が壁に入らないよう、それより少し大きくする。
	const float kEyeToWallMargin = 0.1f;
	// 手に持った銃を囲む箱から、移動の当たり判定の辺までの余白（m）。銃口を壁に入れないため（銃口の光やフラッシュが壁の向こうへ出ないように）。
	// 銃口の少し前に置くもの（マズルフラッシュの光）まで壁の手前に収まるよう、目の余白と同じだけ取る。
	const float kWeaponToWallMargin = 0.1f;

	// 状態ごとの決まりごとの表。状態について知りたいこと（アニメ名・速さ・歩き方・武器を持っているか）は、すべてここから引く。
	// 状態を足すときは、Animator::State に1つ足し、この表に1行足すだけでよい（状態を並べた switch や if は他に無い）。
	// アニメ名は tools/configs/player.json の extra_animations の name。
	// 武器をしまっている間は素手のアニメ、持っている間は銃を肩の高さに持つアニメ（Basic Shooter Pack）。
	// 覗く（ADS）は専用のアニメを持たず、持っているアニメのまま FirstPersonBody が腕と銃を目の前へ寄せる。
	// 先頭は、アニメが見つからない状態・表に無い状態の代わりに使う静止の行。
	const bool kBareHands = false;
	const bool kWeaponDrawn = true;

	// リロード中に腕だけ差し替えるアニメ（Basic Shooter Pack の reloading。tools/configs/player.json の extra_animations）。
	const char* kReloadAnimation = "rifle_reload";

	// 銃口の先端（銃のモデル座標。m）。rifle.txt の一番前の頂点（z = 0.471）での銃身の中心。
	const Vector3 kMuzzlePoint( 0.f, 1.541f, 0.471f );
	// 銃のモデルの銃口の向き（+Z）。
	const Vector3 kMuzzleAxis( 0.f, 0.f, 1.f );

	struct StateAnimation{
		Animator::State state;
		const char* name;
		float speed;  // この状態で進む速さ（m/フレーム）
		Gait gait;    // 歩き方（足音・足の位相の引き継ぎに使う）
		bool drawn;   // 武器を持っている状態か
	};
	const StateAnimation kStateAnimations[] = {
		{ Animator::STATE_IDLE,       "idle",       kStopSpeed,      GAIT_STILL, kBareHands   },
		{ Animator::STATE_WALK,       "walk",       kWalkSpeed,      GAIT_WALK,  kBareHands   },
		{ Animator::STATE_RUN,        "run",        kRunSpeed,       GAIT_RUN,   kBareHands   },
		{ Animator::STATE_RIFLE_IDLE, "rifle_idle", kStopSpeed,      GAIT_STILL, kWeaponDrawn },
		{ Animator::STATE_RIFLE_WALK, "rifle_walk", kRifleWalkSpeed, GAIT_WALK,  kWeaponDrawn },
		{ Animator::STATE_RIFLE_RUN,  "rifle_run",  kRifleRunSpeed,  GAIT_RUN,   kWeaponDrawn },
	};

	// 状態の行。表に無い状態は、先頭（素手の静止）の行を返す。
	const StateAnimation& stateAnimation( Animator::State state ) {
		for ( const StateAnimation& entry : kStateAnimations ) {
			if ( entry.state == state ) {
				return entry;
			}
		}
		return kStateAnimations[ 0 ];
	}

	// 入力から次の状態を選ぶ。武器を持っているか × 静止／歩き／走り の組み合わせを、表から探す。
	Animator::State chooseState( bool moving, bool running, bool drawn ) {
		Gait gait = !moving ? GAIT_STILL : ( running ? GAIT_RUN : GAIT_WALK );
		for ( const StateAnimation& entry : kStateAnimations ) {
			if ( entry.gait == gait && entry.drawn == drawn ) {
				return entry.state;
			}
		}
		ASSERT( false && "PlayerCharacter : no state for this gait and weapon." );
		return kStateAnimations[ 0 ].state;
	}

	// ── 銃の持ち方（右手の骨の空間）──
	// 構え（rifle aiming idle の1フレーム目）で、右手が握りを、左手がハンドガードの下を支える位置を
	// Blender で求めた値。Shooter Pack の歩き・走りでも同じ持ち方のまま右手に付いていく。
	const char* kWeaponBone = "mixamorig:RightHand";
	const Vector3 kGripOnWeapon( 0.f, 1.438f, -0.169f );   // 銃のモデル上の握りの中心（m。rifle.txt の座標）
	const Vector3 kGripInHand( 0.354f, 0.38f, -9.102f );  // 握りの中心を置く、手の骨から見た位置（cm。骨格の単位）
	// 手の骨から見た銃の向き（度）。骨格と同じく Y→X→Z の順に掛ける（Matrix34 の rotateY / X / Z の順）。
	const float kGripRotationY = -156.062f;
	const float kGripRotationX = 2.115f;
	const float kGripRotationZ = 89.224f;
	// 骨格は cm（根に 0.01 倍が入っている）、銃は m なので、骨の空間では銃を100倍して等倍に戻す。
	const float kWeaponScaleInBone = 100.f;
	// 覗いたときに目を置く点（銃のモデル座標。m）。AKM のサイト線（リアサイト z≈0.05 → フロントサイト z≈0.43、
	// どちらも上端 y≈1.597）を、頂点の上端の並びから求め、ほお付けの位置まで後ろへ延ばした所。
	// 目の前のリアサイトが大きく見えすぎる・機関部が視界を塞ぐなら z を、サイトが上下にずれるなら y を調整する。
	const Vector3 kSightEyeOnWeapon( 0.f, 1.600f, -0.12f );
	// 銃を支える手。どのアニメでも、この手の骨が銃の上の決まった点に来るよう右手首を曲げる（FirstPersonBody）。
	const char* kSupportHandBone = "mixamorig:LeftHand";
	// 持ち方（kGripInHand など）を合わせたアニメと時刻。このポーズで左手が銃のどこにあるかを「左手の来るべき点」にする。
	// 数値で持たずにアニメから求めるのは、持ち方を合わせ直したときに一緒に合わせ直す値を増やさないため。
	const char* kGripCalibrationAnimation = "rifle_idle"; // rifle aiming idle
	const float kGripCalibrationTime = 0.f;               // 1フレーム目

	// 銃のモデル座標 → 手の骨の空間 の行列。握りの中心を原点へ動かし、大きさ・向きを合わせてから手の位置へ置く。
	Matrix34 weaponGrip() {
		Matrix34 grip;
		grip.setTranslation( kGripInHand );
		grip.rotateY( kGripRotationY );
		grip.rotateX( kGripRotationX );
		grip.rotateZ( kGripRotationZ );
		grip.scale( kWeaponScaleInBone );
		grip.translate( -kGripOnWeapon.x, -kGripOnWeapon.y, -kGripOnWeapon.z );
		return grip;
	}

	// 持ち方を合わせたアニメで、支える手の骨が銃のどこにあるか（銃のモデル座標）を out へ書く。
	// そのアニメが無ければ false（両手持ちの補正をしない）。起動時に1回だけ呼ぶ（骨格を1つ作って捨てる）。
	bool findSupportHandOnWeapon( const SkinnedMeshData* data, int hand, int supportHand, const Matrix34& grip, Vector3* out ) {
		Scene::Animation calibration = data->animation( kGripCalibrationAnimation );
		if ( !calibration ) {
			GameLib::cout << "PlayerCharacter: animation not found: " << kGripCalibrationAnimation << GameLib::endl;
			return false;
		}
		Scene::Tree skeleton = data->createSkeleton();
		skeleton.setAnimation( calibration );
		skeleton.updateAnimation( kGripCalibrationTime );
		std::vector< Matrix34 > pose( data->boneNumber() );
		data->computeBonePose( skeleton, &pose );

		Matrix34 toWeapon; // 骨格の根の空間 → 銃のモデル座標
		toWeapon.setMul( pose[ hand ], grip );
		toWeapon.invert();
		const Matrix34& support = pose[ supportHand ];
		toWeapon.mul( out, Vector3( support.m03, support.m13, support.m23 ) );
		return true;
	}

	// 補正の効き具合を target へ1フレームぶん近づける。アニメのクロスフェードと同じ長さで効かせる／外す
	// （素手⇔銃・歩き⇔走りのポーズの切り替えに、補正が急に入ると腕や手首が跳ねて見えるため）。
	float approachWeight( float weight, float target ) {
		float step = kFullWeight / kCrossFadeFrames;
		if ( weight < target ) {
			return Math::min( target, weight + step );
		}
		return Math::max( target, weight - step );
	}

	// 足を交互に出す繰り返しのアニメか。この同士の切り替え（歩き⇔走り、素手⇔銃）だけ足の位相を合わせる。
	// 4本の歩き・走りは、どれも1周の前半で左足・後半で右足が前に出る（元の FBX で確認済み）ので、
	// 「1周の何割まで進んだか」を引き継げば足の出方が揃う。
	bool isGaitCycle( Animator::State state ) {
		return stateAnimation( state ).gait != GAIT_STILL;
	}
}

PlayerCharacter::PlayerCharacter( const SkinnedMeshData* data, const StaticMeshData* weapon, const StaticMeshData* magazine, const CollisionWorld* world,
const Vector3& position, float angleY ) :
	mBody( std::make_unique< FirstPersonBody >( data ) ),
	mMesh( std::make_unique< SkinnedMeshInstance >( data ) ),
	mFiredThisTick( false ),
	mReloadArms( data, kReloadAnimation ),
	mDryFiredThisTick( false ),
	mDryFireLatched( false ),
	mMover( world, this ), // this は覚えるだけ（位置を読むのは move のとき＝構築の後）
	mFootprint( data, kBoneNamePrefix, kFootprintPadding ),
	mHitboxLayout( data, kBoneNamePrefix ),
	mHitboxes( &mHitboxLayout ),
	mPosition( position ),
	mAngleY( AngleMath::wrapSigned( angleY ) ),
	mMoveDirection( 0.f, 0.f, 0.f ),
	mSpeed( 0.f ),
	mHoldWeight( 0.f ),
	mCarryWeight( 0.f ),
	mHeld( false ),
	mGrabCooldownTicks( 0 ) {
	mMesh->setPoseModifier( mBody.get() );

	// 状態にアニメを登録する。見つからないアニメは静止のアニメで代用する
	// （レスト姿勢＝Tポーズのまま滑るより目立たない。変換し忘れはデバッグ出力で知らせる）。
	Scene::Animation idle = data->animation( kStateAnimations[ 0 ].name );
	for ( const StateAnimation& entry : kStateAnimations ) {
		Scene::Animation animation = data->animation( entry.name );
		if ( !animation ) {
			GameLib::cout << "PlayerCharacter: animation not found: " << entry.name << GameLib::endl;
			animation = idle;
		}
		mAnimator.setAnimation( entry.state, animation );
	}
	// Animator は静止状態から始まる（request は同じ状態への遷移を無視する）ので、最初のアニメはここで付ける。
	// 武器を持って始まるなら、最初から持った静止のアニメにする（1フレーム目から素手→銃のフェードを見せない）。
	mAnimator.request( chooseState( false, false, mStance.isDrawn() ) );
	mMesh->setAnimation( mAnimator.animation(), 0.f );

	if ( weapon ) {
		int hand = data->findBone( kWeaponBone );
		if ( hand < 0 ) {
			// 骨名違いでも落ちずに素手になるだけなので、ここで知らせる。
			GameLib::cout << "PlayerCharacter: weapon bone not found: " << kWeaponBone << GameLib::endl;
		} else {
			Matrix34 grip = weaponGrip();
			mWeapon = std::make_unique< HeldItem >( weapon, hand, grip );
			// 支える手が見つからなければ、右手だけで持つ（左手が銃から離れることがあるだけで落ちない）。
			int supportHand = data->findBone( kSupportHandBone );
			Vector3 supportOnWeapon( 0.f, 0.f, 0.f );
			if ( supportHand < 0 ) {
				GameLib::cout << "PlayerCharacter: support hand bone not found: " << kSupportHandBone << GameLib::endl;
			} else if ( !findSupportHandOnWeapon( data, hand, supportHand, grip, &supportOnWeapon ) ) {
				supportHand = -1;
			}
			// 銃を囲む箱（移動の当たり判定）には弾倉も含める（弾倉を分ける前と同じ大きさにする）。
			Vector3 weaponMin = weapon->boundsMin();
			Vector3 weaponMax = weapon->boundsMax();
			if ( magazine ) {
				weaponMin.set( Math::min( weaponMin.x, magazine->boundsMin().x ), Math::min( weaponMin.y, magazine->boundsMin().y ), Math::min( weaponMin.z, magazine->boundsMin().z ) );
				weaponMax.set( Math::max( weaponMax.x, magazine->boundsMax().x ), Math::max( weaponMax.y, magazine->boundsMax().y ), Math::max( weaponMax.z, magazine->boundsMax().z ) );
			}
			mBody->setWeapon( hand, grip, kSightEyeOnWeapon, supportHand, supportOnWeapon, weaponMin, weaponMax );
			// 弾倉は支える手（左手）で抜いて挿す。支える手が無ければ、弾倉は描かない。
			if ( magazine && supportHand >= 0 ) {
				mMagazineModel = std::make_unique< HeldMagazine >( magazine, supportHand );
			}
		}
	}
	mHoldWeight = mStance.isDrawn() ? kFullWeight : kNoWeight;
}

PlayerCharacter::~PlayerCharacter() {
}

void PlayerCharacter::update( const PlayerCommand& requested, float cameraYaw, float cameraPitch, Inventory* inventory ) {
	ASSERT( inventory && "PlayerCharacter::update : inventory is null." );
	PlayerCommand command = allowedCommand( requested );
	// 一人称では体の向き＝視線の向き。振り向きを遅らせると、視点と腕・銃の向きがずれて見えるため即座に合わせる。
	mAngleY = AngleMath::wrapSigned( cameraYaw );
	mBody->setPitch( cameraPitch );

	Vector3 direction;
	bool moving = readMoveDirection( command, cameraYaw, &direction );
	bool wantsRun = moving && command.run;
	if ( moving ) {
		mMoveDirection = direction;
	}
	// リロード中は覗かない（覗く入力を渡さない。押したままなら、込め終えた後に押し直したとみなして覗き始める）。
	mStance.update( command.holsterPressed, command.aimHeld && !mReload.isActive(), wantsRun );
	updateReload( command, inventory );
	// 覗いている間は走らない（覗く方を優先して歩きに落とす）。覗くのをやめれば、走る入力が続いている限り走りに戻る。
	// リロード中も走らない（弾倉を替えながら歩くだけ。バイオハザードと同じ）。
	bool running = wantsRun && !mStance.isAiming() && !mReload.isActive();
	Animator::State next = chooseState( moving, running, mStance.isDrawn() );
	// 撃てるのは、銃を手に持っていて、走っていない間だけ（走りでは銃口を下げて胸元に抱えている）。
	// リロード中も撃てない（銃を下げて弾倉を替えている）。
	// 撃てない間も FireControl は進める（次の1発までの間隔を、撃てない間に止めないため）。
	bool canFire = mWeapon && mStance.isDrawn() && !running && !mReload.isActive();
	updateTrigger( command.fireHeld && canFire, command.fireHeld );
	mRecoil.update( mFiredThisTick );
	mBody->setRecoil( mRecoil.kick() );
	// リロード中は腕をアニメへ差し替える。アニメでは左手が銃を離れるので、その間は両手持ちの補正（右手首を左手へ向ける）を弱める。
	float armWeight = 0.f;
	if ( mReload.isActive() && mReloadArms.sample( mReload.animationFrame(), &mReloadPose ) ) {
		armWeight = mReload.armWeight();
	}
	mBody->setArmOverride( armWeight > 0.f ? &mReloadPose : nullptr, armWeight );
	approachHoldWeight();
	approachCarryWeight( next );
	mBody->setHoldWeight( mHoldWeight * ( kFullWeight - armWeight ) );
	mBody->setCarryWeight( mCarryWeight );
	mBody->setAimWeight( mStance.aimWeight() );
	// 入力を離しても最後に進んでいた向きへ減速しながら進み、止まる。
	approachSpeed( stateAnimation( next ).speed * mHealth.walkSpeedScale() );
	Vector3 step;
	step.setMul( mMoveDirection, mSpeed );
	changeState( next );
	// 移動より先に姿勢を進める。移動の当たり判定は今の目の位置まで伸ばす（collider）ので、
	// このティックの姿勢（見下ろして前へ出た目）で判定しないと、1ティックだけ目が壁を越える。
	// 姿勢は骨格の根の空間で作るので、置き場所が決まる前に進めてよい。
	mMesh->setAngleY( mAngleY );
	mMesh->update();
	// 向き（mAngleY）は上で変えてある。変えた向き・伸びた当たり判定で壁にめり込んだ分も、ここで外へ出す。
	mPosition += mMover.move( step );
	mPosition.y = mMover.followGround( mPosition ); // 歩道の段差に乗る
	mMesh->setPosition( mPosition );
	mHitboxes.update( *mMesh );
	// 弾倉を抜いた瞬間の左手から見た場所を覚える（それからは左手に付いて動く）。このティックのポーズ・置き場所が決まった後に読む。
	if ( mReload.cueThisTick() == WeaponReload::CUE_MAG_OUT ) {
		takeOutMagazine();
	}
}

void PlayerCharacter::draw() const {
	mMesh->draw();
	// しまった武器は描かない（背中に背負った見た目は、一人称では見えないので持たない）。
	// 出し入れのアニメが無いので、切り替えた瞬間に手へ現れる・消える。
	if ( !mWeapon || !mStance.isDrawn() ){
		return;
	}
	mWeapon->draw( *mMesh );
	Matrix34 gun;
	if ( mMagazineModel && mWeapon->placement( *mMesh, &gun ) ) {
		mMagazineModel->draw( *mMesh, gun, mReload.isMagazineOut(), mReload.magazineSeatWeight() );
	}
}

void PlayerCharacter::takeOutMagazine() {
	Matrix34 gun;
	if ( mMagazineModel && mWeapon && mWeapon->placement( *mMesh, &gun ) ) {
		mMagazineModel->takeOut( *mMesh, gun );
	}
}

// 銃口の今の位置（ワールド）と向き（長さ1）。銃を持っていない・しまっている・まだポーズが無ければ false。
bool PlayerCharacter::muzzle( GameLib::Math::Vector3* position, GameLib::Math::Vector3* direction ) const {
	if (!mWeapon || !mStance.isDrawn()) {
		return false;
	}
	Matrix34 gun;
	if (!mWeapon->placement( *mMesh, &gun )) {
		return false; // まだポーズが無い
	}
	gun.mul( position, kMuzzlePoint );     // 点なので、移動も掛ける
	gun.mul33( direction, kMuzzleAxis );   // 向きなので、移動は掛けない
	direction->normalize();                // 持ち方・骨の縮尺を取り除いて長さ1に
	return true;
}

const Vector3& PlayerCharacter::position() const {
	return mPosition;
}

BoxCollider PlayerCharacter::collider() const {
	// 目は体（腰・胴・脚）より前にあり、見下ろすとさらに前へ出る。前の辺を、今の目の先（＋余白）まで伸ばす。
	// 目は骨格の根の空間（前方が +Z）で持っているので、z がそのまま足元から前への長さになる。
	float eyeReach = mBody->eyePosition().z + kEyeToWallMargin;
	// 持っている銃は体より前へ突き出ているので、銃を囲む箱（＋余白）も長方形に含める。銃口が壁に入らなくなる。
	// 銃の箱は今のポーズ（覗く・抱える・反動）から求めたものなので、構えを変えると長方形の大きさも変わる。
	Vector3 low;
	Vector3 high;
	if ( !mWeapon || !mStance.isDrawn() || !mBody->weaponBounds( &low, &high ) ) {
		return mFootprint.place( mPosition, mAngleY, eyeReach );
	}
	low.x -= kWeaponToWallMargin;
	low.z -= kWeaponToWallMargin;
	high.x += kWeaponToWallMargin;
	high.z = Math::max( high.z + kWeaponToWallMargin, eyeReach );
	return mFootprint.place( mPosition, mAngleY, low, high );
}

const HitboxSet& PlayerCharacter::hitboxes() const {
	return mHitboxes;
}

Vector3 PlayerCharacter::eyePosition() const {
	// FirstPersonBody が骨格の根の空間で求めた目を、置き場所・向きでワールドへ置く（頂点と同じ式）。
	Vector3 eye;
	mMesh->rootMatrix().mul( &eye, mBody->eyePosition() );
	return eye;
}

float PlayerCharacter::aimWeight() const {
	return mStance.aimWeight();
}

Gait PlayerCharacter::gait() const {
	return stateAnimation( mAnimator.state() ).gait;
}

float PlayerCharacter::gaitPhase() const {
	return mMesh->animationPhase();
}

bool PlayerCharacter::isWeaponDrawn() const {
	return mStance.isDrawn();
}

AimSwayBody PlayerCharacter::aimSwayBody() const {
	AimSwayBody body;
	body.weaponDrawn = mWeapon && mStance.isDrawn();
	body.aimWeight = mStance.aimWeight();
	// 速さの割合は銃を持った歩き（1.0m/秒）で 1。脚が壊れて遅く歩くと小さくなり、走っても 1 で止まる（AimSway が止める）。
	body.walkRate = mSpeed / kRifleWalkSpeed;
	body.gaitPhase = gaitPhase();
	body.gaitCycling = gait() != GAIT_STILL;
	body.carryWeight = mCarryWeight;
	body.danger = mHealth.condition() == PlayerCondition::Danger;
	return body;
}

void PlayerCharacter::setWeaponSway( float yawDegree, float pitchDegree ) {
	mBody->setSway( yawDegree, pitchDegree );
}

bool PlayerCharacter::isAiming() const {
	return mStance.isAiming();
}

bool PlayerCharacter::firedThisTick() const {
	return mFiredThisTick;
}

bool PlayerCharacter::dryFiredThisTick() const {
	return mDryFiredThisTick;
}

bool PlayerCharacter::isReloading() const {
	return mReload.isActive();
}

WeaponReload::Cue PlayerCharacter::reloadCueThisTick() const {
	return mReload.cueThisTick();
}

int PlayerCharacter::magazineRounds() const {
	return mMagazine.rounds();
}

void PlayerCharacter::updateReload( const PlayerCommand& command, Inventory* inventory ) {
	if ( !mStance.isDrawn() ) {
		// しまったらやめる（替えている途中の銃は手から消えるので、弾は込めない）。
		mReload.cancel();
	} else if ( command.reloadPressed && canStartReload( *inventory ) ) {
		mReload.start();
		mStance.stopAiming(); // 覗いていたら銃を下ろしてから替える
	}
	// リロード中でなくても進める（揺れを戻し、合図を消すため）。
	if ( !mReload.update() ) {
		return;
	}
	// 込め終えた。予備から弾倉の足りない分だけ移す（予備が足りなければ、ある分だけ）。
	int count = std::min( mMagazine.missingRounds(), inventory->count( ITEM_AMMO ) );
	for ( int i = 0; i < count; ++i ) {
		inventory->remove( ITEM_AMMO );
	}
	mMagazine.load( count );
}

bool PlayerCharacter::canStartReload( const Inventory& inventory ) const {
	return mWeapon && !mReload.isActive() && mMagazine.missingRounds() > 0 && inventory.count( ITEM_AMMO ) > 0;
}

void PlayerCharacter::updateTrigger( bool pulling, bool triggerHeld ) {
	mFiredThisTick = false;
	mDryFiredThisTick = false;
	if ( !triggerHeld ) {
		mDryFireLatched = false;
	}
	// 空撃ちの後は、離すまで引き金を引いていないことにする（押し続けてもカチッは 1 回。実銃も撃鉄が落ちるのは 1 回）。
	if ( !mFire.update( pulling && !mDryFireLatched ) ) {
		return;
	}
	if ( mMagazine.isEmpty() ) {
		mDryFiredThisTick = true;
		mDryFireLatched = true;
		return;
	}
	mMagazine.takeRound();
	mFiredThisTick = true;
}

PlayerCommand PlayerCharacter::allowedCommand( const PlayerCommand& command ) const {
	PlayerCommand allowed = command;
	if ( isDead() ) {
		// 死んだら、その場で止まって何もしない（押した瞬間の入力も捨てる）。
		allowed.forward = 0.f;
		allowed.strafe = 0.f;
		allowed.run = false;
		allowed.holsterPressed = false;
		allowed.aimHeld = false;
		allowed.fireHeld = false;
		allowed.reloadPressed = false;
		return allowed;
	}
	if ( mHeld ) {
		allowed.forward = 0.f; // つかまれている間は動けない（撃って振りほどく）
		allowed.strafe = 0.f;
		allowed.run = false;
	}
	return allowed;
}

void PlayerCharacter::applyBite( float damage ) {
	if (mInvincible) return;
	mHealth.takeDamage( damage );
}

void PlayerCharacter::setHeld( bool held ) {
	if ( mHeld && !held ) {
		mGrabCooldownTicks = kGrabCooldownTicks; // 離された
	} else if ( !held && mGrabCooldownTicks > 0 ) {
		--mGrabCooldownTicks;
	}
	mHeld = held;
}

bool PlayerCharacter::isHeld() const {
	return mHeld;
}

bool PlayerCharacter::canBeGrabbed() const {
	return !isDead() && !mHeld && mGrabCooldownTicks <= 0;
}

bool PlayerCharacter::isDead() const {
	return mHealth.isDead();
}

const PlayerHealth& PlayerCharacter::health() const {
	return mHealth;
}

bool PlayerCharacter::readMoveDirection( const PlayerCommand& command, float cameraYaw, Vector3* direction ) const {
	// カメラの水平の前方と右（FirstPersonCamera と同じ約束: 前方はヨー0で +Z、右は 前方×上）。
	Vector3 cameraForward( Math::sin( cameraYaw ), 0.f, Math::cos( cameraYaw ) );
	Vector3 cameraRight( -Math::cos( cameraYaw ), 0.f, Math::sin( cameraYaw ) );
	direction->set( 0.f, 0.f, 0.f );
	direction->madd( cameraForward, command.forward );
	direction->madd( cameraRight, command.strafe );
	if ( direction->length() < kMinInputLength ) {
		return false;
	}
	direction->normalize(); // 斜め入力で速くならないように
	return true;
}

void PlayerCharacter::approachSpeed( float targetSpeed ) {
	if ( mSpeed < targetSpeed ) {
		mSpeed = Math::min( targetSpeed, mSpeed + kAcceleration );
	} else {
		mSpeed = Math::max( targetSpeed, mSpeed - kDeceleration );
	}
}

void PlayerCharacter::approachHoldWeight() {
	mHoldWeight = approachWeight( mHoldWeight, mStance.isDrawn() ? kFullWeight : kNoWeight );
}

void PlayerCharacter::approachCarryWeight( Animator::State next ) {
	// 状態で決めるのは、走りのアニメ（肩の高さで構えたまま）に合わせて抱え方を足すため。素手で走るときは抱える銃が無い。
	const StateAnimation& entry = stateAnimation( next );
	bool carrying = entry.drawn && entry.gait == GAIT_RUN;
	mCarryWeight = approachWeight( mCarryWeight, carrying ? kFullWeight : kNoWeight );
}

void PlayerCharacter::changeState( Animator::State next ) {
	Animator::State previous = mAnimator.state();
	if ( !mAnimator.request( next ) ) {
		return;
	}
	Scene::Animation animation = mAnimator.animation();
	// 歩き⇔走りは、前のアニメが1周の何割まで進んでいたかを引き継いで、同じ足の位置から始める。
	// それ以外（止まる⇔動く）は頭から。止まっている姿勢には「どちらの足が前か」が無いため。
	float startTime = 0.f;
	if ( isGaitCycle( previous ) && isGaitCycle( next ) ) {
		startTime = mMesh->animationPhase() * animation.period();
	}
	mMesh->crossFadeTo( animation, startTime, kCrossFadeFrames );
}

void PlayerCharacter::addTo( CollisionWorld* collisionWorld, HitWorld* hitWorld ) {
	collisionWorld->addBody( this );
	hitWorld->addTarget( this );
}

void PlayerCharacter::removeFrom( CollisionWorld* collisionWorld, HitWorld* hitWorld ) {
	collisionWorld->removeBody( this );
	hitWorld->removeTarget( this );
}

void PlayerCharacter::toggleInvincible() {
	mInvincible = !mInvincible;
}

bool PlayerCharacter::isInvincible() const {
	return mInvincible;
}
