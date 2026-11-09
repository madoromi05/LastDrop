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
#include "GameSrc/Raid/RaidStart.h"
#include "GameSrc/Raid/RaidParams.h"
#include "GameSrc/Weapon/WeaponCatalog.h"
#include "GameSrc/Interaction/IInteractable.h"
#include "GameSrc/Interaction/Item/MagazineSync.h"
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
	// カバンの画面の見出しの下に出す、今回の脱出の車の場所（FuelEscapeLoading_Plan.txt Q14）。
	const char* kExitsPrefix = "脱出の車：";
	const char* kExitsSeparator = " / ";
	const float kNoSearch = -1.f;        // HUD に「探している」の棒を出さない（RaidHud::draw。漁り/給油の本体は LootController）
}

GameScene::GameScene( GameResources* resources ) :
	mResources( resources ),
	// 今回の条件（出撃のたびにランダム。F6 で固定していればそれ。SimpleRun_Plan.txt Q5・C9）。世界が持つ（F6 で今の出撃のまま替えられる）。
	mWorld( &resources->meshes(), &resources->town(), resources->raidRecord().chooseCondition( Framework::instance().time() ) ),
	mSceneRenderer( &mWorld ), // 世界を画面へ写すディファードのパイプライン一式（mWorld を読むので、その後に並べる）
	mInventory( RaidStart::startBag() ), // 毎回同じ始めのカバン（拳銃と 9mm の弾。SimpleRun_Plan.txt Q6・C4）
	mInteractor( kInteractReach, [ this ]( const Vector3& origin, const Vector3& delta ) { return mWorld.interaction().findInteractable( origin, delta, mWorld.playerTarget() ); } ),
	// 脱出の車の置き場所は出撃のたびに変える（起動からの時間を種にする。ガソリンを入れる箱と同じ）。
	mSession( mWorld.town().map().escapeCarSpots(), Framework::instance().time() ),
	// キャラの向いている方（その日の車の方）を見始める
	mControl( mWorld.town().map().escapeCarSpots()[ mSession.escapeCarSpot() ].playerStart.yawDegree, &resources->settings() ),
	mFeedback( &resources->audio(), &mWorld.town() ), // 出来事を自キャラの視点・音・画面へ返す
	mHud( &mWorld, &mInventory, &resources->uiRenderer() ), // 遊び中の画面表示（棒・弾数・マーク）
	// 出撃のロード段階（mSession が選んだ脱出の車の置き場所を使うので、その後に並べる）
	mLoad( &mWorld, &resources->meshes(), mSession.escapeCarSpot(), &resources->uiRenderer() ),
	mPauseMenu( &resources->uiRenderer(), &resources->settings(), &resources->audio(), [ this ]() { resume(); } ),
	mModals( &resources->audio(), &mControl ), // 音とカーソルを借りて、モーダルの開閉の共通遷移を受け持つ
	mBag( &mInventory, &mWorld, &mControl, &mModals, &resources->uiRenderer() ), // カバンの開閉と操作
	// 漁れる場所・給油（mWorld の脱出の車の候補を使う mSession を借りるので、その後に並べる）
	mLoot( &mWorld, &mInventory, &resources->audio(), &mModals, &mSession, &resources->uiRenderer() ),
	mRaidFinished( false ) {
}

