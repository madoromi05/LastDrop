#ifndef INCLUDED_POSE_MATH_H
#define INCLUDED_POSE_MATH_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"
#include <vector>

// 一人称の体の書き換えの段（Player/Body/Pose/）が共通に使う、骨の並びを動かす計算
// 座標はすべて骨格の根の空間
class PoseMath{
public:
	// 骨の並びへまとめて、左から変換を掛ける
	static void transformBones( const GameLib::Math::Matrix34& transform, const std::vector< int >& bones,
		std::vector< GameLib::Math::Matrix34 >* pose );
	// 変換を効き具合の分だけ骨へ掛ける（回転は球面補間なので、途中で腕が縮んだりしない）
	static void transformBonesPartially( const GameLib::Math::Matrix34& transform, float weight, const std::vector< int >& bones,
		std::vector< GameLib::Math::Matrix34 >* pose );
	// 銃の置き場所 gun（手の骨 * 持ち方）を、向きが axes（列が 銃の +X / +Y / +Z(銃口) の行き先）になり、
	// 銃に付いた点 anchor が goal へ来るよう動かす変換を求める。両腕へ掛ければ、手が銃から離れずに銃ごと動く
	static GameLib::Math::Matrix34 alignWeapon( const GameLib::Math::Matrix34& gun, const GameLib::Math::Matrix34& axes,
		const GameLib::Math::Vector3& anchor, const GameLib::Math::Vector3& goal );
	static void weaponAxes( const GameLib::Math::Matrix34& gun, GameLib::Math::Vector3* barrel, GameLib::Math::Vector3* side );	// 銃の置き場所 gun の、銃口の向き（モデルの +Z）と横の向き（+X）を長さ1にして求める（持ち方の縮尺を取り除く）
};

#endif
