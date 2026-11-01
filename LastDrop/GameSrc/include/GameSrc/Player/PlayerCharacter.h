#ifndef INCLUDED_PLAYER_CHARACTER_H
#define INCLUDED_PLAYER_CHARACTER_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/Player/Animator.h"
#include "GameSrc/Weapon/WeaponStance.h"
#include "GameSrc/Weapon/FireControl.h"
#include "GameSrc/Weapon/WeaponRecoil.h"
#include "GameSrc/Weapon/Magazine.h"
#include "GameSrc/Weapon/WeaponReload.h"
#include "GameSrc/Render/SkinnedMesh/AnimationPoseSampler.h"
#include "GameSrc/Player/PlayerCommand.h"
#include "GameSrc/Player/PlayerHealth.h"
#include "GameSrc/Player/AimSway.h"
#include "GameSrc/Audio/Gait.h"
#include "GameSrc/Collision/ICollisionBody.h"
#include "GameSrc/Collision/CharacterMover.h"
#include "GameSrc/Collision/BodyFootprint.h"
#include "GameSrc/Hit/IHittable.h"
#include "GameSrc/World/IWorldMember.h"
#include "GameSrc/Hit/HitboxLayout.h"
#include "GameSrc/Hit/HitboxSet.h"
#include <memory>

class SkinnedMeshData;
class SkinnedMeshInstance;
class StaticMeshData;
class HeldItem;
class HeldMagazine;
class FirstPersonBody;
class CollisionWorld;
class Inventory;

// 一人称(FPS)で操作する自キャラ。体は常にカメラの水平の向きを向き、WASD はその向き基準の前後・横歩きになる。
// 地面(Y=0)を歩かせ、静止／歩き／走り のアニメを切り替える。
// カメラは持たない（向きを update の引数で受け取り、目の位置を eyePosition で返す）。
// 見た目は SkinnedMeshInstance、アニメの状態遷移は Animator、手に持つ銃は HeldItem、
// 武器の構えの段階（しまう／持つ／覗く）は WeaponStance、
// 一人称の体の見せ方（視線に合わせた背骨の曲げ・覗くときに銃を目の前へ寄せる・撃った反動で銃を跳ねさせる・頭を消す）は FirstPersonBody に任せる（委譲）。
// 反動の量（撃つと跳ねて時間で戻る）は WeaponRecoil が決める。ここの反動は銃の見た目だけ（視点の跳ね上がりは、カメラを持つ FirstPersonControl の ViewRecoil）。
// 武器は出し入れの操作で切り替え、持っている間は覗く操作で覗く（短押しで覗いたまま・長押しで押している間）。
// 入力デバイスは読まない。操作は PlayerCommand で受け取る（キー割り当てはシーン側）。
// 移動は CharacterMover に任せ、建物やゾンビに当たったら面に沿って滑る（Collide-and-Slide）。
// 移動の当たり判定は体を真上から囲む長方形（BodyFootprint）で、体の向きに合わせて回る。
// 前の辺だけは、今の目の位置の少し先まで伸ばす（目が壁を越えて建物の中が見えないように。見下ろすと目が前へ出るぶん、体が壁から離れる）。
// 銃を持っている間は、手に持った銃を囲む箱も含める（銃口が壁に入らないように。銃のぶん、体が壁から離れる）。
// それを差し出し、ゾンビ側の移動も止められるようにしている（ICollisionBody）。
// 部位ごとの当たり判定（命中判定用）も持つ（IHittable）。一人称の見た目の書き換え（頭を潰す等）の前のポーズに付く。
//
// 撃つ操作からは「このティックに1発撃ったか」だけを決める（連射の間隔は FireControl に任せる）。
// 弾を出す・銃声を鳴らすのは、それを読む側（GameWorld・PlayerBodySounds）。武器をしまっている間と、走っている間（銃を抱えている）は撃てない。
//
// 弾倉の弾の数は Magazine、リロードの時間割は WeaponReload に任せる。予備の弾は持ち物（Inventory の ITEM_AMMO）にあり、
// リロードを込め終えた瞬間に、そこから弾倉の足りない分だけ移す（弾倉に残っていた弾は捨てない）。
// 弾倉が空で撃つ番が来たら、弾は出ずに空撃ちになる（dryFiredThisTick）。引き金を離すまで次の空撃ちはしない。
// リロード中は撃てない・覗けない・走れない（歩ける）。武器をしまうとリロードをやめる（弾は込めない）。
// リロード中の腕は、リロードのアニメ（"rifle_reload"）のポーズを AnimationPoseSampler で求め、FirstPersonBody が腕だけ差し替える（脚は歩き・静止のまま）。
//
// 体力は体全体で 1 つ（PlayerHealth。バイオハザード式）。ゾンビの腕が当たると減る（applyZombieHit）。
// 傷つくほど（Caution・Danger）歩く・走るのが遅い。0 で死に、それからは操作を受け付けない（その場で止まる）。
// どのゾンビが攻撃したか・死んだ後どうするか（暗転・ゲームオーバー）は知らない（GameWorld・シーンが決める）。
//
// v4 アクション用の Player（ワイヤー・射撃）とは別物。ジャンプはまだ持たない。
class PlayerCharacter : public ICollisionBody, public IHittable, public IWorldMember{
public:
	// data: 形・見た目・アニメ（素手の "idle" / "walk" / "run"、銃の "rifle_idle" / "rifle_walk" / "rifle_run" / "rifle_reload"）。所有権は受け取らない。nullptr 不可、isReady() 済みであること。
	//       このキャラより長く生きていること（MeshCache が持つ）。
	// weapon: 右手に持たせる銃（Resource/rifle.txt。弾倉を除いた形）。所有権は受け取らない。nullptr なら素手で描く。
	//         data と同じく、このキャラより長く生きていること。
	// magazine: 銃の弾倉（Resource/rifle_mag.txt。銃と同じ座標）。リロードで左手が抜いて挿す。所有権は受け取らない。
	//           nullptr なら弾倉を描かない（銃に穴が開いて見えるだけで落ちない）。data と同じく、このキャラより長く生きていること。
	// world: 移動で当たる相手の問い合わせ先。所有権は受け取らない。nullptr 不可。このキャラより長く生きていること。
	// position / angleY: 最初の足元の位置と向き（度。0 は +Z）。
	PlayerCharacter( const SkinnedMeshData* data, const StaticMeshData* weapon, const StaticMeshData* magazine, const CollisionWorld* world,
		const GameLib::Math::Vector3& position, float angleY );
	~PlayerCharacter();
	PlayerCharacter( const PlayerCharacter& ) = delete;
	PlayerCharacter& operator=( const PlayerCharacter& ) = delete;

