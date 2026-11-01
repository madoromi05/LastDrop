#include "GameSrc/Zombie/ZombieDismemberComponent.h"
#include "GameSrc/Zombie/ZombieModel.h"
#include "GameSrc/Zombie/ZombieModelComponent.h"
#include "GameSrc/World/GameObject.h"

ZombieDismemberComponent::ZombieDismemberComponent() {
}

void ZombieDismemberComponent::awake() {
	const ZombieModel* model = requireComponent< ZombieModelComponent >()->model();
	mDismemberment = std::make_unique< ZombieDismemberment >( &model->severLayout() );
	mCrawler = std::make_unique< ZombieCrawler >( &model->crawlLayout(), mDismemberment.get() ); // mDismemberment を先に作ってから借りる
}