GameScene::~GameScene() {
	// 開けている音・エンジンの音は mLoot の破棄で止まる（音の道具は次のシーンも使う）。
	if ( mModals.isOpen() ) {
		mResources->audio().setPaused( false );
	}
	// エディタへ渡した物は mEditor の破棄で外れる（世界より先に破棄される）。
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

ItemId GameScene::equippedWeapon() const {
	const GunComponent* gun = HoldComponent::heldGun( mWorld.player() );
	return gun ? gun->spec().item : ITEM_NONE;
}

void GameScene::beginFrame() {
	// 読み込み・構築・テクスチャのロードを進める（構築後もテクスチャのために毎回呼ぶ）。
	// 2回目以降のこのシーン（もう一度・Retry の出撃）では、もう構築済みなのですぐに並べられる。
	bool allReady = false;
	{
		PROFILE_SCOPE( "Game.meshes" );
		allReady = mResources->meshes().update();
	}
	if ( !mLoad.isReady() ) {
		// ロードを 1 段階進める。完了したフレームだけ後処理（弾倉の復元・カーソルのロック）を 1 回行う。
		if ( !mLoad.advance( allReady ) ) {
			return;
		}
		restoreMagazineRounds( mInventory, mWorld ); // 銃の物を作った後に、始めのカバンの弾倉の数を入れる
		mControl.lockCursor(); // 始めから視点を回せるようにロックしておく
#ifdef ZF_EDITOR
		mEditor.markReady();
#endif
	}
	{
		PROFILE_SCOPE( "Game.input" );
		handleFrameInput();
	}
#ifdef ZF_EDITOR
	mEditor.probeFrame( mWorld, mControl, mModals, isFreeCameraActive() ); // 撃つボタンの記録（FireProbe）
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
		mBag.handleInput();
		return;
	}
	// 中を見る画面もカバンと同じ（R3_Plan.txt Q8）。
	if ( mModals.isLoot() ) {
		mLoot.handleInput();
		return;
	}
	// ESC か、窓が裏に回ったら止める（裏に回っている間に世界が進んで、戻ったら攻撃されていた、とならないように）。
	// 死んだ後は止めない（一時停止からやめると、その日をやり直せるので、死んでから逃げられないように。R2_Plan.txt Q3）。
	if ( ( InputMenu() || !GameLib::WindowCreator().isActive() ) && !mWorld.isPlayerDead() ) {
		pause();
		return;
	}
	// カバンは自キャラが生きていて、観察用カメラを使っていない間だけ開く（canOpen は生存だけ見るので、ここで free-cam を足す）。
	if ( InputInventory() && !isFreeCameraActive() && mBag.canOpen() ) {
		mLoot.cancelSearch(); // 開けている途中にカバンを開いたら、開けるのはやめる（閉じた後に勝手に開かないように）
		mBag.open();
		return;
	}
#ifdef ZF_EDITOR
	// 観察用カメラの切り替え（F6 の頼み）を先に行う（この描画のマウスとキーを、自キャラと観察用カメラのどちらが使うか決めるため）。
	mFreeCamera.handleToggle( mControl.camera() );
	// F6 で頼まれた無敵の切り替えを行い、F6 のエディタへ今の物を渡す。
	mEditor.publish( &mWorld, &mInventory, &mSession, &mResources->raidRecord(), isFreeCameraActive() );
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
	mEditor.spawnRequestedZombies( &mWorld, mControl.camera() ); // F6 の Spawn zombie
#endif
}

void GameScene::update() {
	if ( !mLoad.isReady() || mModals.isOpen() || mRaidFinished ) {
		return; // 出撃を終えた後と、止めている間（一時停止・カバン・中を見る画面）は世界を進めない（ティックの数は数えられるが、何もしないので戻った瞬間に早送りにならない）
	}
	// 開けている途中の取りやめは、このティックの操作で決める（押した瞬間の操作は、世界を進めた後に消えるので先に読む）。
	bool commandInterruptsSearch = LootController::interrupts( mControl.command() );
	// 順番に意味がある: 向きを決める（beginFrame）→ その向きでキャラを動かし体を曲げる → 動いた後の頭の位置へ視点を置く（draw）。
	// 逆にすると W が1フレーム前の向きへの前進になり、視点も体に1フレーム遅れて、腕や銃が画面の中で揺れて見える。
	const FirstPersonCamera& camera = mControl.camera();
	// 弾は照準のぶれを足した向き（銃が向いている所）へ飛ぶ。画面の中央（viewDirection）からは、ぶれの分だけずれる。
	mWorld.update( mControl.command(), camera.yaw(), camera.pitch(), camera.weaponDirection(), &mInventory );
	mControl.consumePressed(); // 使ったので消す（同じ描画の間の次の update で、もう一度反応しないように）
	// 撃った手応え（当たった音・強い命中の揺れ）と、攻撃された反応（顔を向ける・怯み・音・赤み）。計測の印は中で Game.hitFeedback・Game.damage。
	mFeedback.reactToCombat( mWorld, &mControl );
	{
		PROFILE_SCOPE( "Game.raid" );
		updateRaid();
	}
	{
		PROFILE_SCOPE( "Game.loot" );
		mLoot.update( commandInterruptsSearch, mRaidFinished ); // 攻撃された・死んだ・出撃が終わったも読むので、その後に
	}
	if ( mModals.isLoot() ) {
		return; // 開け終わって中を見る画面を開いた（世界を止めたので、同じ描画の残りのティックも進めない）
	}
	PROFILE_SCOPE( "Game.view" ); // ここから update の終わりまで
#ifdef ZF_EDITOR
	mEditor.probeTick( mWorld ); // 撃った・空撃ちしたティック（FireProbe）
#endif
	// 反動とぶれをカメラと体へ・右下の弾数を出すか・体の音（反動・揺れ・ぶれは次のティックの絵から効く）。
	mFeedback.syncView( mWorld, &mControl, &mHud );
#ifdef ZF_EDITOR
	mFreeCamera.move(); // 使っていなければ何もしない
#endif
}

