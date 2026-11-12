#ifndef INCLUDED_ZOMBIE_CRAWL_LAYOUT_H
#define INCLUDED_ZOMBIE_CRAWL_LAYOUT_H

#include <vector>

class SkinnedMeshData;

// 体を引きずる動き（ZombieCrawl）の「設計図」。どの骨で腕・脚・胸・頭を曲げ、どの骨で体の向きを測るか
// 骨の番号はメッシュが同じなら同じなので、ゾンビの種類ごとに1つだけ作り（ZombieModel が持つ）、全員で共有する
// 1体ごとに変わるもの（腕を今どこまで伸ばしているか）は持たない（ZombieCrawl が持つ）
class ZombieCrawlLayout{
public:
	// 腕1本。上腕は肩で、前腕は肘で、手は手首で曲げる
	struct Arm{
		int shoulder;                         // 上腕の骨（原点が肩の関節）
		int elbow;                            // 前腕の骨（原点が肘の関節）
		int hand;                             // 手の骨（原点が手首）
		int knuckle;                          // 中指の付け根の骨（手首 → ここを手の向きとみなす）
		std::vector< int > upperFollowers;    // 肩で一緒に回す骨（上腕・前腕・手と指）
		std::vector< int > lowerFollowers;    // 肘で一緒に回す骨（前腕・手と指）
		std::vector< int > handFollowers;     // 手首で一緒に回す骨（手と指）
	};
	// 脚1本。太ももは脚の付け根で、すねは膝で曲げる
	struct Leg{
		int hip;                              // 太ももの骨（原点が脚の付け根）
		int knee;                             // すねの骨（原点が膝）
		int ankle;                            // 足の骨（原点が足首）
		std::vector< int > thighFollowers;    // 付け根で一緒に回す骨（太もも・すね・足）
		std::vector< int > shinFollowers;     // 膝で一緒に回す骨（すね・足）
	};
	enum Side{
		SIDE_LEFT,
		SIDE_RIGHT,
		SIDE_NUMBER,
	};

	// data: 形と骨。所有権は受け取らない。nullptr 不可、isReady() 済み。骨の番号を引くのに構築中だけ使う（覚えない）
	// boneNamePrefix: 骨の名前の頭の共通部分（"Zombie_"）。nullptr 不可
	// 骨が1本でも見つからなければ使えない（isValid が false。デバッグ出力で知らせる。腕は曲がらず、体は進まない）
	ZombieCrawlLayout( const SkinnedMeshData* data, const char* boneNamePrefix );

	bool isValid() const;
	const Arm& arm( Side side ) const;
	const Leg& leg( Side side ) const;
	int hips() const;   // 体の向き（腰 → 首）を測る根元の骨
	int spine() const;  // 胸を反らせる中心の骨（Spine1）
	int chest() const;  // 胸（引きずる体の中心。向きを変える中心・壁に当たる所）
	int neck() const;   // 体の向きを測る先の骨。頭を上げる中心
	int head() const;   // 首 → ここを頭の向きとみなす
	// あごの先の骨（頭 → ここで顔の向きを測る）。無ければ -1（顔を相手へ向けない。isValid には含めない）
	int chin() const;
	const std::vector< int >& chestFollowers() const; // 胸を反らせるときに一緒に回す骨（胸から上・両腕・首・頭）
	const std::vector< int >& headFollowers() const;  // 頭を上げるときに一緒に回す骨（首・頭・あご）

private:
	Arm mArms[ SIDE_NUMBER ];
	Leg mLegs[ SIDE_NUMBER ];
	int mHips;
	int mSpine;
	int mChest;
	int mNeck;
	int mHead;
	int mChin;
	std::vector< int > mChestFollowers;
	std::vector< int > mHeadFollowers;
	bool mValid;
};

#endif
