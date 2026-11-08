#ifndef INCLUDED_ZOMBIE_ROOT_MOTION_H
#define INCLUDED_ZOMBIE_ROOT_MOTION_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Scene/Animation.h"
#include <vector>

class SkinnedMeshData;

// 1回だけ流すアニメ（攻撃・よろけ・倒れる）の、フレームごとの根の骨（腰）の水平の位置の表（ルートモーション）。
// 変換で根の動きを消していないので、これらのアニメは腰が前後へ動く（攻撃で 0.47m 踏み込む・倒れて 0.9〜1.2m 動く）。
// その動きを置き場所へ移すのに使う（移し方は Zombie。SummaryPlan/個別仕様/ZombieRootMotion_Plan.txt）。
// 上下（倒れて腰が下がる）と回転は表にしない（姿勢のまま見せる）。
// 表はメッシュとアニメが同じなら同じなので、種類ごと・アニメごとに1つだけ作る（ZombieModel が持つ）。作るときに全フレームの姿勢を計算する。
class ZombieRootMotion{
public:
	// data: 形と骨。所有権は受け取らない。nullptr 不可、isReady() 済み。姿勢を計算するのに構築中だけ使う（覚えない）。
	// animation: 表にするアニメ。空なら表を作らない（isValid が false）。
	// rootBone: 根の骨（SkinnedMeshData::findBone の番号）。-1 なら表を作らない。
	ZombieRootMotion( const SkinnedMeshData* data, GameLib::Scene::Animation animation, int rootBone );

	// 表があるか（無ければ、使う側が根の動きを移さない）。
	bool isValid() const;
	// アニメ時刻 time（フレーム）の根の水平の位置（骨格の根の空間＝置き場所と向きを掛ける前。m。y は 0）。
	// 時刻はアニメの長さの中に収める（1回だけ流すアニメなので折り返さない）。
	GameLib::Math::Vector3 offsetAt( float time ) const;
	// 根が最初のフレームから一番遠くへ動いた長さ（m。水平だけ）。確認用。
	float farthestDistance() const;

private:
	std::vector< GameLib::Math::Vector3 > mOffsets; // フレームごとの根の水平の位置。添え字はアニメ時刻（フレーム）
};

#endif
