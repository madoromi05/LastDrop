#ifndef INCLUDED_ZOMBIE_CRAWL_BODY_H
#define INCLUDED_ZOMBIE_CRAWL_BODY_H

#include "GameLib/Math/Vector3.h"

class SkinnedMeshInstance;
class ZombieCrawlLayout;
class CharacterMover;

// 脚を失って倒れた体を、胸を中心に動かすための形（ゾンビ 1 体ぶん。SummaryPlan/個別仕様/ゾンビ/ZombieCrawl_Plan.txt）
// 倒れ終わった姿勢で一度だけ「足元から胸まで」と「見た目の向きから進む向き（腰 → 首）までのずれ」を測り、
// その後は胸の位置と進む向きから、見た目の向き・足元を決め直す（胸を中心に向きを変えるので、向きを変えると足元が回る）
// ZombieCrawl（腕の骨を曲げる姿勢の書き換え）とは別物で、こちらは置き場所の計算だけ
// いつ引きずり始めるか・どう追うか・腕が残っているかは持ち主（Zombie）が決める
class ZombieCrawlBody{
public:
	ZombieCrawlBody();

	// 倒れ終わった姿勢（mesh の今の骨）から、腰 → 首を進む向き、胸を体の中心として測る。見た目は動かさない
	// footing: 今の足元。angleY: 今の見た目の向き（度）。進む向きへ書き換える。nullptr 不可
	// 骨がまだ引けない（update 前）なら false を返し、何も変えない
	bool start( const SkinnedMeshInstance& mesh, const ZombieCrawlLayout& layout, const GameLib::Math::Vector3& footing, float* angleY );
	// 胸を step だけ動かした（壁・車に沿って滑った後の量）
	void moveCenter( const GameLib::Math::Vector3& step );
	// 胸の位置と進む向き angleY から、見た目の向きを mesh へ、足元を footing へ書き、mesh を足元へ置く（胸の高さは地面に合わせる）
	// mesh・footing: nullptr 不可
	void place( float angleY, const CharacterMover& mover, SkinnedMeshInstance* mesh, GameLib::Math::Vector3* footing );
	// 胸の位置（ワールド）。追う・回り込む・壁に当たるのはこの位置。start の前は意味が無い
	const GameLib::Math::Vector3& center() const;

private:
	GameLib::Math::Vector3 mCenter;      // 胸の位置（ワールド。動かすのはこちらで、足元はここから決める）
	GameLib::Math::Vector3 mChestOffset; // 足元から胸まで（骨格の根の空間。水平だけ。倒れ終わった姿勢で決める）
	float mYawOffset;                    // 見た目の向きから進む向き（腰 → 首）までの角度（度）
};

#endif
