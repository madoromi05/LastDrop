#include "GameSrc/Scene/Game/GameScene.h"
#include "GameSrc/Scene/GameResources.h"
#include "GameSrc/Player/PlayerComponent.h"
#include "GameSrc/Player/Body/FirstPersonBodyComponent.h"
#include "GameSrc/Player/Body/HoldComponent.h"
#include "GameSrc/Player/PlayerHealthComponent.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/LightComponent.h"
#include "GameSrc/Weapon/GunComponent.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/Core/Pad.h"
#include "GameSrc/Scene/SceneManager.h"
#include "GameSrc/Render/Culling/DrawCulling.h"
#include "GameSrc/Render/Lighting/LightingParams.h"
#include "GameSrc/Render/Shadow/ShadowParams.h"
#include "GameSrc/Render/PostProcess/SsrParams.h"
#include "GameSrc/Render/PostProcess/PostProcessParams.h"
#include "GameSrc/DebugMode/EditorWindow.h"
#include "GameSrc/Raid/RunRules.h"
#include "GameSrc/Raid/RaidParams.h"
#include "GameSrc/Weapon/WeaponCatalog.h"
#include "GameSrc/Interaction/IInteractable.h"
#include "GameSrc/Interaction/Loot/LootContainerComponent.h"
#include "GameSrc/Interaction/Loot/LootParams.h"
#include "GameSrc/Interaction/EscapeCarComponent.h"
#include "GameSrc/Core/GameTime.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/Enum.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Functions.h"
#include "GameLib/WindowCreator/WindowCreator.h"
#include "GameLib/Profiling/Profiler.h"
#include "Framework.h"
#include <string>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const float kInteractReach = 2.5f; // 手が届く距離（m）。足元の缶を立ったまま拾える長さ
	const float kNoShotRecoil = 0.f;   // 撃っていないティックの反動の大きさ（FirstPersonControl::updateViewEffects）
	// カバンの画面の見出しの下に出す、今日の脱出の車の場所（FuelEscapeLoading_Plan.txt Q14）。
	const char* kExitsPrefix = "脱出の車：";
	const char* kExitsSeparator = " / ";
	// 漁れる場所（R3_Plan.txt）。
	const float kNoSearch = -1.f;        // HUD に「探している」の棒を出さない（RaidHud::draw）
	const float kSearchSoundVolume = 0.6f; // 開けている間の音の大きさ（自分の手元の音。服の擦れと同じくらい）
	const float kNoMove = 0.f;           // 移動の量がこれなら動いていない（PlayerCommand::forward / strafe）
	// 脱出の車のエンジン（FuelEscapeLoading_Plan.txt Q5）。掛かったら、銃声と同じくゾンビが寄ってくる。
	const float kEngineStartVolume = 1.f;
	const float kEngineIdleVolume = 0.7f;
	const char* kFuelPortTitle = "給油口"; // 脱出の車を開けた画面の右の見出し（FuelPort_Plan.txt）

#ifdef ZF_EDITOR
	// ── F9 でゾンビを出す（確認用。 ユーザー指定：8m 前に 1 体）──
	const float kDebugSpawnDistance = 8.f; // 自キャラの足元から、向いている方へ（m）
	const int kDebugSpawnNumber = 1;
	// 横並びの間（m）。体の当たり判定の幅（約 0.8m）より広くして、出た瞬間に重ならないようにする。
	const float kDebugSpawnSpacing = 1.2f;
	const float kFaceBackDegree = 180.f; // 自キャラの方を向かせる
	const float kHalf = 0.5f;
#endif
}

GameScene::GameScene( GameResources* resources ) :
	mResources( resources ),
	// 今日の条件（5 日間の始めに決めて save.xml にある。DayCondition_Plan.txt）。世界を作る時の 1 回だけ読む（F6 で変えたら次の出撃から）。
	mWorld( &resources->meshes(), &resources->town(), dayConditionEffect( resources->run().condition( resources->run().day() ) ) ),
	mSceneRenderer( &mWorld ), // 世界を画面へ写すディファードのパイプライン一式（mWorld を読むので、その後に並べる）
	mInventory( resources->run().bag() ), // 拠点のカバンの写し（町の中で変わっても、拠点のカバンは脱出するまで変えない。R2_Plan.txt Q3）
	mInteractor( kInteractReach, [ this ]( const Vector3& origin, const Vector3& delta ) { return mWorld.interaction().findInteractable( origin, delta, mWorld.playerTarget() ); } ),
	mControl( mWorld.map().playerStart().yawDegree, &resources->settings() ), // キャラの向いている方を見始める
	mBodySounds( &resources->audio(), &mWorld.ground(), mWorld.map().groundZones() ),
	mCombatSounds( &resources->audio() ),
	mHud( &resources->uiRenderer() ),
	mAmmoHud( &resources->uiRenderer() ),
	// 脱出の車の置き場所は毎日変える（起動からの時間を種にする。ガソリンを入れる箱と同じ）。
	mSession( mWorld.map().escapeCarSpots(), Framework::instance().time() ),
	mLoader( &mWorld, &resources->meshes(), mSession.escapeCarSpot() ),
	mLoadingScreen( &resources->uiRenderer() ),
	mPauseMenu( &resources->uiRenderer(), &resources->settings(), &resources->audio(), [ this ]() { resume(); } ),
	mInventoryScreen( &resources->uiRenderer() ),
	mLootScreen( &resources->uiRenderer() ),
	mModals( &resources->audio(), &mControl ), // 音とカーソルを借りて、モーダルの開閉の共通遷移を受け持つ
	mReady( false ),
	mRaidFinished( false ),
	mOffer( nullptr ),
	mSearchSound( AudioEngine::kInvalidLoop ),
	mEngineSound( AudioEngine::kInvalidLoop ) {
}

