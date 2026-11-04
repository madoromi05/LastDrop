#include "GameSrc/Scene/Game/GameScene.h"
#include "GameSrc/Scene/GameResources.h"
#include "GameSrc/Player/PlayerComponent.h"
#include "GameSrc/Player/FirstPersonBodyComponent.h"
#include "GameSrc/Player/HoldComponent.h"
#include "GameSrc/Player/PlayerHealthComponent.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/LightComponent.h"
#include "GameSrc/Weapon/GunComponent.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/Core/Pad.h"
#include "GameSrc/Scene/SceneManager.h"
#include "GameSrc/Render/DrawCulling.h"
#include "GameSrc/Render/LightingParams.h"
#include "GameSrc/Render/ShadowParams.h"
#include "GameSrc/Render/SsrParams.h"
#include "GameSrc/Render/PostProcessParams.h"
#include "GameSrc/DebugMode/EditorWindow.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/Enum.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Functions.h"
#include "GameLib/WindowCreator/WindowCreator.h"
#include "GameLib/Profiling/Profiler.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const float kInteractReach = 2.5f; // 手が届く距離（m）。足元の缶を立ったまま拾える長さ
	const float kNoShotRecoil = 0.f;   // 撃っていないティックの反動の大きさ（FirstPersonControl::updateViewEffects）

	// ── 影（Shadow_Plan.txt 5 章）──
	const int kFlashlightShadowSize = 1024; // 懐中電灯の影のマップの一辺（画素）
	const int kFlashlightShadowIndex = 0;   // ライトの一覧の影のマップの番号（1〜3 は月の CSM に空けておく）
	const bool kPerspectiveShadow = true;
	const bool kWithoutPlayer = false;      // 懐中電灯の影には自キャラを描かない（銃の影が画面の真ん中を埋めるため。Q4）
	// ── 月の影（CSM。Shadow_Plan.txt 6 章）──
	const int kMoonShadowSize = 2048;       // 1 枚の影のマップの一辺（画素。Q3）
	const int kMoonShadowFirstIndex = 1;    // ライトの一覧の影のマップの番号（1〜3 の 3 枚）
	const bool kOrthographicShadow = false;
	const bool kWithPlayer = true;          // 月の影には自キャラも描く（足元に自分の影が見える。Q4）
	const int kEveryFrame = 1;              // 月の影の手前の枚は毎フレーム描き直す（遅れると足元の影がずれて見える）
	const float kRoundToNearest = 0.5f;     // 調整値（float）の間隔を一番近い整数のフレームにする
	const float kMinCascadeGap = 0.5f;      // F6 で端を逆に並べても枚の範囲が 0 にならないよう、前の端よりこれだけ奥へ揃える（m）
	const float kNoDrawDistance = 0.f;      // 霧を切っている間は、距離で間引かない（DrawCulling::forCamera）
	const int kFirstCoarseLodCascade = 1;   // 月の影のこの枚から、置物を一番粗い段（LOD）で描く

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
	mWorld( &resources->meshes() ),
	mRenderer( &mWorld, &mWorld.effects() ),
	mInteractor( kInteractReach, [ this ]( const Vector3& origin, const Vector3& delta ) { return mWorld.interaction().findInteractable( origin, delta, mWorld.playerTarget() ); } ),
	mControl( mWorld.map().playerStart().yawDegree, &resources->settings() ), // キャラの向いている方を見始める
	mBodySounds( &resources->audio(), &mWorld.ground(), mWorld.map().groundZones() ),
	mZombieHitSounds( &resources->audio() ),
	mSurfaceHitSounds( &resources->audio() ),
	mZombieAttackSounds( &resources->audio() ),
	mHud( &resources->uiRenderer() ),
	mPauseMenu( &resources->uiRenderer(), &resources->settings(), &resources->audio(), [ this ]() { resume(); } ),
	mInventoryScreen( &resources->uiRenderer() ),
	mFlashlightShadow( kFlashlightShadowSize ),
	mFlashlightCastsShadow( false ),
	mMoonShadow( kMoonShadowSize ),
	mMoonCastsShadow( false ),
	mReady( false ),
	mPaused( false ),
	mBagOpen( false ) {
}