	// 操作に従って1フレーム（1ティック）動かし、アニメを進める。頂点の変形は draw で行う。
	// command: この1フレームの操作。入力デバイスは読まない（作るのは呼び出し側）。
	// cameraYaw: カメラの水平の向き（度。FirstPersonCamera::yaw）。体はこの向きを向き、W がこの向きへの前進になる。
	// cameraPitch: カメラの上下の向き（度。FirstPersonCamera::pitch）。背骨をこのぶん曲げ、腕と銃を視線に付いてこさせる。
	// inventory: 予備の弾（ITEM_AMMO）を持つ持ち物。リロードを込め終えたら、ここから減らす。所有権は受け取らない。nullptr 不可。
	void update( const PlayerCommand& command, float cameraYaw, float cameraPitch, Inventory* inventory );
	void draw() const;

	const GameLib::Math::Vector3& position() const;	// 足元の位置
	// 目の位置（ワールド座標。カメラの視点）。頭の骨に付いていくので、歩き・走りの揺れがそのまま出る。
	// update の後に呼ぶ（その update で見せたポーズの頭の位置を返す）。
	GameLib::Math::Vector3 eyePosition() const;
	// 覗き具合（0〜1。WeaponStance::aimWeight）。カメラのズームを合わせるのに使う。
	float aimWeight() const;