GameScene::~GameScene() {
	cancelLootSearch(); // 開けている間の音を止める（音の道具は次のシーンも使う）
	mResources->audio().stopLoop( mEngineSound ); // 同じ（掛かっていなければ何もしない）
	if ( mModals.isOpen() ) {
		mResources->audio().setPaused( false );
	}
	EditorWindow::setPlayerInvincible( false ); // 自キャラがいなくなるので、ON のまま残さない
	EditorWindow::setObjects( nullptr ); // 世界が消えるので、消えた物を指し続けないように
	EditorWindow::setInventory( nullptr ); // カバンも同じ
	EditorWindow::setRaid( nullptr );
	EditorWindow::setRun( nullptr );
	EditorWindow::setInteraction( nullptr );
}

void GameScene::pause() {
	mPauseMenu.open(); // 前に設定のページで閉じていても、最初のページから
	mModals.enter( ModalController::Mode::Pause ); // 音を止め、押していた入力を放し、カーソルを外す
}

void GameScene::resume() {
	mModals.exit();
}

void GameScene::handlePausedInput() {
	// ESC は 1 ページ戻る。最初のページで押したらゲームへ戻る。
	if ( InputMenu() ) {
		if ( !mPauseMenu.back() ) {
			resume();
		}
		return;
	}
	mPauseMenu.handleInput(); // Resume が押されたら、この中で resume が呼ばれる
}

bool GameScene::canOpenBag() const {
	return mWorld.player() && !mWorld.isPlayerDead() && !isFreeCameraActive();
}

void GameScene::openBag() {
	// 銃のマスの「弾倉の数/大きさ」を今の数にする（町の中では銃の物が本当の数を持つ。InventoryPickup_Plan.txt Q2）。
	storeMagazineRounds();
	mInventoryScreen.open();
	mModals.enter( ModalController::Mode::Bag ); // 一時停止と同じ遷移（音を止め、入力を放し、カーソルを外す）
}

void GameScene::closeBag() {
	mOffer = nullptr; // 入れていない分は地面に残る
	mModals.exit();
}

void GameScene::startPlacing( IInteractable* offer ) {
	ItemStack stack;
	if ( !canOpenBag() || !offer->offeredStack( &stack ) ) {
		return;
	}
	openBag();
	mOffer = offer;
	ItemStack shown;
	offeredStack( &shown );
	mInventoryScreen.openForPlacing( mInventory, shown );
}

bool GameScene::offeredStack( ItemStack* out ) const {
	if ( !mOffer || !mOffer->offeredStack( out ) ) {
		return false;
	}
	// 銃は弾倉の数を銃の物から読む（拾った銃のマスにも「30/30」が出るように）。
	if ( weaponForItem( out->item ) != WEAPON_NONE ) {
		out->loadedRounds = mWorld.magazineRounds( out->item );
	}
	return true;
}

void GameScene::placeOffered( int cell ) {
	ItemStack stack;
	if ( !offeredStack( &stack ) ) {
		return;
	}
	int count = mInventory.countPlaceableInto( cell, stack );
	if ( count <= 0 ) {
		return;
	}
	mInventory.placeInto( cell, stack, count );
	mOffer->takeOffered( count ); // 入りきらない分は持ったまま（続けて別のマスへ置ける）
	finishPlacingIfDone();
}

void GameScene::quickPlaceOffered() {
	if ( !mOffer->canInteract( mInventory ) ) {
		return; // どこにも入らない（捨てる・並べ替えるで空けてから）
	}
	mOffer->interact( &mInventory ); // 今までの自動の入れ方（重ねられるマス → 上の行・左の列の空き）
	storeMagazineRounds();           // 拾った銃のマスにも弾倉の数を出す
	finishPlacingIfDone();
}

void GameScene::finishPlacingIfDone() {
	ItemStack rest;
	if ( !offeredStack( &rest ) ) {
		closeBag(); // 全部入れた。拾う操作が長くならないよう、町へ戻す（Q3）
	}
}

void GameScene::handleBagInput() {
	// Tab でも ESC でも閉じる（ESC で一時停止のメニューは開かない。GridInventory_Plan.txt Q8）。
	// ESC は拾った物を持っている・メニュー・動かしている途中なら、まずそれを取りやめる（拾うのをやめても開いたまま。Q3）。
	if ( InputInventory() ) {
		closeBag();
		return;
	}
	if ( InputMenu() ) {
		if ( !mInventoryScreen.back() ) {
			closeBag();
		} else if ( !mInventoryScreen.isPlacing() ) {
			mOffer = nullptr; // 拾うのをやめた（地面に残る）
		}
		return;
	}
	ItemStack incoming;
	bool placing = mInventoryScreen.isPlacing() && offeredStack( &incoming );
	InventoryScreen::Request request = mInventoryScreen.handleInput( &mInventory, equippedWeapon(), isPlayerInjured(),
		placing ? &incoming : nullptr );
	switch ( request.kind ) {
	case InventoryScreen::Request::USE:
		useFromBag( request.cell );
		break;
	case InventoryScreen::Request::PLACE:
		placeOffered( request.cell );
		break;
	case InventoryScreen::Request::QUICK_PLACE:
		quickPlaceOffered();
		break;
	case InventoryScreen::Request::DROP:
		// 手に持っている銃を捨てたら、閉じた後の最初のティックで素手になる（HoldComponent::syncWithInventory）。
		dropFromBag( request.cell );
		break;
	case InventoryScreen::Request::EQUIP: {
		// 開いている間は世界が止まっているので、持ち替えの時間は作らない（RE:2 と同じ。Pistol_Plan.txt Q6）。
		GameObject* player = mWorld.player();
		HoldComponent* hold = player ? player->getComponent< HoldComponent >() : nullptr;
		if ( hold ) {
			hold->equip( mInventory.grid().stackAt( request.cell ).item );
		}
		break;
	}
	default:
		break;
	}
}

