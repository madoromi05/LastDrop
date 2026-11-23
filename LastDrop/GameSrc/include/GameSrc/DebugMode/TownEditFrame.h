#ifndef INCLUDED_TOWN_EDIT_FRAME_H
#define INCLUDED_TOWN_EDIT_FRAME_H

#include "GameSrc/Map/TownEdits.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"
#include <vector>

// F6 の Town タブで物を示す枠の箱と、飛んでいる自キャラの視点の正面の線から選ぶ物・足す所を求める計算
// 枠の大きさは種類ごとに決まった大きさ（物のモデルを囲む箱）。足元から上へ立て、物と同じ向きに回す（見た目と同じ PlacementMatrix）
// 状態は持たない。Debug・Profile ビルドだけ（Release では中身をビルドしない）
class TownEditFrame{
public:
	// 選ぶ候補 1 つ（種類と、今の置き場所）
	struct Target{
		TownEditKind kind;
		TownPlacement at;
	};
	static constexpr int kNoTarget = -1;	// pick で選べる物が無い

	static GameLib::Math::Matrix34 boxMatrix( TownEditKind kind, const TownPlacement& at );																		// kind の枠を at に置いた行列。-1〜1 の立方体を枠の箱へ写す（HitMath と同じ箱の持ち方）
	static GameLib::Math::Vector3 center( TownEditKind kind, const TownPlacement& at );																			// 枠の箱の真ん中
	static bool hit( const GameLib::Math::Vector3& eye, const GameLib::Math::Vector3& direction, TownEditKind kind, const TownPlacement& at, float* distance );	// eye から direction（長さ 1）へ伸ばした線が枠に入るまでの距離（m）を distance に書いて true。目が枠の中なら 0。distance: nullptr 不可
	static int pick( const GameLib::Math::Vector3& eye, const GameLib::Math::Vector3& direction, const std::vector< Target >& targets );						// 線が枠に当たる物のうち、一番近い物の番号（targets の添字）。どれにも当たらなければ、枠の真ん中が線に一番近い物。targets が空なら kNoTarget
	static GameLib::Math::Vector3 addPoint( const GameLib::Math::Vector3& eye, const GameLib::Math::Vector3& direction );										// 足す物の置き場所。線が地面（y 0 の面）に当たればそこ。当たらなければ（上・水平を向いている）目の 5m 前の、真下の地面
};

#endif
