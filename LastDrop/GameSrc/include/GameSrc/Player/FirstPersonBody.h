#ifndef INCLUDED_FIRST_PERSON_BODY_H
#define INCLUDED_FIRST_PERSON_BODY_H

#include "GameSrc/Render/SkinnedMesh/BonePoseModifier.h"
#include "GameLib/Math/Vector3.h"
#include <vector>

class SkinnedMeshData;

// 一人称（Escape from Tarkov のような「自分の体が見える」FPS）用に、全身のポーズを書き換える。
// 0. リロード中は、両腕（肩から先）だけを別のアニメ（リロード）のポーズに差し替える。胸の骨から見た位置・向きで写すので、
//    脚と背骨は今のアニメ（歩き・静止）のまま、腕だけ胸に付いて動く。差し替えた後の腕も、下の 1〜 の補正をそのまま受ける。
// 1. 視点の上下（ピッチ）ぶん背骨を曲げる。腕と銃が視線に付いてきて、見上げても見下ろしても画面に残る。
//    背骨3本（Spine / Spine1 / Spine2）に分けて曲げるのは、1本で曲げると腰から折れたように見えるため。
// 2. 右手首を曲げて、銃の先を左手（支える手）へ向ける。銃は右手の骨に付けているので、そのままだと
//    アニメによっては左手が銃から離れる（走りで約8cm）。Mixamo のアニメは両手の間に銃がある前提なので、
//    銃を両手の間に置き直すのがアニメの意図に最も近い（走りでは左手が腕の長さより遠く、左腕の IK では届かない）。
// 3. 走るときは、銃を胸元へ引き寄せ、銃口を下・体の内側へ向ける（Tarkov の走りのような抱え方）。
//    持っている走りのアニメは肩の高さで前に構えたままなので、覗くときと同じく両腕をまとめて動かす。
// 4. 覗く（ADS）ときは、銃のサイト線が目と視線に重なるよう、両腕（肩から先）をまとめて動かす（手続き的 ADS）。
//    覗く専用のアニメが無くても、持っているアニメのまま覗ける。両手ごと動かすので手は銃から離れない。
// 4.5 照準のぶれ（AimSway）のぶん、目の少し後ろ（肩の付け根あたり）を中心に両腕と銃を回す。目とカメラは動かさないので、
//    覗いているとサイトが画面の中で少し動いて見える。弾の向き（FirstPersonCamera::weaponDirection）も同じ角度だけぶれている。
// 5. 撃った反動のぶん、銃を後ろへ引いて銃口を跳ね上げる。両腕をまとめて動かすので手は銃から離れない。
//    目は動かさない（照準と弾の向きは変わらず、銃の見た目だけが跳ねる）。
// 6. 頭を消す（頭の骨を縮尺0にして首の付け根へ潰す）。カメラが頭の中にあるので、顔の裏側が映るのを防ぐ。
// 書き換えのついでに目の位置（カメラの視点に使う）と、銃を囲む範囲（移動の当たり判定に使う）を求めて覚える。
//
// 座標はすべて骨格の根の空間（前方 +Z・上 +Y・単位 m）。キャラの置き場所・向きは知らない。
// ボーンは Mixamo の名前で探す。見つからない骨は飛ばす（その部分が動かない・消えないだけで落ちない）。
class FirstPersonBody : public BonePoseModifier{
public:
	// data: 骨の番号を引くためだけに使う（覚えない）。nullptr 不可、isReady() 済みであること。
	explicit FirstPersonBody( const SkinnedMeshData* data );

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
	void setArmOverride( const std::vector< GameLib::Math::Matrix34 >* pose, float weight );

	// 最後に apply したポーズでの目の位置（骨格の根の空間）。apply 前・頭の骨が無いときは足元から一定の高さ。
	const GameLib::Math::Vector3& eyePosition() const;
	// 最後に apply したポーズ（反動で跳ねた後）での銃の頂点の範囲を囲む箱（骨格の根の空間）を low / high へ書く。
	// 銃の8隅を移してから囲むので、銃が斜めを向いていても銃全体が入る。銃が無い・まだ apply していなければ false。
	// 武器をしまっているかは知らない（銃の箱を使うかは持ち主が決める）。
	bool weaponBounds( GameLib::Math::Vector3* low, GameLib::Math::Vector3* high ) const;