void GameScene::dropFromBag( int cell ) {
	ItemStack dropped = mInventory.takeAll( cell );
	if ( !mWorld.dropItem( dropped, mControl.camera().yaw() ) ) {
		// 置けなかった（形の構築に失敗した）。無くさないようカバンへ戻す（空いたばかりなので必ず入る）。
		for ( int i = 0; i < dropped.amount; ++i ) {
			mInventory.add( dropped.item );
		}
	}
}

void GameScene::useFromBag( int cell ) {
	GameObject* player = mWorld.player();
	PlayerHealthComponent* health = player ? player->getComponent< PlayerHealthComponent >() : nullptr; // 外せる部品
	if ( !health || !health->health().isInjured() ) {
		return; // 満タンでは使わない（メニューにも出していない）
	}
	ItemId item = mInventory.grid().stackAt( cell ).item;
	ASSERT( itemSpec( item ).kind == ITEM_KIND_MEDICAL && "GameScene::useFromBag : not a medical item." );
	health->heal( lootParams().bandageHeal );
	mInventory.remove( item ); // 同じ物なら、どのマスから減っても同じ（数が一番少ないマスから減る）
}

bool GameScene::isPlayerInjured() const {
	const GameObject* player = mWorld.player();
	const PlayerHealthComponent* health = player ? player->getComponent< PlayerHealthComponent >() : nullptr;
	return health && health->health().isInjured();
}

PlayerCondition GameScene::playerCondition() const {
	const GameObject* player = mWorld.player();
	const PlayerHealthComponent* health = player ? player->getComponent< PlayerHealthComponent >() : nullptr;
	return health ? health->health().condition() : PlayerCondition::Fine;
}

void GameScene::beginLoot( IInteractable* opened ) {
	LootContainerComponent* container = mWorld.interaction().findContainer( opened );
	if ( !container ) {
		if ( EscapeCarComponent* car = mWorld.interaction().findEscapeCar( opened ) ) {
			cancelLootSearch(); // 給油の待ちの途中なら取りやめ（ガソリンは給油口に入ったまま）
			openLoot( car->gameObject(), GridRule::FuelOnly ); // 給油口は待たずに開く（FuelPort_Plan.txt）
		}
		return;
	}
	float seconds = container->searchSeconds();
	if ( seconds <= 0.f ) {
		openLoot( container->gameObject(), GridRule::Free ); // 一度開けた所は待たない（R3_Plan.txt Q6）
		return;
	}
	cancelLootSearch();
	mLootSearch.start( container->gameObject(), seconds );
	const GameObject* player = mWorld.player();
	if ( player ) {
		// 音は自分の手元から鳴らす（探る手の音。開けている間は動けないので、ループの位置は動かさない）。
		mSearchSound = mResources->audio().startLoopAt( LootTable::searchSound( container->table() ), AudioEngine::BUS_EFFECT,
			player->getComponent< TransformComponent >()->position(), kSearchSoundVolume );
	}
}

void GameScene::beginRefuel( EscapeCarComponent* car ) {
	ASSERT( car && "GameScene::beginRefuel : car is null." );
	cancelLootSearch();
	mLootSearch.start( car->gameObject(), raidParams().refuelSeconds );
	const GameObject* player = mWorld.player();
	if ( player ) {
		// 給油の間は、缶からガソリンを注ぐ音を手元から鳴らす（漁る音は使わない。FuelEscapeLoading_Plan.txt 改訂 2）。
		mSearchSound = mResources->audio().startLoopAt( SOUND_REFUEL_POUR, AudioEngine::BUS_EFFECT,
			player->getComponent< TransformComponent >()->position(), kSearchSoundVolume );
	}
}

void GameScene::finishRefuel( EscapeCarComponent* car ) {
	if ( car->isFueled() || !car->isTankFull() ) {
		return; // 起きない（給油口を開くと待つのはやめる）
	}
	car->startEngine();
	mSession.openEscape();
	// エンジンが掛かる。音は車から鳴らし、銃声と同じくゾンビが寄ってくる（逃げる直前の 5 秒に迷いを出す。Q5）。
	const Vector3& carPosition = car->getComponent< TransformComponent >()->position();
	AudioEngine& audio = mResources->audio();
	audio.play( SOUND_CAR_ENGINE_START, AudioEngine::BUS_EFFECT, carPosition, kEngineStartVolume );
	if ( mEngineSound == AudioEngine::kInvalidLoop ) {
		mEngineSound = audio.startLoopAt( SOUND_CAR_ENGINE_IDLE, AudioEngine::BUS_EFFECT, carPosition, kEngineIdleVolume );
	}
	mWorld.alertZombiesToNoise( carPosition );
}

void GameScene::cancelLootSearch() {
	mLootSearch.cancel();
	if ( mSearchSound != AudioEngine::kInvalidLoop ) {
		mResources->audio().stopLoop( mSearchSound );
		mSearchSound = AudioEngine::kInvalidLoop;
	}
}

