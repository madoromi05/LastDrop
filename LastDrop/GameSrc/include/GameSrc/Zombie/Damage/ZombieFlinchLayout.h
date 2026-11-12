#ifndef INCLUDED_ZOMBIE_FLINCH_LAYOUT_H
#define INCLUDED_ZOMBIE_FLINCH_LAYOUT_H

#include "GameSrc/Hit/BodyPart.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"
#include <vector>

class SkinnedMeshData;

// のけぞり（ZombieFlinch）の「設計図」。撃たれた部位ごとに、どの関節で・どの骨をまとめて・どれだけ曲げるか
// 骨の番号はメッシュが同じなら同じなので、ゾンビの種類ごとに1つだけ作り（ZombieModel が持つ）、全員で共有する
// 1体ごとに変わるもの（今どれだけ押されているか）は持たない（ZombieFlinch が持つ）
// 設計図どおりに曲げる計算（bend）もここに置く。のけぞりと、胴を左右に揺らすフラつき（ZombieSway）の両方が使う
// 部位は「曲げるまとまり（頭・胴・左右の腕・左右の脚）」へまとめる。上腕と前腕は同じ腕の肩で曲げる（腕ごと押される）
class ZombieFlinchLayout{
public:
	// 関節1つ。この骨の原点（関節）を中心に、followers をまとめて回す
	struct Joint{
		int bone;                     // 回す中心の骨
		int tipBone;                  // 曲がる先の骨（関節 → この骨の向きを「今の骨の向き」とみなす）
		float share;                  // まとまりの角度のうち、この関節が受け持つ割合（まとまりの中で足すと 1）
		std::vector< int > followers; // 一緒に回す骨（自分を含む）
	};
	// 曲げるまとまり1つ（頭・胴・腕・脚）。関節は根に近い順
	struct Group{
		std::vector< Joint > joints;
		float kickDegree; // 1 発で押す角度（度）
	};

	// data: 形と骨。所有権は受け取らない。nullptr 不可、isReady() 済み。骨の番号を引くのに構築中だけ使う（覚えない）
	// boneNamePrefix: 骨の名前の頭の共通部分（"Zombie_"）。nullptr 不可
	// 骨が見つからない関節は作らない（デバッグ出力で知らせる。そのまとまりは曲がらないだけ）
	ZombieFlinchLayout( const SkinnedMeshData* data, const char* boneNamePrefix );

	// まとまりの一覧。根に近いまとまり（胴）から曲げる順に並ぶ（胴を曲げた後の首・肩の位置で、頭・腕を曲げるため）
	const std::vector< Group >& groups() const;
	// part の当たったときに曲げるまとまりの番号（groups の添え字）
	int groupOf( BodyPart part ) const;
	// group のまとまりを、根の側の関節から順に、direction（長さ 1。骨格の根の空間）へ degree 度だけ倒す
	// 骨の向きが direction に近いほど少なく倒す（平行なら倒れない）。pose: 骨格の根から見た骨の姿勢。nullptr 不可
	void bend( int group, const GameLib::Math::Vector3& direction, float degree, std::vector< GameLib::Math::Matrix34 >* pose ) const;

private:
	std::vector< Group > mGroups;
	int mPartGroups[ static_cast< int >( BodyPart::Count ) ]; // BodyPart の番号 → まとまりの番号
};

#endif
