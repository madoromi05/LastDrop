#ifndef INCLUDED_HITBOX_SET_H
#define INCLUDED_HITBOX_SET_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"
#include "BodyPart.h"
#include <vector>

class SkinnedMeshData;
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
// 箱は、その部位の骨に一番強く付いている頂点を、部位の形に沿った向きで囲んで作る（BoxFitter）。
// 長い辺は関節を結ぶ向き（肩→肘など）、残り2辺は断面の広がりに合わせる。
// モデルを差し替えても合わせ直さずに済む。骨の名前は「頭の共通部分（"mixamorig:" など）＋部位の骨の名前」で引くので、
// 骨の並びが Mixamo と同じキャラなら、頭の共通部分を変えるだけで使える。
class HitboxSet{
public:
	// data: 形と骨。所有権は受け取らない。nullptr 不可、isReady() 済み。箱の大きさを求めるのに構築中だけ使う。
	// boneNamePrefix: 骨の名前の頭の共通部分（"mixamorig:" / "Zombie_"）。nullptr 不可。
	// 骨が見つからない・頂点が無い部位は箱を作らない（デバッグ出力で知らせる）。
	HitboxSet( const SkinnedMeshData* data, const char* boneNamePrefix );

	// 箱を今のポーズへ置き直す。mesh を update した後に、毎フレーム1回呼ぶ。
	// 見た目の書き換え（一人称で頭を潰すなど）を掛ける前の、アニメどおりのポーズに合わせる。
	void update( const SkinnedMeshInstance& mesh );

	// origin から delta だけ進む線分が、一番早く当たる部位の結果を hit に書く。当たらなければ false。
	// hit: 出力先。nullptr 不可。
	bool raycast( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta, HitResult* hit ) const;

	// ── 当たり判定の表示用 ──
	int boxNumber() const;
	// -1〜1 の立方体をワールドの箱へ置く行列。まだ置いていない箱（update 前）なら false。
	bool boxMatrix( int index, GameLib::Math::Matrix34* out ) const;

private:
	struct Box{
		BodyPart part;
		int anchorBone;                   // 箱が付いている骨
		GameLib::Math::Matrix34 local;    // -1〜1 の立方体 → 骨の空間の箱
		GameLib::Math::Matrix34 world;    // -1〜1 の立方体 → ワールドの箱（update で求める）
		GameLib::Math::Matrix34 toLocal;  // world の逆。線分を箱の空間へ移すのに使う
		bool placed;                      // update で置けたか
	};
	std::vector< Box > mBoxes;
};

#endif
