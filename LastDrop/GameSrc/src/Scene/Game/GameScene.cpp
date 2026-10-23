#include "GameSrc/Scene/Game/GameScene.h"
#include "GameSrc/Scene/GameResources.h"
#include "GameSrc/Player/PlayerCharacter.h"
#include "GameSrc/Core/Pad.h"
#include "GameSrc/Scene/SceneManager.h"
#include "GameSrc/Render/ViewFrustum.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/Enum.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Functions.h"
#include "GameLib/WindowCreator/WindowCreator.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const float kInteractReach = 2.5f; // 手が届く距離（m）。足元の缶を立ったまま拾える長さ

	// ── ライティング ──
	// キャラが歩き回っても明るさが変わらないよう、太陽のような遠い光源にする。
	// 強さに距離を入れると、距離による減衰が打ち消されて一定の明るさになる。
	const Vector3 kSunPosition( 300.f, 500.f, 400.f );
	const Vector3 kSunColor( 1.f, 0.95f, 0.9f );
	const Vector3 kFillPosition( -300.f, 200.f, -400.f ); // 影側を真っ黒にしない補助光
	const Vector3 kFillColor( 0.35f, 0.35f, 0.4f );
	const Vector3 kAmbientColor( 0.4f, 0.4f, 0.4f );
	const int kSunLight = 0;
	const int kFillLight = 1;
	const int kMuzzleLight = 2; // 撃った瞬間だけ銃口の前に灯す（MuzzleFlash）。光っていない間は強さ0
	const int kLightNumber = 4; // GameLib のライト数。使わないライトは強さ0にする

#ifdef _DEBUG
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
	mWorld( &resources->meshes() ),
	mInteractor( &mWorld.hitWorld(), kInteractReach, [ this ]( const HitWorld::Hit& hit ) { return mWorld.resolveInteractable( hit ); } ),
	mControl( mWorld.map().playerStart().yawDegree ), // キャラの向いている方を見始める
	mBodySounds( &resources->audio(), &mWorld.ground(), mWorld.map().groundZones() ),
	mZombieHitSounds( &resources->audio() ),
	mZombieAttackSounds( &resources->audio() ),
	mHud( &resources->uiRenderer() ),
	mPauseMenu( &resources->uiRenderer(), [ this ]() { resume(); } ),
	mReady( false ),
	mPaused( false ) {
}

GameScene::~GameScene() {
	if ( mPaused ) {
		mResources->audio().setPaused( false );
	}
}

void GameScene::pause() {
	mPaused = true;
	mResources->audio().setPaused( true );
	mControl.clearInput();   // 押していた移動・撃つを放す（戻った後も、離すまで撃たない）
	mControl.unlockCursor(); // ボタンをマウスで押せるように
}

void GameScene::resume() {
	mPaused = false;
	mResources->audio().setPaused( false );
	mControl.lockCursor();
}

void GameScene::handlePausedInput() {
	if ( InputMenu() ) {
		resume();
		return;
	}
	mPauseMenu.handleInput(); // Resume が押されたら、この中で resume が呼ばれる
}

void GameScene::beginFrame() {
	// 読み込み・構築・テクスチャのロードを進める（構築後もテクスチャのために毎回呼ぶ）。
	// 2回目以降のこのシーン（Retry）では、もう構築済みなのですぐに並べられる。
	bool allBuilt = mResources->meshes().update();
	if ( !mReady ) {
		if ( !allBuilt ) {
			return; // ロード完了待ち
		}
		mWorld.place();
		mControl.lockCursor(); // 始めから視点を回せるようにロックしておく
		mReady = true;
	}

	// 止めている間は、メニューの操作だけを読む（視点・移動・使う・確認用のキーは読まない）。
	if ( mPaused ) {
		handlePausedInput();
		return;
	}
	// ESC か、窓が裏に回ったら止める（裏に回っている間に世界が進んで、戻ったら噛まれていた、とならないように）。
	if ( InputMenu() || !GameLib::WindowCreator().isActive() ) {
		pause();
		return;
	}

	// デバッグ用。F5 で自キャラの無敵を切り替える（押した瞬間。Debug ビルドだけで効く）。
	if (InputDebugInvincible()) {
		PlayerCharacter* player = mWorld.player();
		if (player) {
			player->toggleInvincible();
		}
	}

#ifdef _DEBUG
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
#ifdef _DEBUG
	mFreeCamera.look(); // 使っていなければ何もしない。カーソルロックの切り替え（readInputIdle）の後に読む
	mDebug.handleInput();
	if ( InputDebugSpawnZombie() ) {
		spawnDebugZombies();
	}
#endif
}

