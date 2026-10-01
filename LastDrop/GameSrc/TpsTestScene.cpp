#include "TpsTestScene.h"
#include "PlayerCharacter.h"
#include "Pad.h"

#include "GameLib/GameLib.h"
#include "Framework.h" // DebugScreen.h が使う Framework の宣言
#include "GameLib/DebugScreen.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/Enum.h"
#include "GameLib/Input/Manager.h"
#include "GameLib/Math/Vector3.h"

#include <cstdio>
using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 置き場所（地面 Y=0 の上）と向き。
	struct Placement{
		float x;
		float z;
		float yawDegree; // 0 で +Z を向く
	};

	// ── 自キャラ（Mixamo Ch15。身長約1.8）──
	const char* kPlayerFile = "Resource/player.txt"; // tools/configs/player.json で変換
	// 通りの手前に、建物（-Z 側）を向いて立たせる。カメラも同じ向きから始め、目線で通りを見る。
	const Placement kPlayerStart = { 0.f, 8.f, 180.f };
	const char* kWeaponFile = "Resource/rifle.txt"; // tools/configs/rifle.json で変換（AKM）

	// ── 背景。配置は SkinTestScene と同じ（見比べられるように）──
	const char* kZombieFile = "Resource/character.txt";
	const char* kZombieAnimation = "walk";
	const Placement kZombiePlacements[] = {
		{  0.0f,  0.0f,   0.f },
		{ -2.5f, -2.0f,  25.f },
		{  2.8f, -3.0f, -30.f },
	};
	const float kAnimationStartStep = 11.f; // 体ごとに歩き出しをずらす（フレーム）
	const char* kBuildingFile = "Resource/building.txt";
	const float kBuildingYaw = -90.f;       // 正面(+X)を +Z へ向ける
	const Placement kBuildingPlacements[] = {
		{ -20.5f, -13.f, kBuildingYaw },
		{   0.0f, -13.f, kBuildingYaw },
		{  20.5f, -13.f, kBuildingYaw },
	};

	// ── 視点操作 ──
	// マウス1画素あたりの回転（度）。横に動かすと左右、縦に動かすと上下に回る。
	const float kLookDegreesPerPixel = 0.15f;
	// 縦の視野角（度）。覗いている間は狭めて少しズームする（アイアンサイトなので控えめ）。
	const float kHipFieldOfView = 60.f;
	const float kAimFieldOfView = 45.f;

	// ── ライティング ──
	// キャラが歩き回っても明るさが変わらないよう、太陽のような遠い光源にする。
	// 強さに距離を入れると、距離による減衰が打ち消されて一定の明るさになる（Main の従来ゲームと同じ方法）。
	const Vector3 kSunPosition( 300.f, 500.f, 400.f );
	const Vector3 kSunColor( 1.f, 0.95f, 0.9f );
	const Vector3 kFillPosition( -300.f, 200.f, -400.f ); // 影側を真っ黒にしない補助光
	const Vector3 kFillColor( 0.35f, 0.35f, 0.4f );
	const Vector3 kAmbientColor( 0.4f, 0.4f, 0.4f );
	const int kSunLight = 0;
	const int kFillLight = 1;
	const int kLightNumber = 4; // GameLib のライト数。使わないライトは強さ0にする

	// ── 床の材質（足音の確認用）──
	// 自キャラの少し前（建物側）に、幅 3m の長方形を横一列に並べる。真ん中はコンクリートのまま。
	// 始めの向き（建物の方）から見て、左へ横歩きすると 水たまり → 砂利 → 草、右へ行くと 木 → 金属 → ガラス。
	// 見た目の目印はまだ無いので、足元の材質は画面左上に出す。
	const float kZoneNearZ = 7.f;
	const float kZoneFarZ = 3.f;
	const GroundZone kGroundZones[] = {
		{ -10.5f, kZoneFarZ, -7.5f, kZoneNearZ, GROUND_GRASS  },
		{  -7.5f, kZoneFarZ, -4.5f, kZoneNearZ, GROUND_GRAVEL },
		{  -4.5f, kZoneFarZ, -1.5f, kZoneNearZ, GROUND_WATER  },
		{   1.5f, kZoneFarZ,  4.5f, kZoneNearZ, GROUND_WOOD   },
		{   4.5f, kZoneFarZ,  7.5f, kZoneNearZ, GROUND_METAL  },
		{   7.5f, kZoneFarZ, 10.5f, kZoneNearZ, GROUND_GLASS  },
	};
	const int kGroundZoneCount = sizeof( kGroundZones ) / sizeof( kGroundZones[ 0 ] );
	const GroundMaterial kDefaultGround = GROUND_CONCRETE; // 通り

	// 装備の音を鳴らす高さ（足元から。胸のあたり）。足音は足元、装備は体から鳴らして聞こえる高さを分ける。
	const float kBodySoundHeight = 1.3f;

	// ── 操作説明（DebugScreen の文字位置）──
	const int kHelpColumn = 1;
	const int kHelpRow = 1;
	const int kGroundRow = 2;
	const int kDebugTextSize = 64;

	// 押されていれば 1、いなければ 0（逆方向同士を引き算して軸にする）。
	float axisValue( bool on ) {
		return on ? 1.f : 0.f;
	}

	// 自キャラへの操作を Pad から読む。キー割り当てとキャラの動きをつなぐのはここだけ。
	PlayerCommand readPlayerCommand() {
		PlayerCommand command;
		command.forward = axisValue( InputUp() ) - axisValue( InputDown() );
		command.strafe = axisValue( InputRight() ) - axisValue( InputLeft() );
		command.run = InputRun();
		command.holsterPressed = InputHolster();
		command.aimHeld = InputAim();
		return command;
	}
}

