#include "GameSrc/Zombie/ZombieDismemberComponent.h"
#include "GameSrc/Zombie/ZombieModel.h"

ZombieDismemberComponent::ZombieDismemberComponent( const ZombieModel* model ) :
	mDismemberment( &model->severLayout() ),
	mCrawler( &model->crawlLayout(), &mDismemberment ) { // mDismemberment を先に作ってから借りる（メンバの宣言順）
}
