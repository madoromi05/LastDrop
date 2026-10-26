#ifndef INCLUDED_PLACEMENT_MATRIX_H
#define INCLUDED_PLACEMENT_MATRIX_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"

// 物を「どこに・どの向きで・どの大きさで」置くかの行列を作る・読む、ただ1か所。状態は持たない。
// 見た目（StaticMeshInstance / SkinnedMeshInstance）・移動の当たり判定（BodyFootprint）・当たり判定の表示が
// 同じ式を使うので、ここを通せば見た目と当たり判定がずれない（掛ける順を変えるときも、直すのはここだけ）。
class PlacementMatrix{
public:
	// 移動 * Y軸まわりの回転（回転→移動の順に効く）。position: 置き場所（ワールド座標）。angleY: 向き（度）。
	static GameLib::Math::Matrix34 make( const GameLib::Math::Vector3& position, float angleY );
	// 移動 * Y軸まわりの回転 * 拡大（拡大→回転→移動の順に効く）。scale: 全体の拡大率（1 で等倍）。
	static GameLib::Math::Matrix34 make( const GameLib::Math::Vector3& position, float angleY, float scale );
	// 行列の移動の部分（m03・m13・m23。置いた位置。骨の行列なら関節の位置）。GameLib の Matrix34 には読む関数が無いのでここに置く。
	static GameLib::Math::Vector3 translationOf( const GameLib::Math::Matrix34& m );
};

#endif
