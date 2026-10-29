#include "GameSrc/Scene/Game/GameScene.h"
#include "GameSrc/Scene/GameResources.h"
#include "GameSrc/Player/PlayerCharacter.h"
#include "GameSrc/Core/Pad.h"
#include "GameSrc/Scene/SceneManager.h"
#include "GameSrc/Render/ViewFrustum.h"
#include "GameSrc/Render/LightingParams.h"
#include "GameSrc/Render/ShadowParams.h"
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
	const float kMinCascadeGap = 0.5f;      // F6 で端を逆に並べても枚の範囲が 0 にならないよう、前の端よりこれだけ奥へ揃える（m）

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
	mInteractor( &mWorld.hitWorld(), kInteractReach, [ this ]( const HitWorld::Hit& hit ) { return mWorld.resolveInteractable( hit ); } ),
	mControl( mWorld.map().playerStart().yawDegree, &resources->settings() ), // キャラの向いている方を見始める
	mBodySounds( &resources->audio(), &mWorld.ground(), mWorld.map().groundZones() ),
	mZombieHitSounds( &resources->audio() ),
	mSurfaceHitSounds( &resources->audio() ),
	mZombieAttackSounds( &resources->audio() ),
	mHud( &resources->uiRenderer() ),
	mPauseMenu( &resources->uiRenderer(), &resources->settings(), &resources->audio(), [ this ]() { resume(); } ),
	mFlashlightShadow( kFlashlightShadowSize ),
	mFlashlightCastsShadow( false ),
	mMoonShadow( kMoonShadowSize ),
	mMoonCastsShadow( false ),
	mReady( false ),
	mPaused( false ) {
}