bool GameScene::isLoading() const {
	return !mLoad.isReady();
}

void GameScene::draw() {
	if ( !mLoad.isReady() ) {
		// ロード中は世界を描かず、ロード画面だけ（FuelEscapeLoading_Plan.txt 6 章）。
		mLoad.drawLoadingScreen( mWorld.dayCondition().title );
		return;
	}
	mControl.setEyeDrop( mFeedback.damageView().eyeDrop() ); // 死んだ後に膝から崩れる分だけ、目を下げて置く
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
	const PlayerDamageView& damageView = mFeedback.damageView();
	SceneRenderer::DamagePostEffect damage = { damageView.damageFlash(), damageView.pain(), damageView.fade() };
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
		mBag.draw( exits.c_str() ); // カバンの画面（HUD は描かない）
	} else if ( mModals.isLoot() && mLoot.hasOpenGrid() ) {
		mLoot.draw( equippedWeapon() );
	} else {
#ifdef ZF_EDITOR
		// 積むだけ。すぐ下の HUD の描画（UiRenderer::draw）でまとめて描かれる。
		mDebug.addZombieHealth( mWorld, camera, &mResources->uiRenderer() );
		mDebug.addPlayerHealth( mWorld, &mResources->uiRenderer() );
		mDebug.addLootContents( mWorld, camera, &mResources->uiRenderer() );
#endif
		// 遊んでいる間の HUD（マーク・弾数・知らせと棒）。積むだけで、下の UiRenderer::draw がまとめて描く。
		mHud.draw( camera, mSession, mWorld.dayCondition(), mLoot.isSearching() ? mLoot.progress() : kNoSearch, mLoot.waitingKind() );
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
		mHud.clearMarkers();
		return;
	}
	// 視線は前回の描画で置いたカメラから撃つ（目の位置・向きは draw で決まるため）。
	const FirstPersonCamera& camera = mControl.camera();
	// 観察用カメラの間は E で使わない（自キャラの見ている物が、観察している画面からは分からないため）。マークは今まで通り出す。
	bool interactPressed = InputInteract() && !isFreeCameraActive() && !mWorld.isPlayerDead();
	if ( interactPressed && mLoot.isSearching() ) {
		mLoot.cancelSearch(); // 開けている途中に E をもう一度押したら取りやめ（R3_Plan.txt Q6）
		interactPressed = false;
	}
	mInteractor.update( camera.eyePosition(), camera.viewDirection(), &mInventory, interactPressed );
	// 拾う物は、カバンを開いて入れるマスを選ぶ（InventoryPickup_Plan.txt Q3）。拾い終えた物は次の描画の後片付けで消える。
	if ( IInteractable* offer = mInteractor.offer() ) {
		mBag.startPlacing( offer );
	}
	// 漁れる場所は、開けるまで待ってから中を見る画面を開く（R3_Plan.txt 5 章）。
	if ( IInteractable* opened = mInteractor.opened() ) {
		mLoot.beginLoot( opened );
	}
	// 拾われた物は印を付けるだけ。実際に消えるのはティックの最後（GameWorld::update）。
	mWorld.interaction().removeTakenPickups();
	// マークは使った後の状態で決める（拾い切った物は出さない）。死んだ後は出さない。
	if ( mWorld.isPlayerDead() ) {
		mHud.clearMarkers();
	} else {
		mHud.updateMarkers( camera.eyePosition(), mInteractor.target() );
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

void GameScene::updateRaid() {
	const GameObject* player = mWorld.player();
	if ( player ) {
		mSession.tick( player->getComponent< TransformComponent >()->position(), mWorld.isPlayerDead(), mWorld.combat().zombiesKilledThisTick() );
	}
	switch ( mSession.ending() ) {
	case RaidEnding::Escaped:
		finishRaid();
		break;
	case RaidEnding::Killed:
		if ( mFeedback.damageView().isFadedOut() ) {
			finishRaid(); // 崩れて真っ暗になるのを見せてから
		}
		break;
	default:
		break;
	}
}

void GameScene::finishRaid() {
	mRaidFinished = true;
	// 持ち帰る物は無い（SimpleRun_Plan.txt Q7・Q10）ので、カバンは捨てて結果だけを渡す。
	mResources->raidRecord().setLastResult( mSession.result() );
	SceneManager::instance().requestChange( SceneManager::SCENE_RAID_END );
}

