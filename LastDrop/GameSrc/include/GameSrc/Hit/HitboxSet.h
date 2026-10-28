#ifndef INCLUDED_HITBOX_SET_H
#define INCLUDED_HITBOX_SET_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"
#include "GameSrc/Hit/BodyPart.h"
#include <vector>

class HitboxLayout;
class SkinnedMeshInstance;

// 線分（弾の通り道）が部位に当たったときの結果。
struct HitResult{
	BodyPart part;                // 当たった部位
	float time;                   // 当たった時刻（0〜1。線分のうち何割進んだところか）
	GameLib::Math::Vector3 point; // 当たった位置（ワールド座標）
	GameLib::Math::Vector3 normal; // 当たった面の外向きの法線（ワールド・長さ1）。始点が箱の中なら、進む向きの逆
};

// 1体ぶんの、部位ごとの当たり判定（命中判定用）。部位ごとに骨に付いた箱を1つずつ持ち、アニメの通りに動く。
// 移動の当たり判定（体を真上から囲む長方形）とは別物。こちらは「弾がどの部位に当たったか」を答えるためのもの。
//
// どの骨にどんな箱を付けるか（設計図）は HitboxLayout を借りる。同じ種類のキャラは全員が同じ設計図を指す。
// ここが持つのは、1体ごとに違うもの（今のポーズで箱がワールドのどこにあるか）だけ。
class HitboxSet{
public:
	// layout: 箱の設計図。所有権は受け取らない。nullptr 不可。この当たり判定より長く生きていること。
	explicit HitboxSet( const HitboxLayout* layout );

	// 箱をどのポーズに合わせるか。
	enum PoseSource{
		// 見た目の書き換え（一人称で頭を潰す・のけぞり等）を掛ける前の、アニメどおりのポーズ
		// （体の書き換え＝ゾンビのフラつきは掛けた後。SkinnedMeshInstance::setBodyPoseModifier）
		POSE_ANIMATED,
		POSE_SHOWN,    // 書き換えた後の、見えているポーズ（アニメ無しで骨を大きく動かす間。体を引きずるゾンビ）
	};
	// 箱を今のポーズへ置き直す。mesh を update した後に、毎フレーム1回呼ぶ。ふだんはアニメどおりのポーズに合わせる。
	void update( const SkinnedMeshInstance& mesh, PoseSource source = POSE_ANIMATED );
	// part の箱を外す（部位欠損で部位が無くなったとき）。以後その箱には当たらず、置いていない箱として扱う（boxMatrix が false）。
	// 戻す入口は無い（無くなった部位は戻らない）。何回呼んでも安全。
	void removePart( BodyPart part );

	// origin から delta だけ進む線分が、一番早く当たる部位の結果を hit に書く。当たらなければ false。
	// hit: 出力先。nullptr 不可。
	bool raycast( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta, HitResult* hit ) const;

	// ── 当たり判定の表示用 ──
	int boxNumber() const;
	// -1〜1 の立方体をワールドの箱へ置く行列。まだ置いていない箱（update 前）なら false。
	bool boxMatrix( int index, GameLib::Math::Matrix34* out ) const;
	// 箱が受け持つ部位（体力の表示で、箱の所に部位の体力を出すのに使う）。
	BodyPart boxPart( int index ) const;
	// part の箱が置かれていれば、その中心（ワールド）を out へ書いて true。置いていない・外した部位なら false（out は変えない）。
	// part の箱が複数あれば、並びの先の物。ゾンビの頭の位置（つかまれた相手が顔を向ける先・噛む音の所）に使う。
	// out: nullptr 不可。
	bool partCenter( BodyPart part, GameLib::Math::Vector3* out ) const;

private:
	// 箱1つの置き場所。並びは HitboxLayout::boxes と同じ。
	struct Placement{
		GameLib::Math::Matrix34 world;    // -1〜1 の立方体 → ワールドの箱（update で求める）
		GameLib::Math::Matrix34 toLocal;  // world の逆。線分を箱の空間へ移すのに使う
		bool placed;                      // update で置けたか
		bool removed;                     // removePart で外したか（外した箱は置かない）
	};
	const HitboxLayout* mLayout; // 借りているだけ
	std::vector< Placement > mPlacements;
};

#endif