RaidHud::ProgressKind GameScene::waitingKind() const {
	const GameObject* target = mLootSearch.target();
	bool refueling = target && target->getComponent< EscapeCarComponent >();
	return refueling ? RaidHud::ProgressKind::Refuel : RaidHud::ProgressKind::Search;
}

bool GameScene::interruptsLootSearch( const PlayerCommand& command ) {
	return command.forward != kNoMove || command.strafe != kNoMove || command.aimHeld || command.fireHeld
		|| command.reloadPressed || command.holsterPressed;
}

void GameScene::updateLootSearch( bool commandInterrupts ) {
	if ( !mLootSearch.isSearching() ) {
		return;
	}
	bool attacked = !mWorld.combat().zombieAttacksThisTick().empty();
	bool interrupted = commandInterrupts || attacked || mWorld.isPlayerDead() || mRaidFinished;
	switch ( mLootSearch.tick( GameTime::kTickSeconds, interrupted ) ) {
	case LootSearch::Step::Cancelled:
		cancelLootSearch(); // 音を止める
		break;
	case LootSearch::Step::Finished: {
		LootContainerComponent* container = mLootSearch.container();
		GameObject* target = mLootSearch.target();
		EscapeCarComponent* car = target ? target->getComponent< EscapeCarComponent >() : nullptr;
		cancelLootSearch();
		if ( container ) {
			container->markSearched();
			openLoot( container->gameObject(), GridRule::Free );
		} else if ( car ) {
			finishRefuel( car ); // 待っていたのは給油（FuelEscapeLoading_Plan.txt Q4）
		}
		break;
	}
	default:
		break;
	}
}

void GameScene::openLoot( GameObject* target, GridRule lootRule ) {
	ASSERT( target && "GameScene::openLoot : target is null." );
	mLootTarget = GameObjectHandle( target );
	// 銃のマスの「弾倉の数/大きさ」を今の数にする（場所へ置いていった銃も、その数のまま残る。カバンを開くときと同じ）。
	storeMagazineRounds();
	mLootScreen.open( lootRule );
	mModals.enter( ModalController::Mode::Loot ); // 一時停止と同じ遷移（音を止め、入力を放し、カーソルを外す）
}

void GameScene::closeLoot() {
	// 給油口に要る数を入れて閉じたら給油し始める（閉じるまでは入れ直せる。FuelPort_Plan.txt）。
	EscapeCarComponent* car = fuelPortTarget();
	bool refuel = car && car->isTankFull();
	mLootTarget = GameObjectHandle();
	// カバンへ入れた銃（場所から拾った銃）の弾倉の数を、銃の物へ入れる（場所の中の銃は込めた数を持つ。木箱のライフルは 0）。
	restoreMagazineRounds();
	mModals.exit(); // 注ぐ音を鳴らす前に音を戻す（exit が setPaused(false)）
	if ( refuel ) {
		beginRefuel( car );
	}
}

void GameScene::handleLootInput() {
	ItemGrid* grid = openedGrid();
	if ( !grid ) {
		closeLoot(); // 起きない（世界は止まっている）が、消えた物を指したままにしない
		return;
	}
	// ESC は、持ち上げている途中ならまずそれを取りやめる。E・Tab はすぐ閉じる（持ち上げていた物は元の所に残る）。
	if ( InputMenu() ) {
		if ( !mLootScreen.back() ) {
			closeLoot();
		}
		return;
	}
	if ( InputInteract() || InputInventory() ) {
		closeLoot();
		return;
	}
	mLootScreen.handleInput( mInventory.gridForTransfer(), grid );
}

LootContainerComponent* GameScene::lootTarget() const {
	GameObject* object = mLootTarget.get();
	return object ? object->getComponent< LootContainerComponent >() : nullptr;
}

EscapeCarComponent* GameScene::fuelPortTarget() const {
	GameObject* object = mLootTarget.get();
	return object ? object->getComponent< EscapeCarComponent >() : nullptr;
}

ItemGrid* GameScene::openedGrid() const {
	if ( LootContainerComponent* container = lootTarget() ) {
		return container->lootGrid();
	}
	EscapeCarComponent* car = fuelPortTarget();
	return car ? car->lootGrid() : nullptr;
}

const char* GameScene::openedTitle() const {
	const LootContainerComponent* container = lootTarget();
	return container ? LootTable::displayName( container->table() ) : kFuelPortTitle;
}

ItemId GameScene::equippedWeapon() const {
	const GunComponent* gun = HoldComponent::heldGun( mWorld.player() );
	return gun ? gun->spec().item : ITEM_NONE;
}

void GameScene::addAmmoHud() {
	const GunComponent* gun = HoldComponent::heldGun( mWorld.player() );
	// 消えていく途中に銃を手放した（捨てた・死んだ）ら、その場で出すのをやめる（出す物が無い）。
	if ( !gun || !mAmmoHudVisibility.isVisible() ) {
		return;
	}
	AmmoHud::Ammo ammo;
	ammo.weapon = gun->spec().item;
	ammo.rounds = gun->magazineRounds();
	ammo.capacity = gun->spec().magazineCapacity;
	ammo.reserve = mInventory.count( gun->spec().ammo );
	mAmmoHud.add( ammo, mAmmoHudVisibility.opacity() );
}

