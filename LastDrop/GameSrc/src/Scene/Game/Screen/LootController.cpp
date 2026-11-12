#include "GameSrc/Scene/Game/Screen/LootController.h"
#include "GameSrc/Scene/Game/Screen/ModalController.h"
#include "GameSrc/Interaction/Item/Inventory.h"
#include "GameSrc/Interaction/Item/ItemGrid.h"
#include "GameSrc/Interaction/Item/MagazineSync.h"
#include "GameSrc/Interaction/IInteractable.h"
#include "GameSrc/Interaction/Loot/LootContainerComponent.h"
#include "GameSrc/Interaction/Loot/LootTable.h"
#include "GameSrc/Interaction/EscapeCarComponent.h"
#include "GameSrc/Scene/Game/World/GameWorld.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Raid/RaidSession.h"
#include "GameSrc/Raid/RaidParams.h"
#include "GameSrc/Player/PlayerCommand.h"
#include "GameSrc/Audio/SoundCatalog.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Core/Pad.h"
#include "GameLib/GameLib.h"
#include "GameLib/Math/Vector3.h"

using namespace GameLib::Math;

namespace{
	const float kSearchSoundVolume = 0.6f; // 開けている間の音の大きさ（自分の手元の音。服の擦れと同じくらい）
	const float kNoMove = 0.f;             // 移動の量がこれなら動いていない（PlayerCommand::forward / strafe）
	// 脱出の車のエンジン。掛かったら、銃声と同じくゾンビが寄ってくる
	const float kEngineStartVolume = 1.f;
	const float kEngineIdleVolume = 0.7f;
	const char* kFuelPortTitle = "給油口"; // 脱出の車を開けた画面の右の見出し
}

LootController::LootController( GameWorld* world, Inventory* inventory, AudioEngine* audio, ModalController* modals, RaidSession* session,
	UiRenderer* ui ) :
	mWorld( world ),
	mInventory( inventory ),
	mAudio( audio ),
	mModals( modals ),
	mSession( session ),
	mScreen( ui ),
	mSearchSound( AudioEngine::kInvalidLoop ),
	mEngineSound( AudioEngine::kInvalidLoop ) {
}

LootController::~LootController() {
	cancelSearch();                   // 開けている間の音を止める
	mAudio->stopLoop( mEngineSound ); // エンジンの音も（掛かっていなければ何もしない）
}

void LootController::beginLoot( IInteractable* opened ) {
	LootContainerComponent* container = mWorld->interaction().findContainer( opened );
	if ( !container ) {
		if ( EscapeCarComponent* car = mWorld->interaction().findEscapeCar( opened ) ) {
			cancelSearch(); // 給油の待ちの途中なら取りやめ（ガソリンは給油口に入ったまま）
			openLoot( car->gameObject(), GridRule::FuelOnly ); // 給油口は待たずに開く
		}
		return;
	}
	float seconds = container->searchSeconds();
	if ( seconds <= 0.f ) {
		openLoot( container->gameObject(), GridRule::Free ); // 一度開けた所は待たない
		return;
	}
	cancelSearch();
	mSearch.start( container->gameObject(), seconds );
	const GameObject* player = mWorld->player();
	if ( player ) {
		// 音は自分の手元から鳴らす（探る手の音。開けている間は動けないので、ループの位置は動かさない）
		mSearchSound = mAudio->startLoopAt( LootTable::searchSound( container->table() ), AudioEngine::BUS_EFFECT,
			player->getComponent< TransformComponent >()->position(), kSearchSoundVolume );
	}
}

void LootController::beginRefuel( EscapeCarComponent* car ) {
	ASSERT( car && "LootController::beginRefuel : car is null." );
	cancelSearch();
	mSearch.start( car->gameObject(), raidParams().refuelSeconds );
	const GameObject* player = mWorld->player();
	if ( player ) {
		// 給油の間は、缶からガソリンを注ぐ音を手元から鳴らす（漁る音は使わない）
		mSearchSound = mAudio->startLoopAt( SOUND_REFUEL_POUR, AudioEngine::BUS_EFFECT,
			player->getComponent< TransformComponent >()->position(), kSearchSoundVolume );
	}
}

void LootController::finishRefuel( EscapeCarComponent* car ) {
	if ( car->isFueled() || !car->isTankFull() ) {
		return; // 起きない（給油口を開くと待つのはやめる）
	}
	car->startEngine();
	mSession->openEscape();
	// エンジンが掛かる。音は車から鳴らし、銃声と同じくゾンビが寄ってくる（逃げる直前の 5 秒に迷いを出す）
	const Vector3& carPosition = car->getComponent< TransformComponent >()->position();
	AudioEngine& audio = *mAudio;
	audio.play( SOUND_CAR_ENGINE_START, AudioEngine::BUS_EFFECT, carPosition, kEngineStartVolume );
	if ( mEngineSound == AudioEngine::kInvalidLoop ) {
		mEngineSound = audio.startLoopAt( SOUND_CAR_ENGINE_IDLE, AudioEngine::BUS_EFFECT, carPosition, kEngineIdleVolume );
	}
	mWorld->alertZombiesToNoise( carPosition );
}

