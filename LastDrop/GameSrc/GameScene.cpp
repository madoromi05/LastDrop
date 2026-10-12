#include "GameScene.h"
#include "GameResources.h"
#include "PlayerCharacter.h"
#include "Pad.h"
#include "SceneManager.h"
#include "ViewFrustum.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/Enum.h"
#include "GameLib/Math/Vector3.h"

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
	const int kLightNumber = 4; // GameLib のライト数。使わないライトは強さ0にする
}

GameScene::GameScene( GameResources* resources ) :
	mResources( resources ),
	mWorld( &resources->meshes() ),
	mInteractor( &mWorld.hitWorld(), kInteractReach, [ this ]( const HitWorld::Hit& hit ) { return mWorld.resolveInteractable( hit ); } ),
	mControl( mWorld.map().playerStart().yawDegree ), // キャラの向いている方を見始める
	mBodySounds( &resources->audio(), &mWorld.ground(), mWorld.map().groundZones() ),
	mHud( &resources->uiRenderer() ),
	mReady( false ) {
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

	// 視点はマウスが動いた分だけ回す。update の回数に合わせると、2回進めた描画で2倍回ってしまう。
	mControl.readInput( aimWeight() );
	updateInteraction();
#ifdef _DEBUG
	mDebug.handleInput();
#endif
}

void GameScene::update() {
	if ( !mReady ) {
		return;
	}
	// 順番に意味がある: 向きを決める（beginFrame）→ その向きでキャラを動かし体を曲げる → 動いた後の頭の位置へ視点を置く（draw）。
	// 逆にすると W が1フレーム前の向きへの前進になり、視点も体に1フレーム遅れて、腕や銃が画面の中で揺れて見える。
	const FirstPersonCamera& camera = mControl.camera();
	mWorld.update( mControl.command(), camera.yaw(), camera.pitch(), camera.viewDirection() );
	mControl.consumePressed(); // 使ったので消す（同じ描画の間の次の update で、もう一度反応しないように）
	if ( const PlayerCharacter* player = mWorld.player() ) {
		mBodySounds.update( *player );
	}
}

void GameScene::draw() {
	if ( !mReady ) {
		return;
	}
	const PlayerCharacter* player = mWorld.player();
	if ( player ) {
		Vector3 eye = player->eyePosition();
		mControl.placeCamera( &eye, aimWeight() );
	} else {
		mControl.placeCamera( nullptr, aimWeight() );
	}
	updateListener();
	mPostProcess.beginScene();
	setupLights();
	// 視点を置いた後の行列から映る範囲を作り、映らない地面・建物を描かない。
	mWorld.draw( ViewFrustum( mControl.camera().projectionViewMatrix() ) );
#ifdef _DEBUG
	mDebug.drawCollision( mWorld, mControl.camera() );
#endif
	mPostProcess.endScene();
	mHud.draw( mInteractor.prompt().c_str(), mWorld.escapeCar(), mInventory );

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
	for ( int i = kFillLight + 1; i < kLightNumber; ++i ) {
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
	mInteractor.update( camera.eyePosition(), camera.viewDirection(), player, &mInventory, InputInteract() );
	// 拾われた物は印を付けるだけ。実際に消えるのはティックの最後（GameWorld::update）。
	mWorld.removeTakenPickups();
	if ( mWorld.isEscapeReady() ) {
		SceneManager::instance().requestChange( SceneManager::SCENE_CLEAR );
	}
}

void GameScene::updateListener() {
	// 聞き手は目の位置と視線の向き。首を振ると音の左右が入れ替わる。
	const FirstPersonCamera& camera = mControl.camera();
	AudioEngine& audio = mResources->audio();
	audio.setListener( camera.eyePosition(), camera.viewDirection() );
	audio.update();
}

float GameScene::aimWeight() const {
	const PlayerCharacter* player = mWorld.player();
	return player ? player->aimWeight() : 0.f;
}
