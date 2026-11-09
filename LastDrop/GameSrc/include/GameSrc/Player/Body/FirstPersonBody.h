#ifndef INCLUDED_FIRST_PERSON_BODY_H
#define INCLUDED_FIRST_PERSON_BODY_H

#include "GameSrc/Render/SkinnedMesh/BonePoseModifier.h"
#include "GameSrc/Render/SkinnedMesh/BonePoseModifierChain.h"
#include "GameSrc/Player/Body/Pose/FirstPersonRig.h"
#include "GameSrc/Player/Body/Pose/ArmOverridePose.h"
#include "GameSrc/Player/Body/Pose/SpineBendPose.h"
#include "GameSrc/Player/Body/Pose/EyePoint.h"
#include "GameSrc/Player/Body/Pose/WeaponPlacePose.h"
#include "GameSrc/Player/Body/Pose/WeaponFeelPose.h"
#include "GameSrc/Player/Body/Pose/WeaponBounds.h"
#include "GameLib/Math/Vector3.h"
#include <vector>

class SkinnedMeshData;

// 一人称（Escape from Tarkov のような「自分の体が見える」FPS）用に、全身のポーズを書き換える。
// 書き換えは小さな段（Player/Body/Pose/ の BonePoseModifier）に分けてあり、BonePoseModifierChain でこの順につなぐ
// （SummaryPlan/個別仕様/設計と分割/HighPrioritySplit_Plan.txt 5 章。2026-11-09 に 1 つのクラスから分けた）：
//   0. ArmOverridePose … リロード中・拳銃の構えで、両腕だけを別のアニメのポーズに差し替える（胸に付けて写す）
//   1. SpineBendPose   … 視点の上下（ピッチ）ぶん背骨 3 本を曲げる（腕と銃が視線に付いてくる）
//   2. EyePoint        … 曲げた後の頭から目の位置（カメラの視点）を決める（測るだけ）
//   3. WeaponPlacePose … 両手持ち → 走るなら胸元へ抱える → 覗くならサイトを目へ（銃を置く）
//   4. WeaponFeelPose  … 照準のぶれ → 撃った反動（置いた銃を手触りのぶん動かす）
//   5. WeaponBounds    … 跳ねた後の銃を囲む箱（移動の当たり判定に使う。測るだけ）
// 段がみんなで読む物（骨の番号・持っている銃・ピッチ・目の位置）は FirstPersonRig。ここはそれを持ち、外からの値を段へ配る窓口。
// 段の中身の説明（なぜその順か・なぜその骨か）は、それぞれの段のヘッダ。
//
// 座標はすべて骨格の根の空間（前方 +Z・上 +Y・単位 m）。キャラの置き場所・向きは知らない。
// ボーンは Mixamo の名前で探す。見つからない骨は飛ばす（その部分が動かないだけで落ちない）。
class FirstPersonBody : public BonePoseModifier{
public:
	// data: 骨の番号を引くためだけに使う（覚えない）。nullptr 不可、isReady() 済みであること。
	explicit FirstPersonBody( const SkinnedMeshData* data );
	// 段が mRig と互いを指すので、写さない。
	FirstPersonBody( const FirstPersonBody& ) = delete;
	FirstPersonBody& operator=( const FirstPersonBody& ) = delete;

