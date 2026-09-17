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
//   ・地上移動（移動方向を向く。静止時は対象を向いて構える）
//   ・回避ステップ（無敵フレーム＋クールタイム）
//   ・立体機動ワイヤー（正面へ1本張り、バネ-ダンパでアンカーへ引かれる振り子。
//     Wire 押下中だけ張り、離すと離脱＝速度維持で射出。引き寄せボタンで手前へ寄る。
//     A/D で空中制御、手元→アンカー間の地形にはワイヤーが巻き付く）
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

	// ── 立体機動ワイヤー（正面1アンカーのバネ-ダンパ振り子）──
	static const int   kWireCount = 1;      // 同時に張るワイヤーの本数（正面へ1本）。
	static const float mWireMaxLength;      // ワイヤーが届く最大距離（＝伸びの絶対上限）。
	static const float mWireMinLength;      // 引き寄せで縮められる自然長の下限。
	static const float mWireReelSpeed;      // 1フレームで自然長を縮める長さ（引き寄せ）。
	static const float mWireSpring;         // 自然長超過1mあたりの内向き復元加速（SpringJoint相当）。
	static const float mWireDamper;         // 外向き速度成分を毎フレーム減衰させる割合。
	static const float mWireObstacleMargin; // 遮蔽判定でアンカー手前に残す余白（自己誤判定回避）。
	static const float mWireAirControl;     // スイング中の左右空中制御の加速。
	static const int   mWireCooldownFrames; // 離脱後の再射出までの待ち。
	static const int   mWireLingerFrames;   // 離脱後もワイヤーを見せておくフレーム数（視認性）。

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
	bool isSwinging() const;     // ワイヤーでスイング中か（HUD用）。
	// 自然長を超えて伸びている量（メートル。たるみ時は0）。色演出のテンション入力。
	float getWireStretch( int index ) const;

	// 設定：接地時に自動離脱するか（true=着地／false=継続）。設定画面から与える。
	void setLandOnContact( bool land );

	// ワイヤー可視化：指定した本が張られている（スイング中／残像中）なら true を返し、
	// 線の両端を格納する。描画は Main の3Dパスが担う（Player は描画変換を持たない）。
	bool getWireEndpoint( int index, Vector3* from, Vector3* to ) const;

	// Main のライティングが弾を光源に使うための問い合わせ。
	int getBulletNumber() const;
	const Vector3* getBulletPosition( int index ) const;

private:
	void updateFacing( const Target& target );               // 移動方向（静止時は対象）へ向き直る。
	void updateMove( const Vector3& forward, const Vector3& right ); // 通常の地上移動。
	void startDodge( const Vector3& forward, const Vector3& right ); // 回避開始。
	void startWireSwing( const Arena& arena, const Target& target ); // 正面へ1本発射して装着する。
	// 正面へワイヤーを飛ばし、最も手前のアンカー（敵球／地形）を探す。命中で true。
	bool castWire( const Arena& arena, const Target& target,
		const Vector3& origin, const Vector3& dir,
		Vector3* anchor, float* length ) const;
	// 重力＋空中制御＋引き寄せ＋遮蔽巻き付き＋バネ-ダンパで速度を作る。
	void updateWireSwing( const Vector3& right, const Arena& arena );
	void applyWireConstraint();                              // 位置更新後、絶対上限の球面で暴走を止める安全ガード。
	void releaseWire();                                      // 離脱（速度を維持したまま切る）。
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

	// 立体機動ワイヤー。
	bool    mWireSwinging;               // スイング中か（true の間だけ振り子物理を回す）。
	bool    mWireActive[ kWireCount ];   // その本が張られている（描画対象）か。
	Vector3 mAnchor[ kWireCount ];       // 各ワイヤーのアンカー座標。
	float   mWireLength[ kWireCount ];   // 各ワイヤーの現在長（巻取り／繰出しで変化）。
	Vector3 mWireDrawFrom[ kWireCount ]; // 残像表示中の手元側の座標（離脱時に固定する）。
	int     mWireCooldown;               // 再射出までのクールタイム。
	int     mWireVisibleCount;           // 離脱後にワイヤーを残して見せる残りフレーム。
	bool    mLandOnContact;              // 接地時に自動離脱するか（設定：着地／継続）。

	// 射撃。
	Array< Bullet > mBullets;
	int mFireCooldown;

	AutoPtr< GraphicsDatabase > mDatabase;
	AutoPtr< Model > mModel;
};

#endif
