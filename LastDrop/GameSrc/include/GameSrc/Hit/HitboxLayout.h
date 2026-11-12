#ifndef INCLUDED_HITBOX_LAYOUT_H
#define INCLUDED_HITBOX_LAYOUT_H

#include "GameLib/Math/Matrix34.h"
#include "GameSrc/Hit/BodyPart.h"
#include <vector>

class SkinnedMeshData;

// 部位ごとの当たり判定（命中判定用）の「設計図」。どの部位の箱を、どの骨に、どんな大きさ・向きで付けるか
// メッシュが同じなら結果も同じなので、キャラの種類（SkinnedMeshData）ごとに1つだけ作り、同じ種類の全員で共有する
// 1体ごとに変わるもの（今のポーズで箱がワールドのどこにあるか）は持たない（HitboxSet が持つ）
// 作るときに全頂点を部位の数だけ調べるので、1体ごとに作ると、体を出すたびにその計算をやり直すことになる
//
// 箱は、その部位の骨に一番強く付いている頂点を、部位の形に沿った向きで囲んで作る（BoxFitter）
// 長い辺は関節を結ぶ向き（肩→肘など）、残り2辺は断面の広がりに合わせる
// モデルを差し替えても合わせ直さずに済む。骨の名前は「頭の共通部分（"mixamorig:" など）＋部位の骨の名前」で引くので、
// 骨の並びが Mixamo と同じキャラなら、頭の共通部分を変えるだけで使える
class HitboxLayout{
public:
	// 骨に付いた箱1つ
	struct Box{
		BodyPart part;
		int anchorBone;                 // 箱が付いている骨（SkinnedMeshData::findBone の番号）
		GameLib::Math::Matrix34 local;  // -1〜1 の立方体 → 骨の空間の箱
	};

	// data: 形と骨。所有権は受け取らない。nullptr 不可、isReady() 済み。箱の大きさを求めるのに構築中だけ使う（覚えない）
	// boneNamePrefix: 骨の名前の頭の共通部分（"mixamorig:" / "Zombie_"）。nullptr 不可
	// 骨が見つからない・頂点が無い部位は箱を作らない（デバッグ出力で知らせる）
	HitboxLayout( const SkinnedMeshData* data, const char* boneNamePrefix );

	const std::vector< Box >& boxes() const;

private:
	std::vector< Box > mBoxes;
};

#endif