	virtual void apply( std::vector< GameLib::Math::Matrix34 >* pose ) override;

private:
	// 背骨1本ぶん。この関節を中心に、自分と先にある骨（上半身）をまとめて回す。
	struct SpineJoint{
		int bone;
		std::vector< int > followers; // 一緒に回す骨（自分を含む）
	};
	void bendSpine( std::vector< GameLib::Math::Matrix34 >* pose ) const;
	// 右手首を中心に右手（と指）を回し、銃の先を左手へ向ける。
	void holdWithBothHands( std::vector< GameLib::Math::Matrix34 >* pose ) const;
	// 胸元へ抱える補正を、寄せ具合の分だけ両腕へ掛ける。
	void carryAtChest( std::vector< GameLib::Math::Matrix34 >* pose ) const;
	// 背骨を曲げた後の頭の位置から、目の位置を求めて mEyePosition へ覚える。
	void updateEyePosition( const std::vector< GameLib::Math::Matrix34 >& pose );
	// サイト線を目と視線へ重ねる補正を、覗き具合の分だけ両腕へ掛ける。
	void alignSightToEye( std::vector< GameLib::Math::Matrix34 >* pose ) const;
	// 照準のぶれの角度だけ、目の少し後ろを中心に両腕を回す。
	void swayWeapon( std::vector< GameLib::Math::Matrix34 >* pose ) const;
	// 反動の量の分だけ、銃を後ろへ引いて銃口を跳ね上げる補正を両腕へ掛ける。
	void kickWeapon( std::vector< GameLib::Math::Matrix34 >* pose ) const;
	// 両腕を、差し替えるポーズの「胸から見た腕」へ効き具合の分だけ寄せる（背骨を曲げる前に掛ける）。
	void overrideArms( std::vector< GameLib::Math::Matrix34 >* pose ) const;
	// 置き終えた銃の8隅から、銃を囲む箱を求めて mWeaponLow / mWeaponHigh へ覚える。
	void updateWeaponBounds( const std::vector< GameLib::Math::Matrix34 >& pose );
	void hideHead( std::vector< GameLib::Math::Matrix34 >* pose ) const;

	std::vector< SpineJoint > mSpine; // 根に近い順
	std::vector< int > mArmBones;     // 両肩から先（腕・手・指）
	std::vector< int > mRightHandBones; // 右手と右手の指（手首を曲げるときに一緒に回す）
	std::vector< int > mHiddenBones;
	int mHeadBone;
	int mChestBone;                   // 走るときに銃を寄せる基準の骨・腕を差し替えるときの基準の骨。-1 ならどちらもしない
	float mPitch;

	int mHandBone;                    // 銃を付けた骨。-1 なら銃なし
	GameLib::Math::Matrix34 mGrip;
	GameLib::Math::Vector3 mSightEye;
	int mSupportHandBone;             // 銃を支える手。-1 なら両手持ちの補正をしない
	GameLib::Math::Vector3 mSupportHandOnWeapon;
	GameLib::Math::Vector3 mWeaponMin;    // 銃の頂点の範囲（銃のモデル座標）
	GameLib::Math::Vector3 mWeaponMax;
	float mHoldWeight;
	float mAimWeight;
	float mCarryWeight;
	float mRecoil;
	float mSwayYaw;   // 照準のぶれ（度）
	float mSwayPitch;
	const std::vector< GameLib::Math::Matrix34 >* mArmOverride; // 借りているだけ。nullptr なら差し替えない
	float mArmOverrideWeight;

	GameLib::Math::Vector3 mEyePosition;
	bool mHasWeaponBounds;                // mWeaponLow / mWeaponHigh を一度でも求めたか
	GameLib::Math::Vector3 mWeaponLow;    // 最後に apply したポーズでの銃を囲む箱（骨格の根の空間）
	GameLib::Math::Vector3 mWeaponHigh;
};

#endif