void LootController::cancelSearch() {
	mSearch.cancel();
	if ( mSearchSound != AudioEngine::kInvalidLoop ) {
		mAudio->stopLoop( mSearchSound );
		mSearchSound = AudioEngine::kInvalidLoop;
	}
}

bool LootController::isSearching() const {
	return mSearch.isSearching();
}

RaidHud::ProgressKind LootController::waitingKind() const {
	const GameObject* target = mSearch.target();
	bool refueling = target && target->getComponent< EscapeCarComponent >();
	return refueling ? RaidHud::ProgressKind::Refuel : RaidHud::ProgressKind::Search;
}

bool LootController::interrupts( const PlayerCommand& command ) {
	return command.forward != kNoMove || command.strafe != kNoMove || command.aimHeld || command.fireHeld
		|| command.reloadPressed || command.holsterPressed;
}

void LootController::update( bool commandInterrupts, bool raidFinished ) {
	if ( !mSearch.isSearching() ) {
		return;
	}
	bool attacked = !mWorld->combat().zombieAttacksThisTick().empty();
	bool interrupted = commandInterrupts || attacked || mWorld->isPlayerDead() || raidFinished;
	switch ( mSearch.tick( GameTime::kTickSeconds, interrupted ) ) {
	case LootSearch::Step::Cancelled:
		cancelSearch(); // 音を止める
		break;
	case LootSearch::Step::Finished: {
		LootContainerComponent* container = mSearch.container();
		GameObject* target = mSearch.target();
		EscapeCarComponent* car = target ? target->getComponent< EscapeCarComponent >() : nullptr;
		cancelSearch();
		if ( container ) {
			container->markSearched();
			openLoot( container->gameObject(), GridRule::Free );
		} else if ( car ) {
			finishRefuel( car ); // 待っていたのは給油
		}
		break;
	}
	default:
		break;
	}
}

void LootController::openLoot( GameObject* target, GridRule lootRule ) {
	ASSERT( target && "LootController::openLoot : target is null." );
	mTarget = GameObjectHandle( target );
	// 銃のマスの「弾倉の数/大きさ」を今の数にする（場所へ置いていった銃も、その数のまま残る。カバンを開くときと同じ）
	storeMagazineRounds( *mInventory, *mWorld );
	mScreen.open( lootRule );
	mModals->enter( ModalController::Mode::Loot ); // 一時停止と同じ遷移（音を止め、入力を放し、カーソルを外す）
}

void LootController::closeLoot() {
	// 給油口に要る数を入れて閉じたら給油し始める（閉じるまでは入れ直せる）
	EscapeCarComponent* car = fuelPortTarget();
	bool refuel = car && car->isTankFull();
	mTarget = GameObjectHandle();
	// カバンへ入れた銃（場所から拾った銃）の弾倉の数を、銃の物へ入れる（場所の中の銃は込めた数を持つ。木箱のライフルは 0）
	restoreMagazineRounds( *mInventory, *mWorld );
	mModals->exit(); // 注ぐ音を鳴らす前に音を戻す（exit が setPaused(false)）
	if ( refuel ) {
		beginRefuel( car );
	}
}

void LootController::handleInput() {
	ItemGrid* grid = openedGrid();
	if ( !grid ) {
		closeLoot(); // 起きない（世界は止まっている）が、消えた物を指したままにしない
		return;
	}
	// ESC は、持ち上げている途中ならまずそれを取りやめる。E・Tab はすぐ閉じる（持ち上げていた物は元の所に残る）
	if ( InputMenu() ) {
		if ( !mScreen.back() ) {
			closeLoot();
		}
		return;
	}
	if ( InputInteract() || InputInventory() ) {
		closeLoot();
		return;
	}
	mScreen.handleInput( mInventory->gridForTransfer(), grid );
}

LootContainerComponent* LootController::lootTarget() const {
	GameObject* object = mTarget.get();
	return object ? object->getComponent< LootContainerComponent >() : nullptr;
}

EscapeCarComponent* LootController::fuelPortTarget() const {
	GameObject* object = mTarget.get();
	return object ? object->getComponent< EscapeCarComponent >() : nullptr;
}

ItemGrid* LootController::openedGrid() const {
	if ( LootContainerComponent* container = lootTarget() ) {
		return container->lootGrid();
	}
	EscapeCarComponent* car = fuelPortTarget();
	return car ? car->lootGrid() : nullptr;
}

const char* LootController::openedTitle() const {
	const LootContainerComponent* container = lootTarget();
	return container ? LootTable::displayName( container->table() ) : kFuelPortTitle;
}

void LootController::draw( ItemId equippedWeapon ) {
	mScreen.draw( mInventory->grid(), *openedGrid(), openedTitle(), equippedWeapon );
}

bool LootController::hasOpenGrid() const {
	return openedGrid() != nullptr;
}

float LootController::progress() const {
	return mSearch.progress();
}