void GameScene::beginFrame() {
	// 読み込み・構築・テクスチャのロードを進める（構築後もテクスチャのために毎回呼ぶ）。
	// 2回目以降のこのシーン（次の日の出撃）では、もう構築済みなのですぐに並べられる。
	bool allReady = false;
	{
		PROFILE_SCOPE( "Game.meshes" );
		allReady = mResources->meshes().update();
	}
	if ( !mReady ) {
		// ロードは 1 回の描画に 1 段階ずつ（間にロード画面を描き、窓のメッセージを回す。FuelEscapeLoading_Plan.txt 6 章）。
		if ( !mLoader.advance( allReady ) ) {
			return;
		}
		restoreMagazineRounds(); // 銃の物を作った後に、拠点で覚えた弾倉の数を入れる
		mControl.lockCursor(); // 始めから視点を回せるようにロックしておく
		mReady = true;
#ifdef ZF_EDITOR
		mFireProbe.markReady();
#endif
	}
	{
		PROFILE_SCOPE( "Game.input" );
		handleFrameInput();
	}
#ifdef ZF_EDITOR
	probeFire();
#endif
}

void GameScene::handleFrameInput() {
	// 止めている間は、メニューの操作だけを読む（視点・移動・使う・確認用のキーは読まない）。
	if ( mModals.isPause() ) {
		handlePausedInput();
		return;
	}
	// カバンを開いている間も同じ（ESC はカバンを閉じる方に使う。世界は止まっているので、窓が裏に回っても一時停止のメニューは重ねない）。
	if ( mModals.isBag() ) {
		handleBagInput();
		return;
	}
	// 中を見る画面もカバンと同じ（R3_Plan.txt Q8）。
	if ( mModals.isLoot() ) {
		handleLootInput();
		return;
	}
	// ESC か、窓が裏に回ったら止める（裏に回っている間に世界が進んで、戻ったら攻撃されていた、とならないように）。
	// 死んだ後は止めない（一時停止からやめると、その日をやり直せるので、死んでから逃げられないように。R2_Plan.txt Q3）。
	if ( ( InputMenu() || !GameLib::WindowCreator().isActive() ) && !mWorld.isPlayerDead() ) {
		pause();
		return;
	}
	if ( InputInventory() && canOpenBag() ) {
		cancelLootSearch(); // 開けている途中にカバンを開いたら、開けるのはやめる（閉じた後に勝手に開かないように）
		openBag();
		return;
	}

#ifdef ZF_EDITOR
	// デバッグ用。F5 で自キャラの無敵を切り替える（押した瞬間。Debug と Profile ビルドだけで効く）。
	if (InputDebugInvincible()) {
		GameObject* player = mWorld.player();
		if (player) {
			if ( PlayerHealthComponent* health = player->getComponent< PlayerHealthComponent >() ) { // 外せる部品（無ければ元から死なない）
				health->toggleInvincible();
			}
		}
	}
	// F6 のエディタの一番上に出す。
	const PlayerHealthComponent* playerHealth = mWorld.player() ? mWorld.player()->getComponent< PlayerHealthComponent >() : nullptr;
	EditorWindow::setPlayerInvincible( playerHealth && playerHealth->isInvincible() );
	EditorWindow::setObjects( &mWorld.objects() ); // F6 の Objects の欄（世界に置いた物の一覧）
	EditorWindow::setInventory( &mInventory );     // F6 の Inventory の欄（物を足す・行を増やす）
	EditorWindow::setRaid( &mSession );            // F6 の Raid の欄（残り時間・すぐ脱出）
	EditorWindow::setRun( &mResources->run() );    // 同じ欄の、日・保存を消す
	EditorWindow::setInteraction( &mWorld.interaction() ); // F6 の Loot の欄（中身を振り直す・全部開けたことにする）


	// 観察用カメラの切り替えを先に読む（この描画のマウスとキーを、自キャラと観察用カメラのどちらが使うか決めるため）。
	mFreeCamera.handleToggle( mControl.camera() );
#endif
	// 視点はマウスが動いた分だけ回す。update の回数に合わせると、2回進めた描画で2倍回ってしまう。
	if ( isFreeCameraActive() ) {
		mControl.readInputIdle(); // 自キャラはその場で立ち止まる（マウスとキーは観察用カメラが使う）
	} else if ( mWorld.isPlayerDead() ) {
		mControl.readInputIdle(); // 死んだ後は視点も回さない（崩れて暗くなるのを見せる）
	} else {
		mControl.readInput( aimWeight() );
	}
	updateInteraction();
#ifdef ZF_EDITOR
	mFreeCamera.look(); // 使っていなければ何もしない。カーソルロックの切り替え（readInputIdle）の後に読む
	mDebug.handleInput();
	if ( InputDebugSpawnZombie() ) {
		spawnDebugZombies();
	}
#endif
}

#ifdef ZF_EDITOR
void GameScene::spawnDebugZombies() {
	const GameObject* player = mWorld.player();
	if ( !player ) {
		return;
	}
	// 視点の水平の向きの前と右（PlayerLocomotionComponent::readMoveDirection の cameraForward / cameraRight と同じ式）。間に建物があっても調べない（中に出ることがある）。
	float yaw = mControl.camera().yaw();
	Vector3 forward( Math::sin( yaw ), 0.f, Math::cos( yaw ) );
	Vector3 right( -Math::cos( yaw ), 0.f, Math::sin( yaw ) );
	Vector3 center;
	center.setMadd( player->getComponent< TransformComponent >()->position(), forward, kDebugSpawnDistance );
	// 真ん中を中心に、横へ等間隔に並べる（3 体なら -1.2m・0・+1.2m）。
	float firstOffset = -kDebugSpawnSpacing * static_cast< float >( kDebugSpawnNumber - 1 ) * kHalf;
	for ( int i = 0; i < kDebugSpawnNumber; ++i ) {
		Vector3 position;
		position.setMadd( center, right, firstOffset + kDebugSpawnSpacing * static_cast< float >( i ) );
		if ( !mWorld.spawnZombie( position, yaw + kFaceBackDegree ) ) {
			break; // 上限に達した
		}
	}
}