GameScene::~GameScene() {
	if ( mPaused ) {
		mResources->audio().setPaused( false );
	}
	EditorWindow::setPlayerInvincible( false ); // 自キャラがいなくなるので、ON のまま残さない
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
	// ESC か、窓が裏に回ったら止める（裏に回っている間に世界が進んで、戻ったら噛まれていた、とならないように）。
	if ( InputMenu() || !GameLib::WindowCreator().isActive() ) {
		pause();
		return;
	}

#ifdef ZF_EDITOR
	// デバッグ用。F5 で自キャラの無敵を切り替える（押した瞬間。Debug と Profile ビルドだけで効く）。
	if (InputDebugInvincible()) {
		PlayerCharacter* player = mWorld.player();
		if (player) {
			player->toggleInvincible();
		}
	}
	// F6 のエディタの一番上に出す。
	EditorWindow::setPlayerInvincible( mWorld.player() && mWorld.player()->isInvincible() );


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
	// 弾は照準のぶれを足した向き（銃が向いている所）へ飛ぶ。画面の中央（viewDirection）からは、ぶれの分だけずれる。
	mWorld.update( mControl.command(), camera.yaw(), camera.pitch(), camera.weaponDirection(), &mInventory );
	mControl.consumePressed(); // 使ったので消す（同じ描画の間の次の update で、もう一度反応しないように）
	mZombieHitSounds.play( mWorld.zombieHitsThisTick() ); // このティックに当たった分だけ鳴らす（次の update で入れ替わるので、ここで読む）
	mSurfaceHitSounds.play( mWorld.surfaceHitsThisTick() ); // 物に当たった分も同じ（読む決まりも同じ）
	updatePlayerDamage();
	PlayerCharacter* player = mWorld.player();
	// 撃ったティックの弾は跳ねる前の向きへ飛び、跳ねた視点は次のティックから効く（1発目は狙った所へ飛ぶ）。
	// 噛まれた怯み・揺れ（updatePlayerDamage で受けた分）もここでカメラへ渡す。
	// 照準のぶれも同じく次のティックから効く（自キャラがいなければ既定の状態＝ぶれない）。
	mControl.updateViewEffects( player && player->firedThisTick(), player ? player->aimSwayBody() : AimSwayBody() );
	if ( player ) {
		// 銃の見た目へ、弾の向き（weaponDirection）と同じ角度のぶれを渡す。
		player->setWeaponSway( mControl.camera().weaponSwayYaw(), mControl.camera().weaponSwayPitch() );
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
	const PlayerCharacter* player = mWorld.player();
	if ( player ) {
		Vector3 eye = player->eyePosition();
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
	{
		PROFILE_SCOPE( "Audio" );
		updateListener();
	}
	// 噛まれた瞬間の赤み・痛み・死んだ後の暗さは、この描画の写すときに効く。
	mPostProcess.setDamageFlash( mDamageView.damageFlash() );
	mPostProcess.setPain( mDamageView.pain() );
	mPostProcess.setFade( mDamageView.fade() );
	mPostProcess.beginScene();
	setupLights( camera );
	drawShadowMaps( camera );
	// 不透明な物は G-Buffer へ描き、全画面 1 回でライティングする（GBuffer_Plan.txt 5 章）。
	// フォワードへの切り替えは段階 3 の始めに消した（PbrNightLighting_Plan.txt Q1。PBR の式とライトを 2 か所に書かない）。
	const GBufferView view = gbufferView();
	const bool showsGBuffer = ( view != GBufferView::Final );
	mGBuffer.begin( mPostProcess.sceneDepth() );
	// 視点を置いた後の行列から映る範囲を作り、映らない地面・建物を描かない。
	{
		PROFILE_SCOPE( "World.draw" );
		mWorld.draw( ViewFrustum( camera.projectionViewMatrix() ), camera.eyePosition() );
	}
	mGBuffer.end();
	if ( !showsGBuffer ) {
		mPostProcess.bindSceneColor(); // 深度はシェーダから読むので、描画先からは外す
		mLights.bindShadowMaps(); // 描画先を変えると t2 以降が外れるので、付け直す（LightList::bindShadowMaps）
		mDeferredLighting.drawLighting( mGBuffer, mPostProcess.sceneDepth(), camera.projectionViewMatrix() );
		Profiling::gpuMark( "Lighting" );
		// 霧は半透明の板より前（板は自分で同じ霧を掛けるので、二重に掛けない。NightFinish_Plan.txt 4 章）。
		if ( fogEnabled() ) {
			mDeferredLighting.drawFog( mPostProcess.sceneDepth(), camera.projectionViewMatrix() );
		}
		Profiling::gpuMark( "Fog" );
	}
	mPostProcess.resumeScene(); // 弾痕などの板は、G-Buffer と同じ深度で隠れるようシーンへ描く
	mLights.bindShadowMaps();   // 板も懐中電灯の影を受ける（同じ evaluateLight を通る）
	mWorld.drawBulletHoles( camera.projectionViewMatrix() ); // 壁・地面を描いた後（奥行きで隠すため）
	mWorld.drawImpactDust( camera.projectionViewMatrix(), camera.viewDirection(), camera.eyePosition() ); // 弾痕の上に舞う
	mWorld.drawBloodSpray( camera.projectionViewMatrix(), camera.viewDirection(), camera.eyePosition() ); // 砂ぼこりの後（同じ作り）
	mWorld.drawMuzzleFlash( camera.projectionViewMatrix(), camera.viewDirection() ); // 加算なので順番は問わない
	mLights.unbind(); // ライトの一覧を読むのは、ここまで（ライティングのパスと半透明の板）
#ifdef ZF_EDITOR
	mDebug.drawCollision( mWorld, camera );
	mDebug.drawLights( mLights, camera );
#endif
	// GPU の時間の区切り（Perf の欄の GPU）。世界の物ごとの印は GameWorld::draw の中（Player〜Ground）。
	// ここまでが弾痕・砂ぼこり・血・マズルフラッシュ・当たり判定の表示、次の印までが画面全体の処理（ブルームなど）。
	Profiling::gpuMark( "Effects" );
	mPostProcess.endScene();
	if ( showsGBuffer ) {
		// ToneMap を通さず中身の数のまま見るので、画面へ写した後に上から描く。
		// 影の表示（Cascade・Shadow maps）はライトの一覧の定数と影のマップを読むので、外した物をもう一度付けて描く。
		mLights.rebindConstants();
		mLights.bindShadowMaps();
		mDeferredLighting.drawView( view, mGBuffer, mPostProcess.sceneDepth(), camera.projectionViewMatrix() );
		mLights.unbind();
	}
	// 「PostProcess」「FXAA」の印は endScene の中で打つ。ここまでは G-Buffer の表示（F6 で選んだときだけ。ふだんはほぼ 0）。
	Profiling::gpuMark( "GBufferView" );
	// UiRenderer は 1 回の描画に 1 回しか描けない（中の PrimitiveRenderer が 2 回目の draw で ASSERT）。
	// 止めている間は HUD を出さず、メニューがまとめて描く（2026-10-23。HUD とメニューの両方が描いて止まっていた）。
	if ( mPaused ) {
		mPauseMenu.draw();
	} else {
#ifdef ZF_EDITOR
		// 積むだけ。すぐ下の HUD の描画（UiRenderer::draw）でまとめて描かれる。
		mDebug.addZombieHealth( mWorld, camera, &mResources->uiRenderer() );
		mDebug.addPlayerHealth( mWorld, &mResources->uiRenderer() );
#endif
		// 真ん中の照準「+」は出さない（ユーザー指定 2026-10-26）。
		mHud.draw( mInteractor.prompt().c_str(), mWorld.escapeCar(), mInventory );
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
	if ( mWorld.muzzleLight( &muzzle ) ) {
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
		mMoonShadow.update( camera.eyePosition(), camera.viewDirection(), camera.fieldOfView(), aspect, direction, ends, shadow.moonCasterMargin );
		for ( int i = 0; i < CascadedShadowMap::kCascadeCount; ++i ) {
			mLights.setShadowMap( kMoonShadowFirstIndex + i, mMoonShadow.map( i ).depth(), mMoonShadow.projectionView( i ),
				mMoonShadow.texelSize( i ), kOrthographicShadow );
		}
		mLights.setCascades( camera.viewDirection(), ends, shadow.moonCascadeBlend );
		moon.shadowMap = kMoonShadowFirstIndex;
		moon.shadowMapCount = CascadedShadowMap::kCascadeCount;
	}
	mLights.add( moon );
}

void GameScene::addFlashlight( const LightingParams& params ) {
	mFlashlightCastsShadow = false;
	const PlayerCharacter* player = mWorld.player();
	Vector3 muzzlePosition;
	Vector3 muzzleDirection;
	// 銃をしまっている間は銃口が無い（muzzle が false）ので照らさない。付けた状態は自キャラが覚えている。
	if ( !player || !player->isFlashlightOn() || !player->muzzle( &muzzlePosition, &muzzleDirection ) ) {
		return;
	}
	Vector3 position;
	position.setMadd( muzzlePosition, muzzleDirection, params.flashlightOffset );
	// F6 で内側を外側より大きくしても止まらないよう、内側は外側までに抑える。
	float innerAngle = min( params.flashlightInnerAngle, params.flashlightOuterAngle );
	LightSource flashlight = spotLight( position, muzzleDirection, params.flashlightColor, params.flashlightIntensity, params.flashlightRange,
		innerAngle, params.flashlightOuterAngle );
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
		mWorld.drawShadowCasters( ViewFrustum( mFlashlightShadowMatrix ), kWithoutPlayer );
		mFlashlightShadow.end();
	}
	if ( mMoonCastsShadow ) {
		for ( int i = 0; i < CascadedShadowMap::kCascadeCount; ++i ) {
			const Matrix44& matrix = mMoonShadow.projectionView( i );
			mMoonShadow.map( i ).begin( matrix );
			mWorld.drawShadowCasters( ViewFrustum( matrix ), kWithPlayer );
			mMoonShadow.map( i ).end();
		}
	}
	// 影のマップは光の側の行列で描いたので、G-Buffer へ描く前にカメラの行列へ戻す。
	Graphics::Manager::instance().setProjectionViewMatrix( camera.projectionViewMatrix() );
	// GPU の時間の区切り。ここまでが影のマップ（懐中電灯と月の 3 枚）。
	Profiling::gpuMark( "Shadow" );
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
	const PlayerCharacter* player = mWorld.player();
	const float kFullHealthRatio = 1.f; // 自キャラがいないときは赤くしない
	float healthRatio = player ? player->health().current() / player->health().maxHealth() : kFullHealthRatio;
	mDamageView.update( bitten, healthRatio, mWorld.isPlayerDead() );
	if ( mDamageView.isFadedOut() ) {
		SceneManager::instance().requestChange( SceneManager::SCENE_GAME_OVER );
	}
}