	// ── 音など、見た目以外の係が読む状態（このクラスは音を知らない。読む側が鳴らし方を決める）──
	Gait gait() const;			// 今の歩き方（アニメの状態から。素手か銃かは含まない）
	// 歩き・走りのアニメが1周の何割まで進んだか（0〜1。update で見せたポーズの位相）。足音を足の動きに合わせるのに使う。
	float gaitPhase() const;
	bool isWeaponDrawn() const;	// 武器を持っているか
	bool isAiming() const;		// 覗いているか（覗き始め・終わりの途中も true）
	bool firedThisTick() const;	// 直前の update で1発撃ったか（弾を出す・銃声を鳴らす側が読む）
	bool dryFiredThisTick() const;	// 直前の update で、弾倉が空のまま引き金を引いたか（空撃ちの音を鳴らす側が読む）
	bool isReloading() const;		// リロード中か
	WeaponReload::Cue reloadCueThisTick() const;	// 直前の update で来たリロードの合図（弾倉を抜く・挿す・槓桿。音を鳴らす側が読む）
	// 銃の懐中電灯（PbrNightLighting_Plan.txt 9 章）。T で付ける／消す。銃に付いているので、照らす位置と向きは muzzle の値。
	// 付けたまま銃をしまうと、muzzle が false の間は照らさない（付けた状態は残り、出すとまた照らす）。
	bool isFlashlightOn() const;
	bool flashlightToggledThisTick() const; // 直前の update で付けた／消したか（スイッチの音を鳴らす側が読む）
	int magazineRounds() const;	// 弾倉に入っている弾の数
	// 銃口の今の位置（ワールド）と向き（長さ1）。マズルフラッシュを描く側が描くたびに読む。
	// 銃を持っていない・しまっている・まだポーズが無ければ false（out は変えない）。
	bool muzzle( GameLib::Math::Vector3* position, GameLib::Math::Vector3* direction ) const;

	// ── 照準のぶれ（SummaryPlan/個別仕様/AimSway_Plan.txt）──
	// ぶれを決めるのに要る今の状態（持っているか・覗き具合・歩きの速さと位相・抱えている割合・壊れた腕の数）。update の後に呼ぶ。
	AimSwayBody aimSwayBody() const;
	// 照準のぶれ（度。FirstPersonCamera::setWeaponSway と同じ角度）。次の update で銃の見た目に入る（1 ティック遅れ。反動の見た目と同じ）。
	void setWeaponSway( float yawDegree, float pitchDegree );

	// ── 体力（ゾンビの攻撃。SummaryPlan/個別仕様/ZombieAttack_Plan.txt の 8 章）──
	// ゾンビの腕が当たった（立っているゾンビも、足元から腕を伸ばす這うゾンビも同じ）。damage: ダメージの量。死んだ後は何もしない。
	void applyZombieHit( float damage );
	bool canBeAttacked() const;		// ゾンビが攻撃してよいか（生きている）
	bool isDead() const;			// 死んだか（PlayerHealth::isDead）
	void toggleInvincible();      // 無敵を切り替える（デバッグ用。無敵の間は体力が減らない）
	bool isInvincible() const;    // 無敵か（F6 のエディタに出すため）
	const PlayerHealth& health() const;