void GameScene::probeFire() {
	FireProbe::FrameState frame;
	frame.rawFireHeld = InputFireHeld();
	frame.commandFire = mControl.command().fireHeld;
	frame.suppressed = mControl.isFireSuppressed();
	frame.cursorLocked = mControl.isCursorLocked();
	frame.windowActive = GameLib::WindowCreator().isActive();
	frame.paused = mModals.isPause();
	frame.bagOpen = mModals.isBag();
	frame.lootOpen = mModals.isLoot();
	frame.freeCamera = isFreeCameraActive();
	frame.dead = mWorld.isPlayerDead();
	FireProbe::GunState gunState = {};
	const GameObject* player = mWorld.player();
	const HoldComponent* hold = player ? player->getComponent< HoldComponent >() : nullptr;
	const GunComponent* gun = HoldComponent::heldGun( player );
	if ( hold && gun ) {
		gunState.hasGun = true;
		gunState.inHand = hold->isWeaponInHand();
		gunState.aiming = hold->isAiming();
		gunState.reloading = gun->isReloading();
		gunState.running = mControl.command().run;
		gunState.rounds = gun->magazineRounds();
	}
	mFireProbe.onFrame( frame, gunState );
}
#endif

void GameScene::update() {
	if ( !mReady || mModals.isOpen() || mRaidFinished ) {
		return; // 1 日を終えた後と、止めている間（一時停止・カバン・中を見る画面）は世界を進めない（ティックの数は数えられるが、何もしないので戻った瞬間に早送りにならない）
	}
	// 開けている途中の取りやめは、このティックの操作で決める（押した瞬間の操作は、世界を進めた後に消えるので先に読む）。
	bool commandInterruptsSearch = interruptsLootSearch( mControl.command() );
	// 順番に意味がある: 向きを決める（beginFrame）→ その向きでキャラを動かし体を曲げる → 動いた後の頭の位置へ視点を置く（draw）。
	// 逆にすると W が1フレーム前の向きへの前進になり、視点も体に1フレーム遅れて、腕や銃が画面の中で揺れて見える。
	const FirstPersonCamera& camera = mControl.camera();
	// 弾は照準のぶれを足した向き（銃が向いている所）へ飛ぶ。画面の中央（viewDirection）からは、ぶれの分だけずれる。
	mWorld.update( mControl.command(), camera.yaw(), camera.pitch(), camera.weaponDirection(), &mInventory );
	mControl.consumePressed(); // 使ったので消す（同じ描画の間の次の update で、もう一度反応しないように）
	{
		PROFILE_SCOPE( "Game.hitFeedback" );
		mCombatSounds.playHits( mWorld.combat() ); // ゾンビ・物に当たった音（このティックに当たった分だけ。次の update で入れ替わるので、ここで読む）
		shakeOnHeavyHits(); // 揺れも同じ出来事を読む（次の updateViewEffects から効く）
	}
	{
		PROFILE_SCOPE( "Game.damage" );
		updatePlayerDamage();
	}
	{
		PROFILE_SCOPE( "Game.raid" );
		updateRaid();
	}
	{
		PROFILE_SCOPE( "Game.loot" );
		updateLootSearch( commandInterruptsSearch ); // 攻撃された・死んだ・1 日が終わったも読むので、その後に
	}
	if ( mModals.isLoot() ) {
		return; // 開け終わって中を見る画面を開いた（世界を止めたので、同じ描画の残りのティックも進めない）
	}
	PROFILE_SCOPE( "Game.view" ); // ここから update の終わりまで
	GameObject* player = mWorld.player();
	const GunComponent* gun = HoldComponent::heldGun( player ); // 素手なら nullptr
	// 撃ったティックの弾は跳ねる前の向きへ飛び、跳ねた視点は次のティックから効く（1発目は狙った所へ飛ぶ）。
	// 攻撃された怯み・揺れ（updatePlayerDamage で受けた分）もここでカメラへ渡す。
	// 照準のぶれも同じく次のティックから効く（自キャラがいなければ既定の状態＝ぶれない）。
	// 反動の大きさは銃ごと（拳銃はライフルの半分。WeaponCatalog）。
	float shotRecoil = ( gun && gun->firedThisTick() ) ? gun->spec().recoilScale : kNoShotRecoil;
#ifdef ZF_EDITOR
	mFireProbe.onTick( gun && gun->firedThisTick(), gun && gun->dryFiredThisTick() );
#endif
	// 右下の弾数は、銃を覗いている間とリロード中だけ出す（持っているだけでは出さない。AmmoHud_Plan.txt Q1）。
	const HoldComponent* hold = player ? player->getComponent< HoldComponent >() : nullptr;
	mAmmoHudVisibility.update( hold && gun && ( hold->isAiming() || gun->isReloading() ) );
	mControl.updateViewEffects( shotRecoil, player ? player->getComponent< PlayerComponent >()->aimSwayBody() : AimSwayBody() );
	if ( player ) {
		// 銃の見た目へ、弾の向き（weaponDirection）と同じ角度のぶれを渡す。
		player->getComponent< FirstPersonBodyComponent >()->setWeaponSway( mControl.camera().weaponSwayYaw(), mControl.camera().weaponSwayPitch() );
		mBodySounds.update( *player );
	}
#ifdef ZF_EDITOR
	mFreeCamera.move(); // 使っていなければ何もしない
#endif
}

bool GameScene::isLoading() const {
	return !mReady;
}

