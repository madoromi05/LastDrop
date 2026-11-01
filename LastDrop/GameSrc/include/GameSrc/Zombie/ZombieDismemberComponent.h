#ifndef INCLUDED_ZOMBIE_DISMEMBER_COMPONENT_H
#define INCLUDED_ZOMBIE_DISMEMBER_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Zombie/ZombieDismemberment.h"
#include "GameSrc/Zombie/ZombieCrawler.h"

class ZombieModel;

// ゾンビの部位欠損（頭・腕・脚が 0 になると見た目から消す ZombieDismemberment）と、脚を失って倒れた後に
// 腕で体を引きずる体（ZombieCrawler。腕の骨・胸の置き場所・腕の速さの段階。SummaryPlan/個別仕様/ZombieCrawl_Plan.txt）。
// 引きずる腕は残っている腕しか使えないので、2 つを同じ部品に置く（ZombieCrawler が ZombieDismemberment を借りる）。
// いつ切る・いつ引きずり始めるかは ZombieBrainComponent が決める。
class ZombieDismemberComponent : public Component{
public:
	// model: 切り口・引きずる腕の設計図（ZombieModel）。所有権は受け取らない。nullptr 不可。この部品より長く生きていること。
	explicit ZombieDismemberComponent( const ZombieModel* model );

	ZombieDismemberment& dismemberment(){ return mDismemberment; }
	ZombieCrawler& crawler(){ return mCrawler; }
	const ZombieCrawler& crawler() const { return mCrawler; }

private:
	ZombieDismemberment mDismemberment;
	ZombieCrawler mCrawler; // mDismemberment を借りる（腕が残っているか）ので、それより後に宣言する
};

#endif
