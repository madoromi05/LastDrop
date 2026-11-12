#ifndef INCLUDED_WEAPON_BOUNDS_H
#define INCLUDED_WEAPON_BOUNDS_H

#include "GameSrc/Render/SkinnedMesh/BonePoseModifier.h"
#include "GameLib/Math/Vector3.h"

struct FirstPersonRig;

// 一人称の体の書き換えの段 5（最後）：置き終えた銃（反動で跳ねた後）の頂点の範囲を囲む箱を求めて覚える。骨は動かさない（測るだけ）
// 移動の当たり判定を銃の先まで伸ばすのに使う（PlayerComponent::collider）。最後に銃を動かした後に測るので、見えている銃と当たり判定がずれない
class WeaponBounds : public BonePoseModifier{
public:
	// rig: 骨の番号・持ち方・銃の大きさを読む。所有権は受け取らない。nullptr 不可。この段より長く生きていること
	explicit WeaponBounds( const FirstPersonRig* rig );

	// 覚えた箱を捨てる（持ち替えたとき。次の apply で、その銃の範囲を求め直す）
	void reset();
	// 最後に apply したポーズでの銃を囲む箱（骨格の根の空間）を low / high へ書く。銃が無い・まだ測っていなければ false
	bool bounds( GameLib::Math::Vector3* low, GameLib::Math::Vector3* high ) const;

	virtual void apply( std::vector< GameLib::Math::Matrix34 >* pose ) override;

private:
	const FirstPersonRig* mRig;
	bool mHasBounds;               // mLow / mHigh を一度でも求めたか
	GameLib::Math::Vector3 mLow;
	GameLib::Math::Vector3 mHigh;
};

#endif