void GameScene::draw() {
	if ( !mReady ) {
		// ロード中は世界を描かず、ロード画面だけ（FuelEscapeLoading_Plan.txt 6 章）。
		mLoadingScreen.draw( mResources->run().day(), mLoader.stageLabel(), mLoader.progress() );
		return;
	}
	mControl.setEyeDrop( mDamageView.eyeDrop() ); // 死んだ後に膝から崩れる分だけ、目を下げて置く
	const GameObject* player = mWorld.player();
	if ( player ) {
		Vector3 eye = player->getComponent< FirstPersonBodyComponent >()->eyePosition();
		mControl.placeCamera( &eye, aimWeight() );
	} else {
		mControl.placeCamera( nullptr, aimWeight() );
	}
#ifdef ZF_EDITOR
	// 自キャラの視点は、観察用カメラの間も目へ置き続ける（見ている物を使う視線と、戻ったときの位置のため）。
	// 描くのは観察用カメラから（自キャラの視点で設定した描画ステートを上書きする）。
	if ( mFreeCamera.isActive() ) {
		mFreeCamera.apply();
	}
#endif
	const FirstPersonCamera& camera = viewCamera();
	// このフレームで描くカメラ（観察用カメラの間はそちら）を、次のティックのゾンビのアニメの間引きのために渡す（OptimizationB_Plan.txt Q9）。
	mWorld.setAnimationView( camera.projectionViewMatrix(), camera.eyePosition() );
	{
		PROFILE_SCOPE( "Audio" );
		updateListener();
	}
	// 世界を画面へ写す（ディファードのパイプラインは SceneRenderer が持つ。GBuffer_Plan.txt〜Taa_Plan.txt）。
	// 攻撃された瞬間の赤み・痛み・死んだ後の暗さは、この写すときに効く（PlayerDamageView から渡す）。
	SceneRenderer::DamagePostEffect damage = { mDamageView.damageFlash(), mDamageView.pain(), mDamageView.fade() };
	mSceneRenderer.render( camera, damage
#ifdef ZF_EDITOR
		, &mDebug // 当たり判定・ライトの表示（パイプラインの途中に挟むので渡す）
#endif
	);
	// UiRenderer は 1 回の描画に 1 回しか描けない（中の PrimitiveRenderer が 2 回目の draw で ASSERT）。
	// 止めている間は HUD を出さず、メニューがまとめて描く（2026-10-23。HUD とメニューの両方が描いて止まっていた）。
	if ( mModals.isPause() ) {
		mPauseMenu.draw();
	} else if ( mModals.isBag() ) {
		std::string exits = kExitsPrefix + RaidHud::exitNames( mSession.extraction(), kExitsSeparator );
		ItemStack incoming;
		bool placing = mInventoryScreen.isPlacing() && offeredStack( &incoming );
		mInventoryScreen.draw( mInventory, equippedWeapon(), exits.c_str(), placing ? &incoming : nullptr, playerCondition() ); // カバンの画面も同じ（HUD は描かない）
	} else if ( mModals.isLoot() && openedGrid() ) {
		mLootScreen.draw( mInventory.grid(), *openedGrid(), openedTitle(), equippedWeapon() );
	} else {
#ifdef ZF_EDITOR
		// 積むだけ。すぐ下の HUD の描画（UiRenderer::draw）でまとめて描かれる。
		mDebug.addZombieHealth( mWorld, camera, &mResources->uiRenderer() );
		mDebug.addPlayerHealth( mWorld, &mResources->uiRenderer() );
		mDebug.addLootContents( mWorld, camera, &mResources->uiRenderer() );
#endif
		// 使える物の上のマーク（積むだけ。すぐ下の HUD の描画でまとめて描かれる）。
		mInteractMarkers.add( camera, &mResources->uiRenderer() );
		addAmmoHud(); // 右下の弾数（積むだけ。同じく HUD の描画でまとめて描かれる）
		// 真ん中の照準「+」は出さない（ユーザー指定 2026-10-26）。
		mHud.draw( mSession, mResources->run().day(), mWorld.dayCondition(), mLootSearch.isSearching() ? mLootSearch.progress() : kNoSearch, waitingKind() );
	}

	GameDebugOverlay::TextState text;
	text.cursorLocked = mControl.isCursorLocked();
	// パイプラインのシェーダの誤りは SceneRenderer がまとめて順に見る（写す→キャラ→G-Buffer→表示）。
	// キャラのスキニングの誤りは GameResources が持つので、順番の都合で渡す。
	text.shaderError = mSceneRenderer.firstShaderError( mResources->meshes().skinning().errorMessage() );
}

void GameScene::updateInteraction() {
	const GameObject* player = mWorld.player();
	if ( !player ) {
		mInteractMarkers.clear();
		return;
	}
	// 視線は前回の描画で置いたカメラから撃つ（目の位置・向きは draw で決まるため）。
	const FirstPersonCamera& camera = mControl.camera();
	// 観察用カメラの間は E で使わない（自キャラの見ている物が、観察している画面からは分からないため）。マークは今まで通り出す。
	bool interactPressed = InputInteract() && !isFreeCameraActive() && !mWorld.isPlayerDead();
	if ( interactPressed && mLootSearch.isSearching() ) {
		cancelLootSearch(); // 開けている途中に E をもう一度押したら取りやめ（R3_Plan.txt Q6）
		interactPressed = false;
	}
	mInteractor.update( camera.eyePosition(), camera.viewDirection(), &mInventory, interactPressed );
	// 拾う物は、カバンを開いて入れるマスを選ぶ（InventoryPickup_Plan.txt Q3）。拾い終えた物は次の描画の後片付けで消える。
	if ( IInteractable* offer = mInteractor.offer() ) {
		startPlacing( offer );
	}
	// 漁れる場所は、開けるまで待ってから中を見る画面を開く（R3_Plan.txt 5 章）。
	if ( IInteractable* opened = mInteractor.opened() ) {
		beginLoot( opened );
	}
	// 拾われた物は印を付けるだけ。実際に消えるのはティックの最後（GameWorld::update）。
	mWorld.interaction().removeTakenPickups();
	// マークは使った後の状態で決める（拾い切った物は出さない）。死んだ後は出さない。
	if ( mWorld.isPlayerDead() ) {
		mInteractMarkers.clear();
	} else {
		mInteractMarkers.update( mWorld.interaction(), camera.eyePosition(), mInteractor.target(), mInventory );
	}
}

