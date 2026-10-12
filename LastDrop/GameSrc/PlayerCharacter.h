#ifndef INCLUDED_PLAYER_CHARACTER_H
#define INCLUDED_PLAYER_CHARACTER_H

#include "GameLib/Math/Vector3.h"
#include "Animator.h"
#include "WeaponStance.h"
#include "FireControl.h"
#include "WeaponRecoil.h"
#include "PlayerCommand.h"
#include "Gait.h"
#include "ICollisionBody.h"
#include "CharacterMover.h"
#include "BodyFootprint.h"
#include "IHittable.h"
#include "IWorldMember.h"
#include "HitboxLayout.h"
#include "HitboxSet.h"
#include <memory>

class SkinnedMeshData;
class SkinnedMeshInstance;
class StaticMeshData;
class HeldItem;
class FirstPersonBody;
class CollisionWorld;

// 一人称(FPS)で操作する自キャラ。体は常にカメラの水平の向きを向き、WASD はその向き基準の前後・横歩きになる。
// 地面(Y=0)を歩かせ、静止／歩き／走り のアニメを切り替える。
// カメラは持たない（向きを update の引数で受け取り、目の位置を eyePosition で返す）。
// 見た目は SkinnedMeshInstance、アニメの状態遷移は Animator、手に持つ銃は HeldItem、
// 武器の構えの段階（しまう／持つ／覗く）は WeaponStance、
// 一人称の体の見せ方（視線に合わせた背骨の曲げ・覗くときに銃を目の前へ寄せる・撃った反動で銃を跳ねさせる・頭を消す）は FirstPersonBody に任せる（委譲）。
// 反動の量（撃つと跳ねて時間で戻る）は WeaponRecoil が決める。反動は銃の見た目だけで、視点と弾の向きは変えない。
// 武器は出し入れの操作で切り替え、持っている間は覗く操作で覗く（短押しで覗いたまま・長押しで押している間）。
// 入力デバイスは読まない。操作は PlayerCommand で受け取る（キー割り当てはシーン側）。
// 移動は CharacterMover に任せ、建物やゾンビに当たったら面に沿って滑る（Collide-and-Slide）。
// 移動の当たり判定は体を真上から囲む長方形（BodyFootprint）で、体の向きに合わせて回る。
// 前の辺だけは、今の目の位置の少し先まで伸ばす（目が壁を越えて建物の中が見えないように。見下ろすと目が前へ出るぶん、体が壁から離れる）。
// それを差し出し、ゾンビ側の移動も止められるようにしている（ICollisionBody）。
// 部位ごとの当たり判定（命中判定用）も持つ（IHittable）。一人称の見た目の書き換え（頭を潰す等）の前のポーズに付く。
//
// 撃つ操作からは「このティックに1発撃ったか」だけを決める（連射の間隔は FireControl に任せる）。
// 弾を出す・銃声を鳴らすのは、それを読む側（GameWorld・PlayerBodySounds）。武器をしまっている間と、走っている間（銃を抱えている）は撃てない。
//
// v4 アクション用の Player（ワイヤー・射撃）とは別物。ジャンプはまだ持たない。
class PlayerCharacter : public ICollisionBody, public IHittable, public IWorldMember{
public:
	// data: 形・見た目・アニメ（素手の "idle" / "walk" / "run"、銃の "rifle_idle" / "rifle_walk" / "rifle_run"）。所有権は受け取らない。nullptr 不可、isReady() 済みであること。
	//       このキャラより長く生きていること（MeshCache が持つ）。
	// weapon: 右手に持たせる銃（Resource/rifle.txt）。所有権は受け取らない。nullptr なら素手で描く。
	//         data と同じく、このキャラより長く生きていること。
	// world: 移動で当たる相手の問い合わせ先。所有権は受け取らない。nullptr 不可。このキャラより長く生きていること。
	// position / angleY: 最初の足元の位置と向き（度。0 は +Z）。
	PlayerCharacter( const SkinnedMeshData* data, const StaticMeshData* weapon, const CollisionWorld* world,
		const GameLib::Math::Vector3& position, float angleY );
	~PlayerCharacter();
	PlayerCharacter( const PlayerCharacter& ) = delete;
	PlayerCharacter& operator=( const PlayerCharacter& ) = delete;

	// 操作に従って1フレーム（1ティック）動かし、アニメを進める。頂点の変形は draw で行う。
	// command: この1フレームの操作。入力デバイスは読まない（作るのは呼び出し側）。
	// cameraYaw: カメラの水平の向き（度。FirstPersonCamera::yaw）。体はこの向きを向き、W がこの向きへの前進になる。
	// cameraPitch: カメラの上下の向き（度。FirstPersonCamera::pitch）。背骨をこのぶん曲げ、腕と銃を視線に付いてこさせる。
	void update( const PlayerCommand& command, float cameraYaw, float cameraPitch );
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

	// 移動の当たり判定（体を真上から囲む長方形。体の向きに合わせて回る）。
	virtual BoxCollider collider() const;
	// 部位ごとの当たり判定。
	virtual const HitboxSet& hitboxes() const;
	// 移動の名簿（体の長方形）と命中の名簿（部位の箱）の両方へ、自分を載せる／外す（IWorldMember）。
	virtual void addTo( CollisionWorld* collisionWorld, HitWorld* hitWorld );
	virtual void removeFrom( CollisionWorld* collisionWorld, HitWorld* hitWorld );

private:
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

	// mMesh が借りる（setPoseModifier）ので、mMesh より先に宣言する（＝後に破棄される）。
	std::unique_ptr< FirstPersonBody > mBody;
	std::unique_ptr< SkinnedMeshInstance > mMesh;
	std::unique_ptr< HeldItem > mWeapon; // 持っていなければ空
	Animator mAnimator;
	WeaponStance mStance;
	FireControl mFire;         // 連射の間隔
	bool mFiredThisTick;       // 直前の update で1発撃ったか
	WeaponRecoil mRecoil;      // 撃った反動の量（FirstPersonBody::setRecoil へ渡す）
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
};

#endif