	// 視点の上下（度。正で見下ろす。FirstPersonCamera::pitch と同じ約束）。
	void setPitch( float degree );
	// 手に持つ銃。覗くときにサイト線を目へ合わせるのに使う。呼ばなければ覗いても腕は動かない。
	// handBone: 銃を付けた骨。grip: 銃のモデル座標 → 骨の空間（HeldItem に渡したのと同じ持ち方）。
	// sightEye: 覗いたときに目を置く点（銃のモデル座標）。サイト線（リアサイト→フロントサイト）を後ろへ延ばした所。
	//           サイト線は銃のモデルの +Z（銃口の向き）と平行であること。
	// supportHandBone: 銃を支える手（左手）の骨。-1 なら右手首を曲げない。
	// supportHandOnWeapon: 支える手の骨が来るべき点（銃のモデル座標）。持ち方を合わせたアニメから求めたもの。
	// weaponMin / weaponMax: 銃の頂点の範囲（銃のモデル座標。StaticMeshData::boundsMin / boundsMax）。weaponBounds で使う。
	void setWeapon( int handBone, const GameLib::Math::Matrix34& grip, const GameLib::Math::Vector3& sightEye,
		int supportHandBone, const GameLib::Math::Vector3& supportHandOnWeapon,
		const GameLib::Math::Vector3& weaponMin, const GameLib::Math::Vector3& weaponMax );
	// 銃を持たない（素手）に戻す。銃に合わせる補正（両手持ち・覗く・反動・ぶれ・胸元への寄せ）が効かなくなる。
	void clearWeapon();
	// 両手持ちの効き具合（0 = 右手首を曲げない、1 = 銃の先が左手を向く）。武器をしまっている間は 0 にする。
	void setHoldWeight( float weight );
	// 覗き具合（0 = アニメのまま、1 = サイト線が目と視線にぴったり重なる）。
	void setAimWeight( float weight );
	// 胸元への寄せ具合（0 = アニメのまま、1 = 銃を胸元に抱える）。走っている間だけ 1 にする。
	void setCarryWeight( float weight );
	// 反動の量（0 = 反動なし、1 = 撃った瞬間。WeaponRecoil::kick）。
	void setRecoil( float kick );
	// 照準のぶれ（度。FirstPersonCamera::setWeaponSway と同じ角度・同じ約束。ヨーは正で左、ピッチは正で見下ろす）。
	void setSway( float yawDegree, float pitchDegree );
	// 腕を差し替えるポーズ（AnimationPoseSampler::sample の結果）と効き具合（0〜1）。pose が nullptr か weight が 0 なら差し替えない。
	// pose は借りるだけ（次の apply まで中身を変えないこと）。腕がアニメで銃から手を離すので、両手持ちの補正（setHoldWeight）は呼び出し側が弱める。
	// followReference：pose を作ったときの体のアニメの右手（銃の手。骨格の根の空間）。nullptr なら寄せない。
	// followHandWeight：差し替えた腕を、今の体のアニメの右手が followReference からどれだけ動いたか（位置と向き）のとおりに、
	// まとめて動かす割合（0〜1。0 なら胸に付けて写すだけ）。拳銃の構え（pistol_ready）はライフルの止まった構え（rifle_idle）の体の上で作ってあるので、
	// 1 にすると、歩き・走りでも拳銃はライフルの右手と同じに動き、同じ所・同じ向きで持つ（PistolAnimation_Plan.txt 14-7）。
	// followReference は借りるだけ（次の apply まで中身を変えないこと）。
	void setArmOverride( const std::vector< GameLib::Math::Matrix34 >* pose, float weight, float followHandWeight,
		const GameLib::Math::Matrix34* followReference );

	// 最後に apply したポーズでの目の位置（骨格の根の空間）。apply 前・頭の骨が無いときは足元から一定の高さ。
	const GameLib::Math::Vector3& eyePosition() const;
	// 最後に apply したポーズ（反動で跳ねた後）での銃の頂点の範囲を囲む箱（骨格の根の空間）を low / high へ書く。
	// 銃の8隅を移してから囲むので、銃が斜めを向いていても銃全体が入る。銃が無い・まだ apply していなければ false。
	// 武器をしまっているかは知らない（銃の箱を使うかは持ち主が決める）。
	bool weaponBounds( GameLib::Math::Vector3* low, GameLib::Math::Vector3* high ) const;

	virtual void apply( std::vector< GameLib::Math::Matrix34 >* pose ) override;

private:
	FirstPersonRig mRig;            // 段がみんなで読む物（段より先に宣言する）
	ArmOverridePose mArmOverride;
	SpineBendPose mSpineBend;
	EyePoint mEyePoint;
	WeaponPlacePose mWeaponPlace;
	WeaponFeelPose mWeaponFeel;
	WeaponBounds mWeaponBounds;
	BonePoseModifierChain mChain;     // 上の段をこの順につないだもの（apply はこれを掛けるだけ）
};

#endif