void GameScene::updateListener() {
	// 聞き手は目の位置と視線の向き。首を振ると音の左右が入れ替わる。観察用カメラの間はそのカメラで聞く（近づいたゾンビの音を確かめるため）。
	const FirstPersonCamera& camera = viewCamera();
	AudioEngine& audio = mResources->audio();
	audio.setListener( camera.eyePosition(), camera.viewDirection() );
	audio.update();
}

float GameScene::aimWeight() const {
	const GameObject* player = mWorld.player();
	const HoldComponent* hold = player ? player->getComponent< HoldComponent >() : nullptr; // 外せる部品（無ければ素手）
	return hold ? hold->aimWeight() : 0.f;
}

const FirstPersonCamera& GameScene::viewCamera() const {
#ifdef ZF_EDITOR
	if ( mFreeCamera.isActive() ) {
		return mFreeCamera.camera();
	}
#endif
	return mControl.camera();
}

bool GameScene::isFreeCameraActive() const {
#ifdef ZF_EDITOR
	return mFreeCamera.isActive();
#else
	return false;
#endif
}

void GameScene::shakeOnHeavyHits() {
	typedef CombatResolver::ZombieHit::Weight Weight;
	for ( const CombatResolver::ZombieHit& hit : mWorld.combat().zombieHitsThisTick() ) {
		if ( hit.weight == Weight::Severed || hit.weight == Weight::HeadBurst ) {
			mControl.shakeOnHeavyHit( hit.weight == Weight::HeadBurst );
		}
	}
}

void GameScene::updatePlayerDamage() {
	bool hit = false;
	for ( const CombatResolver::ZombieAttackEvent& event : mWorld.combat().zombieAttacksThisTick() ) {
		switch ( event.kind ) {
			case CombatResolver::ZombieAttackEvent::Kind::Grab:
				mControl.lookToward( event.point ); // つかんだゾンビの顔を見る（その後はマウスで自由に回せる）
				mControl.shakeOnGrab();
				break;
			case CombatResolver::ZombieAttackEvent::Kind::Hit:
				mControl.flinch(); // 狙いが跳ね、絵が大きく揺れる
				hit = true;
				break;
		}
	}
	mCombatSounds.playAttacks( mWorld.combat() );
	mControl.updateLookPull(); // 揺れ・反動をカメラへ渡す updateViewEffects より前に
	const GameObject* player = mWorld.player();
	const float kFullHealthRatio = 1.f; // 自キャラがいないときは赤くしない
	const PlayerHealthComponent* healthComponent = player ? player->getComponent< PlayerHealthComponent >() : nullptr; // 外せる部品
	const PlayerHealth* health = healthComponent ? &healthComponent->health() : nullptr;
	float healthRatio = health ? health->current() / health->maxHealth() : kFullHealthRatio;
	mDamageView.update( hit, healthRatio, mWorld.isPlayerDead() );
}

void GameScene::updateRaid() {
	const GameObject* player = mWorld.player();
	if ( player ) {
		mSession.tick( player->getComponent< TransformComponent >()->position(), mWorld.isPlayerDead(), mWorld.combat().zombiesKilledThisTick() );
	}
	switch ( mSession.ending() ) {
	case RaidEnding::Survived:
	case RaidEnding::Missing:
		finishRaid();
		break;
	case RaidEnding::Killed:
		if ( mDamageView.isFadedOut() ) {
			finishRaid(); // 崩れて真っ暗になるのを見せてから
		}
		break;
	default:
		break;
	}
}

void GameScene::finishRaid() {
	mRaidFinished = true;
	RunState& run = mResources->run();
	DayResult result = mSession.result();
	if ( result.ending == RaidEnding::Survived ) {
		storeMagazineRounds();
		RunRules::finishDay( &run, result, mInventory.grid() );
		SceneManager::instance().requestChange( SceneManager::SCENE_DAY_RESULT );
	} else {
		RunRules::failDay( &run, result );
		SceneManager::instance().requestChange( SceneManager::SCENE_RUN_END );
	}
}

void GameScene::storeMagazineRounds() {
	const ItemGrid& grid = mInventory.grid();
	for ( int cell = 0; cell < grid.cellCount(); ++cell ) {
		if ( grid.ownerCell( cell ) != cell ) {
			continue;
		}
		ItemId item = grid.stackAt( cell ).item;
		if ( weaponForItem( item ) != WEAPON_NONE ) {
			mInventory.setLoadedRounds( cell, mWorld.magazineRounds( item ) );
		}
	}
}

void GameScene::restoreMagazineRounds() {
	for ( const ItemStack& stack : mInventory.grid().stacks() ) {
		if ( weaponForItem( stack.item ) != WEAPON_NONE ) {
			mWorld.setMagazineRounds( stack.item, stack.loadedRounds );
		}
	}
}

