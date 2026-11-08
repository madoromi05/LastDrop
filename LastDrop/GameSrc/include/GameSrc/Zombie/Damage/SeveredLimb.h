#ifndef INCLUDED_SEVERED_LIMB_H
#define INCLUDED_SEVERED_LIMB_H

#include "GameSrc/Render/SkinnedMesh/SkinnedMeshInstance.h"
#include "GameSrc/Render/SkinnedMesh/BonePoseModifier.h"
#include "GameSrc/Zombie/Damage/ZombieSeverLayout.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"
#include <random>
#include <vector>

class SkinnedMeshData;
class IRayBlocker;
class DrawCulling;

// ゾンビから千切れた部位1つ（腕・脚・頭）。弾の向きへ回りながら飛び、地面に落ちて止まり、しばらく残ってから沈んで消える（isGone）。
// 部位ごとのメッシュは作らず、ゾンビと同じメッシュをもう1体ぶん描く。千切れた部位の骨だけを残し、
// ほかの骨を全部切り口の1点へ潰す（体の側の ZombieDismemberment と逆の潰し方）。
// 姿勢は切れた瞬間のものを固め、丸ごと動かす（剛体。関節は曲がらない）。当たり判定は無い（弾は抜け、踏めない。壁も抜ける）。
// 書き換えは自分で行う（BonePoseModifier を自分の中だけで使うので、外からは見せない）。
// 持ち主（SeveredLimbList）が unique_ptr で持つ（メッシュが自分を書き換えとして指すので、動かさない）。
class SeveredLimb : private BonePoseModifier{
public:
	// data: 形と見た目（千切れたゾンビと同じ物）。所有権は受け取らない。nullptr 不可、isReady() 済み。この部位より長く生きていること。
	// cut: 切れた部位の決まり（残す骨・切り口の関節）。中身は写す（覚えない）。
	// worldPose: 切れた瞬間の骨の姿勢（ワールド。Zombie::shownWorldPose）。boneNumber() 個ぶん。写す。
	// direction: 弾の進む向き（長さ1）。この向きへ飛ぶ。
	// random: 回る軸と速さのばらつき。所有権は受け取らない。nullptr 不可。
	SeveredLimb( const SkinnedMeshData* data, const ZombieSeverLayout::Cut& cut, const std::vector< GameLib::Math::Matrix34 >& worldPose,
		const GameLib::Math::Vector3& direction, std::mt19937* random );
	SeveredLimb( const SeveredLimb& ) = delete;
	SeveredLimb& operator=( const SeveredLimb& ) = delete;

	// 1ティック進める。ground: 落ちる先の地面（下向きの線で高さを聞く）。nullptr なら高さ 0 の平らな地面とみなす。
	void update( const IRayBlocker* ground );
	// culling で描くと決まった時だけ描く（小さい物として間引く。OptimizationA_Plan.txt 6 章）。
	void draw( const DrawCulling& culling ) const;
	// 沈み終わって、消してよいか。
	bool isGone() const;

private:
	enum class State{
		Flying,  // 飛んでいる
		Resting, // 地面で止まっている
		Sinking, // 地面へ沈んでいる
	};

	void fly( const IRayBlocker* ground ); // 重力で落とし、回し、地面に着いたら止める
	// 今の動き（回した角度・中心の位置）を、固めた姿勢に掛ける行列へまとめる。
	void updateMotion( float sinkOffset );
	// 固めた姿勢を今の動きの分だけ動かす（BonePoseModifier）。
	virtual void apply( std::vector< GameLib::Math::Matrix34 >* pose ) override;

	SkinnedMeshInstance mMesh;
	std::vector< GameLib::Math::Matrix34 > mBasePose; // 切れた瞬間の姿勢（ワールド）。残さない骨は切り口へ潰してある
	GameLib::Math::Vector3 mStartCenter; // 切れた瞬間の部位の中心（回す中心。ワールド）
	GameLib::Math::Vector3 mCenter;      // 今の部位の中心（ワールド。沈む分は含まない）
	float mReach;                        // 中心から一番遠い骨までの距離（m。回しても変わらないので、描くときの箱の半分の幅に使う）
	GameLib::Math::Vector3 mVelocity;    // 中心の速さ（m/ティック）
	GameLib::Math::Vector3 mSpinAxis;    // 回る軸（長さ1）
	float mSpinPerTick;                  // 回る速さ（度/ティック）
	float mSpinAngle;                    // ここまでに回った角度（度）
	GameLib::Math::Matrix34 mMotion;     // 固めた姿勢 → 今の姿勢（apply で掛ける）
	State mState;
	int mStateTicks;                     // 今の状態に入ってからのティック数（Resting・Sinking で使う）
};

#endif
