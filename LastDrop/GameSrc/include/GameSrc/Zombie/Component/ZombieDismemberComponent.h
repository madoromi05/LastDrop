#ifndef INCLUDED_ZOMBIE_DISMEMBER_COMPONENT_H
#define INCLUDED_ZOMBIE_DISMEMBER_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Zombie/Damage/ZombieDismemberment.h"
#include "GameSrc/Zombie/Crawl/ZombieCrawler.h"
#include <memory>

// ゾンビの部位欠損（頭・腕・脚が 0 になると見た目から消す ZombieDismemberment）と、脚を失って倒れた後に
// 腕で体を引きずる体（ZombieCrawler。腕の骨・胸の置き場所・腕の速さの段階。SummaryPlan/個別仕様/ゾンビ/ZombieCrawl_Plan.txt）
// 引きずる腕は残っている腕しか使えないので、2 つを同じ部品に置く（ZombieCrawler が ZombieDismemberment を借りる）
// いつ切るかは ZombieHealthComponent、いつ・どちらへ引きずるかは ZombieCrawlComponent が決める
// 体の中心（ZombieState）が引きずる体の胸を読むので、ゾンビには外せない部品
class ZombieDismemberComponent : public Component{
public:
	ZombieDismemberComponent();

	ZombieDismemberment& dismemberment(){ return *mDismemberment; }	// どちらも awake で作る（隣の部品は start から借りる）
	// 最後に撃たれたときに切れた部位（severedThisHit）は、持ち主が千切れた部位を飛ばすのに読む
	const ZombieDismemberment& dismemberment() const { return *mDismemberment; }
	ZombieCrawler& crawler(){ return *mCrawler; }
	const ZombieCrawler& crawler() const { return *mCrawler; }

	void awake() override;	// Component（ZombieModelComponent が要る。切り口・引きずる腕の設計図を借りて作る）

private:
	std::unique_ptr< ZombieDismemberment > mDismemberment;
	std::unique_ptr< ZombieCrawler > mCrawler; // mDismemberment を借りる（腕が残っているか）ので、その後に作る
};

#endif
