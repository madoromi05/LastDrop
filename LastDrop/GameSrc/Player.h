#ifndef INCLUDED_PLAYER_H
#define INCLUDED_PLAYER_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Base/AutoPtr.h"
#include "GameLib/Base/Array.h"
using namespace GameLib;
using namespace GameLib::Math;

namespace GameLib{
	namespace Math{
		class Matrix34;
	}
}

class GraphicsDatabase;
class Model;
class Bullet;
class Arena;
class Target;

// ────────────────────────────────────────────────
// Player：企画書 v4 の機動部隊員。自作キャラクターコントローラーの本体。
//
// このクラスが持つ「アクション部分」：
//   ・地上移動（対象へ自動で正面を向けたロックオン移動）
//   ・回避ステップ（無敵フレーム＋クールタイム）
//   ・拘束ワイヤー移動（アンカーへ直線で自分を引き寄せる。振り子スイングは無し）
//   ・射撃で対象を弱化（既存 Bullet を流用）
//   ・被弾・体力（無敵中は無効）
//
// 実装しないもの（別スコープ）：捕縛（拘束）／任務分岐／ボスAI。
//
// 依存の入口：update( arena, target )。地形は Arena に、対象は Target に問い合わせる。
// ────────────────────────────────────────────────
class Player{
public:
	// ── 移動・接地 ──
	static const float mMaxMoveSpeed;    // 最大移動速度。
	static const int   mMoveAccelFrames; // 静止から最大速度に達するまでのフレーム数。
	static const float mTurnSpeedY;      // 対象へ向き直る1フレームあたりの角度。
	static const float mBodyRadius;      // 壁との押し戻しに使う体の半径。
	static const float mChestHeight;     // ワイヤー射出・カメラ基準の胸の高さ。
	static const float mGravity;         // ワイヤーで浮いた後に床へ戻すための重力。

	// ── 回避 ──
	static const float mDodgeSpeed;          // 回避中の速度。
	static const int   mDodgeFrames;         // 回避の継続フレーム数。
	static const int   mDodgeIFrames;        // 回避で無敵になるフレーム数。
	static const int   mDodgeCooldownFrames; // 回避後の再使用までの待ち。

	// ── 拘束ワイヤー ──
	static const float mWirePullSpeed;     // 引き寄せ速度。
	static const float mWireStopRadius;    // アンカーへこの距離まで来たら引き寄せ終了。
	static const float mWireMaxLength;      // ワイヤーが届く最大距離。
	static const int   mWireCooldownFrames; // 射出後の再使用までの待ち。
	static const int   mWireMaxFrames;      // 安全弁：引き寄せがこの長さを超えたら強制終了。
	static const int   mWireLingerFrames;   // 引き寄せ終了後もワイヤーを見せておくフレーム数（視認性）。

	// ── 体力・射撃 ──
	static const int mMaxHitPoint;        // 最大体力。
	static const int mBulletNumber;       // 同時に存在できる弾数。
	static const int mFireCooldownFrames; // 連射間隔。
	static const int mBulletDamage;       // 弾1発で対象に与える弱化量。

	// ── カメラ ──
	static const float mCameraDistanceZ;       // カメラの後方距離。
	static const float mCameraDistanceY;       // カメラの高さ。
	static const float mCameraTargetDistanceZ; // 注視点を前方へ伸ばす距離。

	Player( const Vector3& position );
	~Player();

	// 1フレーム更新。地形と対象を受け取り、機動・射撃・被弾を処理する。
	void update( const Arena& arena, Target* target );
	void draw() const;                                    // 本体＋弾。
	void getView( Matrix34* viewMatrix, Vector3* eye ) const;

	const Vector3* getPosition() const;
	float getAngleY() const;

	// 被弾（無敵フレーム中は無効）。今回のダミーは攻撃してこないため通常は呼ばれないが、
	// 回避のiフレームを意味あるものにするための入口として用意しておく。
	void setDamage( int damage );

	int  getHitPoint() const;
	bool isInvincible() const;   // 回避の無敵フレーム中か。
	bool isWirePulling() const;  // 引き寄せ中か（HUD用）。

	// ワイヤー可視化：引き寄せ中だけ true を返し、線の両端を格納する。
	// 描画は Main の3Dパスが担う（Player は描画変換を持たない）。
	bool getWireEndpoints( Vector3* from, Vector3* to ) const;

	// Main のライティングが弾を光源に使うための問い合わせ。
	int getBulletNumber() const;
	const Vector3* getBulletPosition( int index ) const;

private:
	void updateFacing( const Target& target );               // 移動方向（静止時は対象）へ向き直る。
	void updateMove( const Vector3& forward, const Vector3& right ); // 通常の地上移動。
	void startDodge( const Vector3& forward, const Vector3& right ); // 回避開始。
	void startWire( const Arena& arena, const Target& target );      // アンカーを決めて引き寄せ開始。
	void updateWirePull();                                   // 引き寄せ中の速度を作る。
	void updateAttack( Target* target );                     // 射撃と弾の衝突。

	Vector3 mPosition;
	Vector3 mVelocity;
	float   mAngleY;

	int mHitPoint;
	int mInvincibleCount; // >0 の間は無敵。

	// 回避。
	Vector3 mDodgeDir;
	int mDodgeCount;    // 残り回避フレーム。
	int mDodgeCooldown; // 回避クールタイム。

	// 拘束ワイヤー。
	bool    mWirePulling;    // 引き寄せ中か。
	Vector3 mAnchor;         // 引き寄せ先。
	int     mWireCount;      // 引き寄せ経過フレーム（安全弁用）。
	int     mWireCooldown;   // ワイヤークールタイム。
	int     mWireVisibleCount; // 引き寄せ終了後にワイヤーを残して見せる残りフレーム。
	Vector3 mWireDrawFrom;   // 残像表示中の手元側の座標（終了時に固定する）。

	// 射撃。
	Array< Bullet > mBullets;
	int mFireCooldown;

	AutoPtr< GraphicsDatabase > mDatabase;
	AutoPtr< Model > mModel;
};

#endif
