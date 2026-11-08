#include "GameSrc/Zombie/Component/ZombieCorpseComponent.h"
#include "GameSrc/Zombie/Component/ZombieStateComponent.h"
#include "GameSrc/Zombie/Component/ZombieRootMotionComponent.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/SkinnedModelComponent.h"
#include "GameSrc/World/GameObject.h"

using namespace GameLib::Math;

ZombieCorpseComponent::ZombieCorpseComponent() :
	mPosition( nullptr ),
	mMesh( nullptr ),
	mState( nullptr ),
	mRootSpan( nullptr ) {
}

void ZombieCorpseComponent::start() {
	mPosition = &requireComponent< TransformComponent >()->position();
	mMesh = &requireComponent< SkinnedModelComponent >()->mesh();
	mState = requireComponent< ZombieStateComponent >();
	mRootSpan = &requireComponent< ZombieRootMotionComponent >()->span();
}

void ZombieCorpseComponent::update() {
	switch ( mState->state() ) {
		case ZombieStateComponent::State::Falling:
			if ( mMesh->isAnimationFinished() ) {
				mState->setState( ZombieStateComponent::State::Corpse );
				mCorpse.start();
				mRootSpan->freeze(); // 置き場所は死体の腰の下・絵の根はそのまま（もう動かない）
			}
			break;
		case ZombieStateComponent::State::Corpse:
			mCorpse.tick();
			if ( mCorpse.isSinking() ) {
				mState->setState( ZombieStateComponent::State::Sinking ); // 沈めるのは次のティックから
			}
			break;
		case ZombieStateComponent::State::Sinking: {
			// 見た目の位置だけ下げる（部位の箱も見た目に付いて沈む）。
			mCorpse.tick();
			Vector3 shown = mRootSpan->meshRoot( *mPosition );
			shown.y -= mCorpse.sinkDepth();
			mMesh->setPosition( shown );
			break;
		}
		case ZombieStateComponent::State::Alive:
		case ZombieStateComponent::State::Downed:
			break;
	}
}

bool ZombieCorpseComponent::isGone() const {
	return mState->state() == ZombieStateComponent::State::Sinking && mCorpse.isGone();
}

float ZombieCorpseComponent::secondsUntilGone() const {
	switch ( mState->state() ) {
		case ZombieStateComponent::State::Corpse:
		case ZombieStateComponent::State::Sinking: return mCorpse.secondsUntilGone();
		case ZombieStateComponent::State::Alive:
		case ZombieStateComponent::State::Downed:
		case ZombieStateComponent::State::Falling: break;
	}
	return 0.f;
}