	// 移動の当たり判定（体を真上から囲む長方形。体の向きに合わせて回る）。
	virtual BoxCollider collider() const;
	// 部位ごとの当たり判定。
	virtual const HitboxSet& hitboxes() const;
	// 移動の名簿（体の長方形）と命中の名簿（部位の箱）の両方へ、自分を載せる／外す（IWorldMember）。
	virtual void addTo( CollisionWorld* collisionWorld, HitWorld* hitWorld );
	virtual void removeFrom( CollisionWorld* collisionWorld, HitWorld* hitWorld );

private:
	// 今できない操作を外した操作（死んだら何もしない・脚が壊れていたら走らない）。
	PlayerCommand allowedCommand( const PlayerCommand& command ) const;
	// 操作の前後・横の量から移動方向（水平・長さ1）を求める。移動の操作が無ければ false。
	bool readMoveDirection( const PlayerCommand& command, float cameraYaw, GameLib::Math::Vector3* direction ) const;
	// 移動の速さを targetSpeed へ、1フレームの加速・減速の上限まで近づける。
	void approachSpeed( float targetSpeed );
	// 両手持ちの効き具合を、武器を持っていれば 1、しまっていれば 0 へ1フレームぶん近づける。
	void approachHoldWeight();
	// 胸元への寄せ具合を、銃を持って走っていれば 1、それ以外は 0 へ1フレームぶん近づける。
	void approachCarryWeight( Animator::State next );
	// アニメの状態を切り替える（同じ状態なら何もしない）。前のポーズからクロスフェードでつなぐ。
	void changeState( Animator::State );
	// R でリロードを始め・しまったらやめ、時間割を進め、込め終えたら予備の弾を弾倉へ移す（update から。構えの段階を決めた後）。
	void updateReload( const PlayerCommand& command, Inventory* inventory );
	bool canStartReload( const Inventory& inventory ) const; // 銃がある・リロード中でない・弾倉が満タンでない・予備がある
	// 撃つ番が来たら、弾倉に弾があれば 1 発撃ち、空なら空撃ちにする。
	// pulling: 撃てる状態で引き金を引いているか。triggerHeld: 撃てるかに関わらず引き金を引いているか（離したら空撃ちの止めを外す）。
	void updateTrigger( bool pulling, bool triggerHeld );
	// 弾倉を銃から外して左手に持たせる（弾倉を抜く合図のティックに、update の最後から）。
	void takeOutMagazine();

	// mMesh が借りる（setPoseModifier）ので、mMesh より先に宣言する（＝後に破棄される）。
	std::unique_ptr< FirstPersonBody > mBody;
	std::unique_ptr< SkinnedMeshInstance > mMesh;
	std::unique_ptr< HeldItem > mWeapon; // 持っていなければ空
	std::unique_ptr< HeldMagazine > mMagazineModel; // 弾倉の見た目。銃・弾倉・支える手のどれかが無ければ空
	Animator mAnimator;
	WeaponStance mStance;
	FireControl mFire;         // 連射の間隔
	bool mFiredThisTick;       // 直前の update で1発撃ったか
	WeaponRecoil mRecoil;      // 撃った反動の量（FirstPersonBody::setRecoil へ渡す）
	Magazine mMagazine;        // 弾倉の弾の数
	WeaponReload mReload;      // リロードの時間割と、アニメの何フレーム目を見せるか
	AnimationPoseSampler mReloadArms;  // リロードのアニメのポーズを求める（腕の差し替えに使う）
	std::vector< GameLib::Math::Matrix34 > mReloadPose; // 求めたポーズ（FirstPersonBody が次の姿勢の計算まで借りる）
	bool mDryFiredThisTick;    // 直前の update で空撃ちしたか
	bool mDryFireLatched;      // 空撃ちした後、まだ引き金を離していないか（離すまで次の空撃ちをしない）
	bool mFlashlightOn;        // 銃の懐中電灯が付いているか（始めは消えている）
	bool mFlashlightToggledThisTick; // 直前の update で付けた／消したか
	//無敵モードの判定
	bool mInvincible = false;
	CharacterMover mMover;    // 当たりながら滑って進む移動
	BodyFootprint mFootprint;  // 移動の当たり判定の大きさ
	HitboxLayout mHitboxLayout; // 部位の箱の設計図（自キャラは1人なので自分で持つ）。mHitboxes が借りるので先に宣言する
	HitboxSet mHitboxes;       // 部位ごとの当たり判定
	GameLib::Math::Vector3 mPosition;
	float mAngleY; // 向き（度）。setAngleY と同じ約束で、0 は +Z。一人称なので常にカメラのヨーと同じ
	GameLib::Math::Vector3 mMoveDirection; // 最後に進んだ向き（水平・長さ1）。入力を離した後の減速中もこの向きへ進む
	float mSpeed;                          // 今の移動の速さ（m/フレーム）。目標の速さへ加速・減速して近づく
	float mHoldWeight;                     // 両手持ちの効き具合（FirstPersonBody::setHoldWeight へ渡す）
	float mCarryWeight;                    // 胸元への寄せ具合（FirstPersonBody::setCarryWeight へ渡す）
	PlayerHealth mHealth;
};

#endif