TpsTestScene::TpsTestScene() :
	mPlayerData( 0 ),
	mWeaponData( 0 ),
	mZombieData( 0 ),
	mBuildingData( 0 ),
	mGround( kGroundZones, kGroundZoneCount, kDefaultGround ),
	mFootsteps( &mAudio ),
	mFoley( &mAudio ),
	mReady( false ) {
	// 読み込みを頼むだけ（非同期）。構築が終わったら placeObjects で並べる。
	mPlayerData = mCache.skinnedMesh( kPlayerFile );
	mWeaponData = mCache.staticMesh( kWeaponFile );
	mZombieData = mCache.skinnedMesh( kZombieFile );
	mBuildingData = mCache.staticMesh( kBuildingFile );
	mCamera.rotate( kPlayerStart.yawDegree, 0.f ); // キャラの向いている方を見始める
}

TpsTestScene::~TpsTestScene() {
	// シーンを抜けたらカーソルを返す（隠れたまま・閉じ込められたままにしない）。
	GameLib::Input::Manager::instance().setCursorLocked( false );
}

void TpsTestScene::placeObjects() {
	// 構築に失敗したデータ（理由は MeshCache がデバッグ出力済み）は置かない。
	if ( mPlayerData->isReady() ) {
		// 銃の構築に失敗していたら素手で出す（キャラまで出さないと操作の確認ができないため）。
		const StaticMeshData* weapon = mWeaponData->isReady() ? mWeaponData : nullptr;
		mPlayer = std::make_unique< PlayerCharacter >(
			mPlayerData, weapon, Vector3( kPlayerStart.x, 0.f, kPlayerStart.z ), kPlayerStart.yawDegree );
	}
	int zombieIndex = 0;
	for ( const Placement& p : kZombiePlacements ) {
		if ( !mZombieData->isReady() ) {
			break;
		}
		auto zombie = std::make_unique< SkinnedMeshInstance >( mZombieData );
		zombie->setPosition( Vector3( p.x, 0.f, p.z ) );
		zombie->setAngleY( p.yawDegree );
		zombie->setAnimation( kZombieAnimation, kAnimationStartStep * zombieIndex );
		mZombies.push_back( std::move( zombie ) );
		++zombieIndex;
	}
	for ( const Placement& p : kBuildingPlacements ) {
		StaticMeshInstance building( mBuildingData );
		building.setPosition( Vector3( p.x, 0.f, p.z ) );
		building.setAngleY( p.yawDegree );
		mBuildings.push_back( building );
	}
	// 始めから視点を回せるようにロックしておく。
	GameLib::Input::Manager::instance().setCursorLocked( true );
}

void TpsTestScene::updateCursorLock() {
	GameLib::Input::Manager input = GameLib::Input::Manager::instance();
	if ( InputMenu() ) {
		input.setCursorLocked( false );
	} else if ( InputFire() && !input.isCursorLocked() ) {
		// 解除中のクリックは「ゲームに戻る」操作として扱い、ロックし直す。
		input.setCursorLocked( true );
	}
}

