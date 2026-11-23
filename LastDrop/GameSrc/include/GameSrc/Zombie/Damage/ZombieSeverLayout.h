#ifndef INCLUDED_ZOMBIE_SEVER_LAYOUT_H
#define INCLUDED_ZOMBIE_SEVER_LAYOUT_H

#include "GameSrc/Hit/BodyPart.h"
#include <vector>

class SkinnedMeshData;

// 部位欠損の「設計図」。体力が 0 になった部位を、どの関節で切り、どの骨と部位が一緒に無くなるか
// 骨の番号はメッシュが同じなら同じなので、ゾンビの種類ごとに1つだけ作り（ZombieModel が持つ）、全員で共有する
// 1体ごとに変わるもの（どこを失ったか）は持たない（ZombieDismemberment が持つ）
// 表は .cpp だけが知っている。胴は切れない
class ZombieSeverLayout{
public:
	// 1つの部位を切るときの決まり
	struct Cut{
		int jointBone;               // 切る関節の骨（この骨の原点が切り口）。-1 なら、この部位は切れない
		int stumpBone;               // 切り口の手前（体に残る側）の骨。-1 なら切り口を 1 点へ潰す。ほかは、この骨から関節への向きを軸に切り口を丸める
		std::vector< int > bones;    // 無くなる骨（関節の骨を含む）
		std::vector< BodyPart > parts; // 一緒に無くなる部位（自分を含む。上腕なら前腕も）
		bool losesFooting;           // 失うと立っていられないか（脚）
	};

	// data: 形と骨。所有権は受け取らない。nullptr 不可、isReady() 済み。骨の番号を引くのに構築中だけ使う（覚えない）
	// boneNamePrefix: 骨の名前の頭の共通部分（"Zombie_"）。nullptr 不可
	// 関節の骨が見つからない部位は切れない部位にする（デバッグ出力で知らせる）
	ZombieSeverLayout( const SkinnedMeshData* data, const char* boneNamePrefix );

	bool canSever( BodyPart part ) const;
	const Cut& cut( BodyPart part ) const;

private:
	Cut mCuts[ static_cast< int >( BodyPart::Count ) ]; // BodyPart の番号で引く
};

#endif