GameScene::~GameScene() {
	if ( mPaused || mBagOpen ) {
		mResources->audio().setPaused( false );
	}
	EditorWindow::setPlayerInvincible( false ); // 自キャラがいなくなるので、ON のまま残さない
	EditorWindow::setObjects( nullptr ); // 世界が消えるので、消えた物を指し続けないように
	EditorWindow::setInventory( nullptr ); // カバンも同じ
}

void GameScene::pause() {
	mPaused = true;
	mPauseMenu.open(); // 前に設定のページで閉じていても、最初のページから
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
	mBagOpen = true;
	mInventoryScreen.open();
	mResources->audio().setPaused( true );
	mControl.clearInput();   // 押していた移動・撃つを放す（一時停止と同じ）
	mControl.unlockCursor(); // マスをマウスで選べるように
}

void GameScene::closeBag() {
	mBagOpen = false;
	mResources->audio().setPaused( false );
	mControl.lockCursor();
}

void GameScene::handleBagInput() {
	// Tab でも ESC でも閉じる（ESC で一時停止のメニューは開かない。GridInventory_Plan.txt Q8）。
	// ESC はメニュー・動かしている途中なら、まずそれを取りやめる。
	if ( InputInventory() ) {
		closeBag();
		return;
	}
	if ( InputMenu() ) {
		if ( !mInventoryScreen.back() ) {
			closeBag();
		}
		return;
	}
	InventoryScreen::Request request = mInventoryScreen.handleInput( &mInventory, equippedWeapon() );
	switch ( request.kind ) {
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

ItemId GameScene::equippedWeapon() const {
	const GunComponent* gun = HoldComponent::heldGun( mWorld.player() );
	return gun ? gun->spec().item : ITEM_NONE;
}

void GameScene::beginFrame() {
	// 読み込み・構築・テクスチャのロードを進める（構築後もテクスチャのために毎回呼ぶ）。
	// 2回目以降のこのシーン（Retry）では、もう構築済みなのですぐに並べられる。
	bool allReady = mResources->meshes().update();
	if ( !mReady ) {
		if ( !allReady ) {
			return; // ロード完了待ち（テクスチャの展開まで。始まった直後にひっかからないように）
		}
		{
			LOAD_SCOPE( "place", "GameWorld::place" );
			mWorld.place();
		}
		mControl.lockCursor(); // 始めから視点を回せるようにロックしておく
		mReady = true;
	}

	// 止めている間は、メニューの操作だけを読む（視点・移動・使う・確認用のキーは読まない）。
	if ( mPaused ) {
		handlePausedInput();
		return;
	}
	// カバンを開いている間も同じ（ESC はカバンを閉じる方に使う。世界は止まっているので、窓が裏に回っても一時停止のメニューは重ねない）。
	if ( mBagOpen ) {
		handleBagInput();
		return;
	}
	// ESC か、窓が裏に回ったら止める（裏に回っている間に世界が進んで、戻ったら攻撃されていた、とならないように）。
	if ( InputMenu() || !GameLib::WindowCreator().isActive() ) {
		pause();
		return;
	}
	if ( InputInventory() && canOpenBag() ) {
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
#endif

void GameScene::update() {
	if ( !mReady || mPaused || mBagOpen ) {
		return; // 止めている間（一時停止・カバン）は世界を進めない（ティックの数は数えられるが、何もしないので戻った瞬間に早送りにならない）
	}
	// 順番に意味がある: 向きを決める（beginFrame）→ その向きでキャラを動かし体を曲げる → 動いた後の頭の位置へ視点を置く（draw）。
	// 逆にすると W が1フレーム前の向きへの前進になり、視点も体に1フレーム遅れて、腕や銃が画面の中で揺れて見える。
	const FirstPersonCamera& camera = mControl.camera();
	// 弾は照準のぶれを足した向き（銃が向いている所）へ飛ぶ。画面の中央（viewDirection）からは、ぶれの分だけずれる。
	mWorld.update( mControl.command(), camera.yaw(), camera.pitch(), camera.weaponDirection(), &mInventory );
	mControl.consumePressed(); // 使ったので消す（同じ描画の間の次の update で、もう一度反応しないように）
	mZombieHitSounds.play( mWorld.combat().zombieHitsThisTick() ); // このティックに当たった分だけ鳴らす（次の update で入れ替わるので、ここで読む）
	mSurfaceHitSounds.play( mWorld.combat().surfaceHitsThisTick() ); // 物に当たった分も同じ（読む決まりも同じ）
	updatePlayerDamage();
	GameObject* player = mWorld.player();
	const GunComponent* gun = HoldComponent::heldGun( player ); // 素手なら nullptr
	// 撃ったティックの弾は跳ねる前の向きへ飛び、跳ねた視点は次のティックから効く（1発目は狙った所へ飛ぶ）。
	// 攻撃された怯み・揺れ（updatePlayerDamage で受けた分）もここでカメラへ渡す。
	// 照準のぶれも同じく次のティックから効く（自キャラがいなければ既定の状態＝ぶれない）。
	// 反動の大きさは銃ごと（拳銃はライフルの半分。WeaponCatalog）。
	float shotRecoil = ( gun && gun->firedThisTick() ) ? gun->spec().recoilScale : kNoShotRecoil;
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
	// TAA のときは、描く行列を 1 画素より細かく毎フレームずらす（Taa_Plan.txt 5 章）。カメラが置いた行列を、ずらした物で上書きし、
	// 下の G-Buffer・SSAO・SSR・霧・デカール・半透明の板にも同じ行列を渡す（全部のパスが同じずれで揃う）。
	// 映る範囲（DrawCulling）と影はずらさない（1 画素より細かいずれは効かない／影は光の側から見た絵）。
	mJitter.advance( antiAliasingMode() == AntiAliasing::Taa );
	const Matrix44 projectionView = mJitter.apply( camera.projectionViewMatrix(),
		mPostProcess.sceneDepth().width(), mPostProcess.sceneDepth().height() );
	Graphics::Manager::instance().setProjectionViewMatrix( projectionView );
	{
		PROFILE_SCOPE( "Audio" );
		updateListener();
	}
	// 攻撃された瞬間の赤み・痛み・死んだ後の暗さは、この描画の写すときに効く。
	mPostProcess.setDamageFlash( mDamageView.damageFlash() );
	mPostProcess.setPain( mDamageView.pain() );
	mPostProcess.setFade( mDamageView.fade() );
	// SSAO・SSR・ディザのノイズは、TAA のときだけフレームごとに動かす（TAA が均す。Taa_Plan.txt 8 章）。TAA でないときは番号が 0 のまま。
	mPostProcess.setNoiseFrame( mJitter.frameIndex() );
	mPostProcess.beginScene();
	setupLights( camera );
	drawShadowMaps( camera );
	// 不透明な物は G-Buffer へ描き、全画面 1 回でライティングする（GBuffer_Plan.txt 5 章）。
	// フォワードへの切り替えは段階 3 の始めに消した（PbrNightLighting_Plan.txt Q1。PBR の式とライトを 2 か所に書かない）。
	const GBufferView view = gbufferView();
	const bool showsGBuffer = ( view != GBufferView::Final );
	// 映り込みの表示（SSR・SSR color）は、照らした後の色を映すので、表示中もライティングまで描く（表示は endScene の後に上から写す）。
	const bool showsReflection = ( view == GBufferView::Ssr || view == GBufferView::SsrColor );
	const bool drawsLighting = !showsGBuffer || showsReflection;
	mGBuffer.begin( mPostProcess.sceneDepth() );
	// 視点を置いた後の行列から映る範囲を作り、映らない地面・建物を描かない。
	// 動かない物 → 弾痕（デカール）→ 動く物の順（SsrDecalMaterial_Plan.txt 4 章・Q7）。デカールは画素に見えている面へ書くので、
	// 後から描くキャラがデカールを上書きして、キャラには弾痕が乗らない。
	{
		PROFILE_SCOPE( "World.draw" );
		const DrawCulling culling = cameraCulling( camera );
		mRenderer.drawStatic( culling, camera.eyePosition() );
		mGBuffer.beginDecals( mPostProcess.sceneDepth() );
		mRenderer.drawDecals( projectionView );
		Profiling::gpuMark( "Decals" );
		mGBuffer.endDecals( mPostProcess.sceneDepth() );
		// 動く物は動きの種類を G-Buffer へ書き分ける（TAA が前のフレームのどこを読むかを分ける。Taa_Plan.txt 6 章）。
		mGBuffer.setMotion( MotionKind::FirstPerson );
		mRenderer.drawFirstPerson();
		mGBuffer.setMotion( MotionKind::Moving );
		mRenderer.drawCharacters( culling );
	}
	mGBuffer.end();
	// SSAO は G-Buffer の後・ライティングの前（SsaoTranslucent_Plan.txt 4 章）。G-Buffer の表示（SSAO）でも見るので、表示中も作る。
	mAmbientOcclusion.draw( mGBuffer, mPostProcess.sceneDepth(), projectionView, mJitter.frameIndex() );
	Profiling::gpuMark( "SSAO" );
	if ( drawsLighting ) {
		mPostProcess.bindSceneColor(); // 深度はシェーダから読むので、描画先からは外す
		mLights.bindShadowMaps(); // 描画先を変えると t2 以降が外れるので、付け直す（LightList::bindShadowMaps）
		mDeferredLighting.drawLighting( mGBuffer, mAmbientOcclusion.result(), mPostProcess.sceneDepth(), projectionView );
		Profiling::gpuMark( "Lighting" );
		// 映り込みはライティングの後・霧の前（照らした色を映し、映り込みにも霧が掛かる。SsrDecalMaterial_Plan.txt 4 章）。
		// 作るパスはシーンの色を読むので、自分の描画先へ描いてから、シーンの色へ付け直して混ぜる。
		if ( ssrEnabled() ) {
			mReflection.buildHiZ( mPostProcess.sceneDepth() ); // F6 の Hi-Z を切っている間は作らない（HiZSsr_Plan.txt Q5）
			Profiling::gpuMark( "Hi-Z" );
			mReflection.draw( mGBuffer, mPostProcess.sceneColor(), mPostProcess.sceneDepth(), projectionView, mJitter.frameIndex() );
			Profiling::gpuMark( "SSR" );
			mPostProcess.bindSceneColor();
			mDeferredLighting.drawReflection( mGBuffer, mAmbientOcclusion.result(), mReflection.result(), mPostProcess.sceneDepth(),
				projectionView );
		} else {
			mReflection.clear(); // 表示（SSR）が前の絵を出さないように
			Profiling::gpuMark( "Hi-Z" ); // 切っていても打つ（計測の列を揃えるため）
			Profiling::gpuMark( "SSR" );
		}
		Profiling::gpuMark( "SSR composite" );
		// 霧は半透明の板より前（板は自分で同じ霧を掛けるので、二重に掛けない。NightFinish_Plan.txt 4 章）。
		if ( fogEnabled() ) {
			mDeferredLighting.drawFog( mPostProcess.sceneDepth(), projectionView );
		}
		Profiling::gpuMark( "Fog" );
		// TAA は霧の後・半透明の板の前（板は動きが取れず、マズルフラッシュに光の尾が出るため。Taa_Plan.txt Q3）。
		// 見ているカメラが替わった（観察用カメラ F4 の切り替え）フレームは、前の履歴が別の場所の絵なので捨てる（Q7）。
		if ( &camera != mTaaCamera ) {
			mTaa.invalidate();
		}
		if ( antiAliasingMode() == AntiAliasing::Taa ) {
			mTaa.resolve( mPostProcess.sceneColor(), mGBuffer, mPostProcess.sceneDepth(), projectionView, camera.projectionViewMatrix() );
		} else {
			mTaa.invalidate(); // TAA に戻したとき、古い履歴を混ぜないように
		}
		Profiling::gpuMark( "TAA" );
	} else {
		mTaa.invalidate(); // G-Buffer の表示の間は照らした色が無いので、履歴も続かない
	}
	mTaaCamera = &camera;
	// 砂ぼこりなどの半透明の板は、G-Buffer と同じ深度で隠れるようシーンへ描く。板は深度を書かないので読み取り専用で付け、
	// 同じ深度を読んで壁・地面に刺さった所を薄くする（SsaoTranslucent_Plan.txt 7 章）。弾痕は G-Buffer のデカールになった（上）。
	mPostProcess.resumeSceneReadOnlyDepth();
	mLights.bindShadowMaps();   // 板も懐中電灯の影を受ける（同じ evaluateLight を通る）
	mRenderer.drawImpactDust( projectionView, camera.viewDirection(), camera.eyePosition() ); // 弾痕の上に舞う
	mRenderer.drawBloodSpray( projectionView, camera.viewDirection(), camera.eyePosition() ); // 砂ぼこりの後（同じ作り）
	mRenderer.drawMuzzleFlash( projectionView, camera.viewDirection() ); // 加算なので順番は問わない
	mLights.unbind(); // ライトの一覧を読むのは、ここまで（ライティングのパスと半透明の板）
#ifdef ZF_EDITOR
	mPostProcess.resumeScene(); // 当たり判定の表示は深度を書くことがあるので、書ける付け方へ戻す
	mDebug.drawCollision( mWorld, camera );
	mDebug.drawLights( mLights, camera );
#endif
	// GPU の時間の区切り（Perf の欄の GPU）。世界の物ごとの印は WorldRenderer の中（Player〜Ground）。
	// ここまでが弾痕・砂ぼこり・血・マズルフラッシュ・当たり判定の表示、次の印までが画面全体の処理（ブルームなど）。
	Profiling::gpuMark( "Effects" );
	mPostProcess.endScene();
	if ( showsGBuffer ) {
		// ToneMap を通さず中身の数のまま見るので、画面へ写した後に上から描く。
		// 影の表示（Cascade・Shadow maps）はライトの一覧の定数と影のマップを読むので、外した物をもう一度付けて描く。
		mLights.rebindConstants();
		mLights.bindShadowMaps();
		mDeferredLighting.drawView( view, mGBuffer, mAmbientOcclusion.result(), mReflection.result(), mReflection.hiZ(),
			mPostProcess.sceneDepth(), projectionView );
		mLights.unbind();
	}
	// 「PostProcess」「FXAA」の印は endScene の中で打つ。ここまでは G-Buffer の表示（F6 で選んだときだけ。ふだんはほぼ 0）。
	Profiling::gpuMark( "GBufferView" );
	// UiRenderer は 1 回の描画に 1 回しか描けない（中の PrimitiveRenderer が 2 回目の draw で ASSERT）。
	// 止めている間は HUD を出さず、メニューがまとめて描く（2026-10-23。HUD とメニューの両方が描いて止まっていた）。
	if ( mPaused ) {
		mPauseMenu.draw();
	} else if ( mBagOpen ) {
		mInventoryScreen.draw( mInventory, equippedWeapon() ); // カバンの画面も同じ（HUD は描かない）
	} else {
#ifdef ZF_EDITOR
		// 積むだけ。すぐ下の HUD の描画（UiRenderer::draw）でまとめて描かれる。
		mDebug.addZombieHealth( mWorld, camera, &mResources->uiRenderer() );
		mDebug.addPlayerHealth( mWorld, &mResources->uiRenderer() );
#endif
		// 使える物の上のマーク（積むだけ。すぐ下の HUD の描画でまとめて描かれる）。
		mInteractMarkers.add( camera, &mResources->uiRenderer() );
		// 真ん中の照準「+」は出さない（ユーザー指定 2026-10-26）。
		mHud.draw( mWorld.fuelTank() );
	}

	GameDebugOverlay::TextState text;
	text.cursorLocked = mControl.isCursorLocked();
	// 画面に出せるのは1行なので、写す処理（PostProcess）→ キャラ（SkinningPipeline）→ G-Buffer → G-Buffer の表示の順に、
	// 先に見つかったものを出す。
	const char* const shaderErrors[] = {
		mPostProcess.errorMessage(),
		mResources->meshes().skinning().errorMessage(),
		mGBuffer.errorMessage(),
		mDeferredLighting.errorMessage(),
		mAmbientOcclusion.errorMessage(),
		mReflection.errorMessage(),
		mTaa.errorMessage(),
		mFlashlightShadow.errorMessage(),
		mMoonShadow.errorMessage(),
	};
	text.shaderError = "";
	for ( const char* message : shaderErrors ) {
		if ( message[ 0 ] != '\0' ) {
			text.shaderError = message;
			break;
		}
	}
	mDebug.drawText( text );
}

void GameScene::setupLights( const FirstPersonCamera& camera ) {
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.enableDepthTest( true );
	gm.enableDepthWrite( true );
	// 既定は両面描画。メッシュは MeshMaterial::apply が材質ごとに裏を捨てる（2026-10-27）ので、これが効くのはそれ以外の物だけ。
	gm.setCullMode( Graphics::CULL_NONE );
	// GameLib の 4 灯と環境光（固定パイプライン）は使わない。照らすのはライトの一覧だけ（ライティングのパスと半透明の板が b2 から読む）。
	// 月・補助光・空・環境光は調整値（LightingParams。初期値は夜。PbrNightLighting_Plan.txt 7 章）から毎フレーム作る（F6 ですぐ効くように）。
	const LightingParams& params = lightingParams();
	mLights.clear();
	const ShadowParams& shadow = shadowParams();
	mLights.setShadowSettings( shadow.normalOffsetTexels, shadow.lightOffsetTexels, shadow.strength );
	addMoon( params, camera );
	if ( params.fillIntensity > 0.f ) {
		mLights.add( directionalLight( lightDirectionFrom( params.fillElevation, params.fillAzimuth ), params.fillColor, params.fillIntensity ) );
	}
	// 自分の手元の光（懐中電灯・マズルフラッシュ）は、街灯より先に足す（16 個を選ぶとき、目からの距離が同じなら足した順で残るため）。
	addFlashlight( params );
	LightSource muzzle;
	if ( mRenderer.muzzleLight( &muzzle ) ) {
		mLights.add( muzzle );
	}
	// 交差点の街灯。16 個を超えた分は、LightList が目から遠い物から外す（PbrNightLighting_Plan.txt 8 章）。
	if ( params.streetLightIntensity > 0.f ) {
		for ( const TownPlacement& spot : mWorld.map().streetLights() ) {
			Vector3 position( spot.x, spot.y + params.streetLightHeight, spot.z );
			mLights.add( pointLight( position, params.streetLightColor, params.streetLightIntensity, params.streetLightRange ) );
		}
	}
	EnvironmentLight environment;
	environment.skyZenithColor = params.skyZenithColor;
	environment.skyHorizonColor = params.skyHorizonColor;
	environment.groundColor = params.groundColor;
	environment.skyIntensity = params.skyIntensity;
	environment.ambientIntensity = params.ambientIntensity;
	// 霧を切っている間は 0 のまま（霧なし）。半透明の板もライトの一覧の霧の値を読むので、板の霧も一緒に消える。
	if ( fogEnabled() ) {
		environment.fogHalfDistance = params.fogHalfDistance;
		environment.fogGroundDensity = params.fogGroundDensity;
		environment.fogHeightHalf = params.fogHeightHalf;
	}
	// 月の絵（MoonStarsLut_Plan.txt 4 章）。切っている間は 0 のまま（今までの空）。月が地平線より下なら月は描かない。
	if ( skyDetailEnabled() ) {
		environment.moonDirection.setNeg( lightDirectionFrom( params.moonElevation, params.moonAzimuth ) ); // 光の進む向きの逆が、月のある方
		environment.moonColor = params.moonColor;
		environment.moonRadius = params.moonRadius;
		environment.moonDiscIntensity = ( params.moonElevation > 0.f ) ? params.moonDiscIntensity : 0.f;
		environment.moonHaloIntensity = params.moonHaloIntensity;
		environment.moonHaloFalloff = params.moonHaloFalloff;
		environment.pixelAngle = camera.fieldOfView() / static_cast< float >( gm.height() );
	}
	mLights.setEnvironment( environment );
	mLights.bind( camera.eyePosition() );
}

void GameScene::addMoon( const LightingParams& params, const FirstPersonCamera& camera ) {
	Vector3 direction = lightDirectionFrom( params.moonElevation, params.moonAzimuth );
	LightSource moon = directionalLight( direction, params.moonColor, params.moonIntensity );
	mMoonCastsShadow = shadowsEnabled() && ( params.moonIntensity > 0.f );
	if ( mMoonCastsShadow ) {
		// 枚の受け持ちの端。F6 で逆に並べても範囲が 0 にならないよう、前の端より奥へ揃える。
		const ShadowParams& shadow = shadowParams();
		float ends[ CascadedShadowMap::kCascadeCount ] = { shadow.moonCascadeEnd0, shadow.moonCascadeEnd1, shadow.moonCascadeEnd2 };
		for ( int i = 1; i < CascadedShadowMap::kCascadeCount; ++i ) {
			ends[ i ] = max( ends[ i ], ends[ i - 1 ] + kMinCascadeGap );
		}
		Graphics::Manager gm = Graphics::Manager::instance();
		float aspect = static_cast< float >( gm.width() ) / static_cast< float >( gm.height() );
		// 遠い枚ほど間を空けて描き直す（OptimizationA_Plan.txt 7 章）。手前の枚は毎フレーム、間隔は調整値を整数に丸める。
		CascadedShadowMap::Refresh refresh;
		refresh.intervals[ 0 ] = kEveryFrame;
		refresh.intervals[ 1 ] = static_cast< int >( shadow.moonCascadeInterval1 + kRoundToNearest );
		refresh.intervals[ 2 ] = static_cast< int >( shadow.moonCascadeInterval2 + kRoundToNearest );
		refresh.redrawAngleDegree = shadow.moonRedrawAngle;
		refresh.redrawDistance = shadow.moonRedrawDistance;
		mMoonShadow.update( camera.eyePosition(), camera.viewDirection(), camera.fieldOfView(), aspect, direction, ends, shadow.moonCasterMargin,
			refresh );
		for ( int i = 0; i < CascadedShadowMap::kCascadeCount; ++i ) {
			mLights.setShadowMap( kMoonShadowFirstIndex + i, mMoonShadow.map( i ).depth(), mMoonShadow.projectionView( i ),
				mMoonShadow.texelSize( i ), kOrthographicShadow );
		}
		mLights.setCascades( camera.viewDirection(), ends, shadow.moonCascadeBlend );
		moon.shadowMap = kMoonShadowFirstIndex;
		moon.shadowMapCount = CascadedShadowMap::kCascadeCount;
	} else {
		mMoonShadow.invalidate(); // 影を戻したときに、止めていた間の古い影を使わないよう全部描き直す
	}
	mLights.add( moon );
}

void GameScene::addFlashlight( const LightingParams& params ) {
	mFlashlightCastsShadow = false;
	const GameObject* player = mWorld.player();
	const HoldComponent* hold = player ? player->getComponent< HoldComponent >() : nullptr;
	const LightComponent* light = hold ? hold->light() : nullptr; // 銃に付いたライト（素手なら無い）
	Vector3 muzzlePosition;
	Vector3 muzzleDirection;
	// 銃をしまっている間は銃口が無い（muzzle が false）ので照らさない。付けた状態は自キャラが覚えている。
	if ( !light || !light->isOn() || !hold->muzzle( &muzzlePosition, &muzzleDirection ) ) {
		return;
	}
	Vector3 position;
	position.setMadd( muzzlePosition, muzzleDirection, params.flashlightOffset );
	// F6 で内側を外側より大きくしても止まらないよう、内側は外側までに抑える。
	float innerAngle = min( params.flashlightInnerAngle, params.flashlightOuterAngle );
	LightSource flashlight = spotLight( position, muzzleDirection, params.flashlightColor, params.flashlightIntensity, params.flashlightRange,
		innerAngle, params.flashlightOuterAngle );
	flashlight.nearDistance = params.flashlightNearDistance;
	// 影（Shadow_Plan.txt 5 章）。光と同じ位置・向き・角度から見た深度を、drawShadowMaps が描く。F6 で影を切っている間は付けない。
	if ( shadowsEnabled() ) {
		mFlashlightShadowMatrix = spotShadowProjectionView( position, muzzleDirection, params.flashlightOuterAngle,
			shadowParams().flashlightNearClip, params.flashlightRange );
		mLights.setShadowMap( kFlashlightShadowIndex, mFlashlightShadow.depth(), mFlashlightShadowMatrix,
			spotShadowTexelSizeAtOneMeter( params.flashlightOuterAngle, mFlashlightShadow.size() ), kPerspectiveShadow );
		flashlight.shadowMap = kFlashlightShadowIndex;
		mFlashlightCastsShadow = true;
	}
	mLights.add( flashlight );
}

void GameScene::drawShadowMaps( const FirstPersonCamera& camera ) {
	if ( mFlashlightCastsShadow ) {
		mFlashlightShadow.begin( mFlashlightShadowMatrix );
		mRenderer.drawShadowCasters( DrawCulling( mFlashlightShadowMatrix ), kWithoutPlayer );
		mFlashlightShadow.end();
	}
	if ( mMoonCastsShadow ) {
		for ( int i = 0; i < CascadedShadowMap::kCascadeCount; ++i ) {
			if ( !mMoonShadow.isDue( i ) ) {
				continue; // このフレームは描き直さない（前に描いた影と行列をそのまま読む）
			}
			const Matrix44& matrix = mMoonShadow.projectionView( i );
			mMoonShadow.map( i ).begin( matrix );
			// 2 枚目からは置物を一番粗い段で描く（OptimizationC_Plan.txt Q8。遠い枚は 1 画素が大きく、形の差が影に出ない）。
			mRenderer.drawShadowCasters( DrawCulling::forOrthographicShadow( matrix, mMoonShadow.texelSize( i ), i >= kFirstCoarseLodCascade ),
				kWithPlayer );
			mMoonShadow.map( i ).end();
			PROFILE_COUNT( "Shadow.cascades", 1 ); // 描き直した月の影の枚数（OptimizationA_Plan.txt 5 章。間引く前は毎フレーム 3、間引いた後はふだん 1〜2）
		}
	}
	// 影のマップは光の側の行列で描いたので、G-Buffer へ描く前にカメラの行列へ戻す。
	Graphics::Manager::instance().setProjectionViewMatrix( camera.projectionViewMatrix() );
	// GPU の時間の区切り。ここまでが影のマップ（懐中電灯と月の 3 枚）。
	Profiling::gpuMark( "Shadow" );
}

DrawCulling GameScene::cameraCulling( const FirstPersonCamera& camera ) const {
	float drawDistance = fogEnabled() ? DrawCulling::drawDistanceForFog( lightingParams().fogHalfDistance ) : kNoDrawDistance;
	// 小ささの画素は、G-Buffer へ描く絵（シーンの深度）の高さで数える。
	// 隠れ（PVS）は目から見る絵だけ（影の culling には渡さない。OptimizationC_Plan.txt Q5）。
	const TownVisibility* visibility = pvsEnabled() ? &mWorld.visibility() : nullptr;
	return DrawCulling::forCamera( camera.projectionViewMatrix(), camera.eyePosition(), drawDistance, camera.fieldOfView(),
		mPostProcess.sceneDepth().height(), visibility );
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
	mInteractor.update( camera.eyePosition(), camera.viewDirection(), &mInventory, InputInteract() && !isFreeCameraActive() && !mWorld.isPlayerDead() );
	// 拾われた物は印を付けるだけ。実際に消えるのはティックの最後（GameWorld::update）。
	mWorld.interaction().removeTakenPickups();
	// マークは使った後の状態で決める（拾い切った物は出さない・給油した後の満タンは灰色）。死んだ後は出さない。
	if ( mWorld.isPlayerDead() ) {
		mInteractMarkers.clear();
	} else {
		mInteractMarkers.update( mWorld.interaction(), camera.eyePosition(), mInteractor.target(), mInventory );
	}
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

void GameScene::updatePlayerDamage() {
	bool hit = !mWorld.combat().zombieAttacksThisTick().empty();
	if ( hit ) {
		mControl.flinch(); // 狙いが跳ね、絵が大きく揺れる
	}
	mZombieAttackSounds.play( mWorld.combat().zombieAttacksThisTick() );
	const GameObject* player = mWorld.player();
	const float kFullHealthRatio = 1.f; // 自キャラがいないときは赤くしない
	const PlayerHealthComponent* healthComponent = player ? player->getComponent< PlayerHealthComponent >() : nullptr; // 外せる部品
	const PlayerHealth* health = healthComponent ? &healthComponent->health() : nullptr;
	float healthRatio = health ? health->current() / health->maxHealth() : kFullHealthRatio;
	mDamageView.update( hit, healthRatio, mWorld.isPlayerDead() );
	if ( mDamageView.isFadedOut() ) {
		SceneManager::instance().requestChange( SceneManager::SCENE_GAME_OVER );
	}
}