void TpsTestScene::rotateCamera() {
	if ( !GameLib::Input::Manager::instance().isCursorLocked() ) {
		return;
	}
	AxisStatus look = InputLook();
	// ズーム中は視野角の比で感度を下げる。同じだけマウスを動かしたとき、画面上で動く量を揃えるため。
	float degreesPerPixel = kLookDegreesPerPixel * fieldOfView() / kHipFieldOfView;
	// マウスを右へ動かすと右を向く＝ヨーは減る（ヨーは上から見て反時計回りが正）。
	// マウスを下へ動かすと下を向く＝ピッチ（見下ろしが正）は増える。
	mCamera.rotate( -look.m_width * degreesPerPixel, look.m_height * degreesPerPixel );
}

float TpsTestScene::fieldOfView() const {
	float aim = mPlayer ? mPlayer->aimWeight() : 0.f;
	return kHipFieldOfView + ( kAimFieldOfView - kHipFieldOfView ) * aim;
}

void TpsTestScene::placeCameraAtEye() {
	if ( mPlayer ) {
		mCamera.setEyePosition( mPlayer->eyePosition() );
	}
	mCamera.setFieldOfView( fieldOfView() );
	mCamera.apply();
}

void TpsTestScene::updateAudio() {
	// 聞き手は目の位置と視線の向き。首を振ると音の左右が入れ替わる。
	mAudio.setListener( mCamera.eyePosition(), mCamera.viewDirection() );
	if ( mPlayer ) {
		const Vector3& feet = mPlayer->position();
		bool stepped = mFootsteps.update( mPlayer->gait(), mPlayer->gaitPhase(), feet, mGround.materialAt( feet ) );
		PlayerFoley::FoleyState foley;
		foley.bodyPosition.set( feet.x, feet.y + kBodySoundHeight, feet.z );
		foley.gait = mPlayer->gait();
		foley.weaponDrawn = mPlayer->isWeaponDrawn();
		foley.aiming = mPlayer->isAiming();
		mFoley.update( foley, stepped );
	}
	mAudio.update();
}

void TpsTestScene::setupLights() const {
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.enableDepthTest( true );
	gm.enableDepthWrite( true );
	gm.setCullMode( Graphics::CULL_NONE ); // 裏返っても見えるように両面描画（SkinTestScene と同じ）
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

void TpsTestScene::draw() const {
	if ( mPlayer ) {
		mPlayer->draw();
	}
	for ( const auto& zombie : mZombies ) {
		zombie->draw();
	}
	for ( const StaticMeshInstance& building : mBuildings ) {
		building.draw();
	}
}

void TpsTestScene::update() {
	// 入力は1フレームに1回だけ読む（従来ゲームは Main が読むが、SceneManager のシーンは自分で読む）。
	Pad::instance()->update();

	// 読み込み・構築・テクスチャのロードを進める（構築後もテクスチャのために毎フレーム呼ぶ）。
	bool allBuilt = mCache.update();
	if ( !mReady ) {
		if ( !allBuilt ) {
			return; // ロード完了待ち
		}
		placeObjects();
		mReady = true;
	}

	updateCursorLock();
	// 順番に意味がある: 向きを決める → その向きでキャラを動かし体を曲げる → 動いた後の頭の位置へ視点を置く。
	// 逆にすると W が1フレーム前の向きへの前進になり、視点も体に1フレーム遅れて、腕や銃が画面の中で揺れて見える。
	rotateCamera();
	if ( mPlayer ) {
		mPlayer->update( readPlayerCommand(), mCamera.yaw(), mCamera.pitch() );
	}
	for ( auto& zombie : mZombies ) {
		zombie->update();
	}
	placeCameraAtEye();
	updateAudio();
	setupLights();
	draw();

	DebugScreen::instance().draw( kHelpColumn, kHelpRow,
		GameLib::Input::Manager::instance().isCursorLocked()
			? "WASD: move  Shift: run  1: draw/holster  RMB: aim (tap=toggle, hold)  Mouse: look  Esc: release cursor"
			: "Click: lock cursor (mouse look)" );
	if ( mPlayer ) {
		char ground[ kDebugTextSize ];
		std::snprintf( ground, sizeof( ground ), "Floor: %s", groundMaterialName( mGround.materialAt( mPlayer->position() ) ) );
		DebugScreen::instance().draw( kHelpColumn, kGroundRow, ground );
	}
}