#ifdef _DEBUG
void GameScene::spawnDebugZombies() {
	const PlayerCharacter* player = mWorld.player();
	if ( !player ) {
		return;
	}
	// 視点の水平の向きの前と右（PlayerCharacter の cameraForward / cameraRight と同じ式）。間に建物があっても調べない（中に出ることがある）。
	float yaw = mControl.camera().yaw();
	Vector3 forward( Math::sin( yaw ), 0.f, Math::cos( yaw ) );
	Vector3 right( -Math::cos( yaw ), 0.f, Math::sin( yaw ) );
	Vector3 center;
	center.setMadd( player->position(), forward, kDebugSpawnDistance );
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
#endif

void GameScene::update() {
	if ( !mReady || mPaused ) {
		return; // 止めている間は世界を進めない（ティックの数は数えられるが、何もしないので戻った瞬間に早送りにならない）
	}
	// 順番に意味がある: 向きを決める（beginFrame）→ その向きでキャラを動かし体を曲げる → 動いた後の頭の位置へ視点を置く（draw）。
	// 逆にすると W が1フレーム前の向きへの前進になり、視点も体に1フレーム遅れて、腕や銃が画面の中で揺れて見える。
	const FirstPersonCamera& camera = mControl.camera();
	mWorld.update( mControl.command(), camera.yaw(), camera.pitch(), camera.viewDirection(), &mInventory );
	mControl.consumePressed(); // 使ったので消す（同じ描画の間の次の update で、もう一度反応しないように）
	mZombieHitSounds.play( mWorld.zombieHitsThisTick() ); // このティックに当たった分だけ鳴らす（次の update で入れ替わるので、ここで読む）
	updatePlayerDamage();
	const PlayerCharacter* player = mWorld.player();
	// 撃ったティックの弾は跳ねる前の向きへ飛び、跳ねた視点は次のティックから効く（1発目は狙った所へ飛ぶ）。
	// 噛まれた怯み・揺れ（updatePlayerDamage で受けた分）もここでカメラへ渡す。
	mControl.updateViewEffects( player && player->firedThisTick() );
	if ( player ) {
		mBodySounds.update( *player );
	}
#ifdef _DEBUG
	mFreeCamera.move(); // 使っていなければ何もしない
#endif
}

void GameScene::draw() {
	if ( !mReady ) {
		return;
	}
	mControl.setEyeDrop( mDamageView.eyeDrop() ); // 死んだ後に膝から崩れる分だけ、目を下げて置く
	const PlayerCharacter* player = mWorld.player();
	if ( player ) {
		Vector3 eye = player->eyePosition();
		mControl.placeCamera( &eye, aimWeight() );
	} else {
		mControl.placeCamera( nullptr, aimWeight() );
	}
#ifdef _DEBUG
	// 自キャラの視点は、観察用カメラの間も目へ置き続ける（見ている物を使う視線と、戻ったときの位置のため）。
	// 描くのは観察用カメラから（自キャラの視点で設定した描画ステートを上書きする）。
	if ( mFreeCamera.isActive() ) {
		mFreeCamera.apply();
	}
#endif
	const FirstPersonCamera& camera = viewCamera();
	updateListener();
	// 噛まれた瞬間の赤み・痛み・死んだ後の暗さは、この描画の写すときに効く。
	mPostProcess.setDamageFlash( mDamageView.damageFlash() );
	mPostProcess.setPain( mDamageView.pain() );
	mPostProcess.setFade( mDamageView.fade() );
	mPostProcess.beginScene();
	setupLights();
	// 視点を置いた後の行列から映る範囲を作り、映らない地面・建物を描かない。
	mWorld.draw( ViewFrustum( camera.projectionViewMatrix() ) );
	mWorld.drawBulletHoles( camera.projectionViewMatrix() ); // 壁・地面を描いた後（奥行きで隠すため）
	mWorld.drawImpactDust( camera.projectionViewMatrix(), camera.viewDirection() ); // 弾痕の上に舞う
	mWorld.drawBloodSpray( camera.projectionViewMatrix(), camera.viewDirection() ); // 砂ぼこりの後（同じ作り）
	mWorld.drawMuzzleFlash( camera.projectionViewMatrix(), camera.viewDirection() ); // 加算なので順番は問わない
#ifdef _DEBUG
	mDebug.drawCollision( mWorld, camera );
#endif
	mPostProcess.endScene();
	// UiRenderer は 1 回の描画に 1 回しか描けない（中の PrimitiveRenderer が 2 回目の draw で ASSERT）。
	// 止めている間は HUD を出さず、メニューがまとめて描く（2026-10-23。HUD とメニューの両方が描いて止まっていた）。
	if ( mPaused ) {
		mPauseMenu.draw();
	} else {
#ifdef _DEBUG
		// 積むだけ。すぐ下の HUD の描画（UiRenderer::draw）でまとめて描かれる。
		mDebug.addZombieHealth( mWorld, camera, &mResources->uiRenderer() );
		mDebug.addPlayerHealth( mWorld, &mResources->uiRenderer() );
#endif
		mHud.draw( mInteractor.prompt().c_str(), mWorld.escapeCar(), mInventory, player ? &player->health() : nullptr );
	}

	GameDebugOverlay::TextState text;
	text.cursorLocked = mControl.isCursorLocked();
	text.floorMaterial = nullptr;
#ifdef _DEBUG
	// 足元の材質は地面へ線分を撃って調べるので、毎回の描画で引くのは Debug ビルドの確認用の表示だけにする。
	if ( player ) {
		text.floorMaterial = groundMaterialName( mBodySounds.materialAt( player->position() ) );
	}
#endif
	// 画面に出せるのは1行なので、写す処理（PostProcess）→ キャラ（SkinningPipeline）の順に、先に見つかった方を出す。
	text.shaderError = mPostProcess.errorMessage();
	if ( text.shaderError[ 0 ] == '\0' ) {
		text.shaderError = mResources->meshes().skinning().errorMessage();
	}
	mDebug.drawText( text );
}

void GameScene::setupLights() const {
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.enableDepthTest( true );
	gm.enableDepthWrite( true );
	gm.setCullMode( Graphics::CULL_NONE ); // 裏返っても見えるように両面描画
	gm.setAmbientColor( kAmbientColor );
	gm.setLightColor( kSunLight, kSunColor );
	gm.setLightPosition( kSunLight, kSunPosition );
	gm.setLightIntensity( kSunLight, kSunPosition.length() );
	gm.setLightColor( kFillLight, kFillColor );
	gm.setLightPosition( kFillLight, kFillPosition );
	gm.setLightIntensity( kFillLight, kFillPosition.length() );
	MuzzleFlash::Light muzzle;
	if ( mWorld.muzzleLight( &muzzle ) ) {
		gm.setLightColor( kMuzzleLight, muzzle.color );
		gm.setLightPosition( kMuzzleLight, muzzle.position );
		gm.setLightIntensity( kMuzzleLight, muzzle.intensity );
	} else {
		gm.setLightIntensity( kMuzzleLight, 0.f );
	}
	for ( int i = kMuzzleLight + 1; i < kLightNumber; ++i ) {
		gm.setLightIntensity( i, 0.f );
	}
}

void GameScene::updateInteraction() {
	PlayerCharacter* player = mWorld.player();
	if ( !player ) {
		return;
	}
	// 視線は前回の描画で置いたカメラから撃つ（目の位置・向きは draw で決まるため）。
	const FirstPersonCamera& camera = mControl.camera();
	// 観察用カメラの間は E で使わない（自キャラの見ている物が、観察している画面からは分からないため）。案内は今まで通り出す。
	mInteractor.update( camera.eyePosition(), camera.viewDirection(), &mInventory, InputInteract() && !isFreeCameraActive() && !mWorld.isPlayerDead() );
	// 拾われた物は印を付けるだけ。実際に消えるのはティックの最後（GameWorld::update）。
	mWorld.removeTakenPickups();
	if ( mWorld.isEscapeReady() ) {
		SceneManager::instance().requestChange( SceneManager::SCENE_CLEAR );
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
	const PlayerCharacter* player = mWorld.player();
	return player ? player->aimWeight() : 0.f;
}

const FirstPersonCamera& GameScene::viewCamera() const {
#ifdef _DEBUG
	if ( mFreeCamera.isActive() ) {
		return mFreeCamera.camera();
	}
#endif
	return mControl.camera();
}

bool GameScene::isFreeCameraActive() const {
#ifdef _DEBUG
	return mFreeCamera.isActive();
#else
	return false;
#endif
}

void GameScene::updatePlayerDamage() {
	bool bitten = false;
	for ( const GameWorld::ZombieAttackEvent& event : mWorld.zombieAttacksThisTick() ) {
		if ( event.kind == GameWorld::ZombieAttackEvent::Kind::Grab ) {
			mControl.lookToward( event.point ); // つかんだゾンビの顔を見る（その後はマウスで自由に回せる）
			mControl.shakeOnGrab();
		} else {
			mControl.flinch(); // 狙いが跳ね、絵が大きく揺れる
			bitten = true;
		}
	}
	mZombieAttackSounds.play( mWorld.zombieAttacksThisTick() );
	mControl.updateLookPull();
	mDamageView.update( bitten, mWorld.isPlayerDead() );
	if ( mDamageView.isFadedOut() ) {
		SceneManager::instance().requestChange( SceneManager::SCENE_GAME_OVER );
	}
}
