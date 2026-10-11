#include "TpsTestScene.h"
#include "PlayerCharacter.h"
#include "Pad.h"
#include "SceneManager.h"
#include "SpawnPicker.h"
#include "UiLabel.h"
#include "VertexColor.h"

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
	// 置き場所はどれも TownMap が決める。ここにあるのは、何を置くか（ファイル）と、置き場所以外の決めごとだけ。

	// ── 自キャラ（Mixamo Ch15。身長約1.8）──
	const char* kPlayerFile = "Resource/player.txt"; // tools/configs/player.json で変換
	const char* kWeaponFile = "Resource/rifle.txt"; // tools/configs/rifle.json で変換（AKM）

	// ── 背景 ──
	const char* kZombieFile = "Resource/character.txt";
	const char* kZombieAnimation = "walk";
	const float kAnimationStartStep = 11.f; // 体ごとに歩き出しをずらす（フレーム）
	// 建物は tools/configs/building.json の scale で、ドアが人の背丈に合う大きさ（ドアの高さ約 2.0m）に縮めてある。
	const char* kBuildingFile = "Resource/building.txt";
	// 町を囲む建物。同じ建物から足元の歩道の板を除いた形（tools/configs/building_noground.json）。
	// 板があると、壁どうしを付けて並べたときに隣の板と重なって面がちらつくため。
	const char* kPerimeterBuildingFile = "Resource/building_noground.txt";

	// ── ゲームループ（ガソリンを集めて車で脱出）──
	const char* kVehicleFile = "Resource/uaz.txt"; // tools/configs/uaz_atlas.json → uaz.json で変換（実物大・原点は底面の中心）
	const char* kFuelCanFile = "Resource/fuelcan.txt"; // tools/configs/fuelcan_atlas.json → fuelcan.json で変換（同上）
	// 車に入れるとクリアになるガソリンの数。候補の場所（TownMap）から、毎回この数だけ選んで置く（SpawnPicker）。
	const int kRequiredFuel = 3;
	const float kInteractReach = 2.5f; // 手が届く距離（m）。足元の缶を立ったまま拾える長さ

	// ── HUD ──
	const char* kCrosshair = "+";
	const unsigned kHudColor = VertexColor::pack( 0.95f, 0.95f, 0.95f );
	const unsigned kPromptColor = VertexColor::pack( 1.f, 0.85f, 0.3f ); // 燃料の色（黄）
	const int kTankLine = 1;  // 左下から数えた行（0 が一番下）
	const int kCarryLine = 0;
	const int kHudTextSize = 64;

	// ── 視点操作 ──
	// マウス1画素あたりの回転（度）。横に動かすと左右、縦に動かすと上下に回る。
	const float kLookDegreesPerPixel = 0.15f;
	// 縦の視野角（度）。覗いている間は狭めて少しズームする（アイアンサイトなので控えめ）。
	const float kHipFieldOfView = 60.f;
	const float kAimFieldOfView = 45.f;

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

	// ── 床の材質 ──
	// 周りと違う材質の場所（足音の確認用）は TownMap が決める。それ以外の、道路と歩道の材質がこれ。
	const GroundMaterial kDefaultGround = GROUND_CONCRETE;

	// 装備の音を鳴らす高さ（足元から。胸のあたり）。足音は足元、装備は体から鳴らして聞こえる高さを分ける。
	const float kBodySoundHeight = 1.3f;

	// ── 操作説明（DebugScreen の文字位置）──
	const int kHelpColumn = 1;
	const int kHelpRow = 1;
	const int kGroundRow = 2;
	const int kCollisionRow = 3; // 当たり判定の表示の ON/OFF（Debug ビルドだけ）
	const int kShaderErrorRow = 4; // シェーダのコンパイルエラー（あるときだけ）
	const int kRayRow = 5;         // 画面中央から撃った確認用の線分が何に当たったか（Debug ビルド・当たり判定の表示中だけ）
	const float kDebugRayLength = 100.f; // 確認用の線分の長さ（m）。通りの先の建物まで届く長さ
	const int kDebugTextSize = 64;
	const int kErrorTextSize = 160; // コンパイラのエラーはファイル名と行番号が付いて長い

	// コンパイラのエラー文の1行目を、ファイル名から先だけ画面に出す。
	// 1行目は「フォルダ\ファイル名(行,列): error ...」の形で、フォルダまで出すと画面の幅が足りず、肝心の行番号と理由が見えないため。
	// DebugScreen は改行を扱わないので2行目以降は出さない（全文はデバッグ出力にある）。
	void drawErrorLine( int column, int row, const char* text ) {
		int lineEnd = 0;
		while ( text[ lineEnd ] != '\0' && text[ lineEnd ] != '\n' ) {
			++lineEnd;
		}
		// ファイル名の始まり＝行番号の括弧より前にある、最後のフォルダの区切りの次。
		// 括弧より後（エラーの理由）にある '/' はファイル名と関係ないので見ない。
		int nameStart = 0;
		for ( int i = 0; i < lineEnd && text[ i ] != '('; ++i ) {
			if ( text[ i ] == '\\' || text[ i ] == '/' ) {
				nameStart = i + 1;
			}
		}
		char line[ kErrorTextSize ];
		int length = 0;
		while ( nameStart + length < lineEnd && length < kErrorTextSize - 1 ) {
			line[ length ] = text[ nameStart + length ];
			++length;
		}
		line[ length ] = '\0';
		DebugScreen::instance().draw( column, row, line );
	}

	Vector3 toPosition( const TownPlacement& placement ) {
		return Vector3( placement.x, placement.y, placement.z );
	}

	// 押されていれば 1、いなければ 0（逆方向同士を引き算して軸にする）。
	float axisValue( bool on ) {
		return on ? 1.f : 0.f;
	}

	// 自キャラへの操作を Pad から読む。キー割り当てとキャラの動きをつなぐのはここだけ。
	// 押されている間の入力は今の状態で上書きし、押した瞬間の入力は使われるまで残す（update で消す）。
	void readPlayerCommand( PlayerCommand* command ) {
		command->forward = axisValue( InputUp() ) - axisValue( InputDown() );
		command->strafe = axisValue( InputRight() ) - axisValue( InputLeft() );
		command->run = InputRun();
		command->holsterPressed = command->holsterPressed || InputHolster();
		command->aimHeld = InputAim();
	}
}

TpsTestScene::TpsTestScene() :
	mPlayerData( 0 ),
	mWeaponData( 0 ),
	mZombieData( 0 ),
	mBuildingData( 0 ),
	mPerimeterBuildingData( 0 ),
	mVehicleData( 0 ),
	mFuelCanData( 0 ),
	mTownGround( &mCache ),
	mZombies( &mWorld, &mHitWorld ),
	mPickups( &mWorld, &mHitWorld ),
	mInteractor( &mHitWorld, kInteractReach, [ this ]( const HitWorld::Hit& hit ) { return resolveInteractable( hit ); } ),
	mPromptLabel( 0 ),
	mTankLabel( 0 ),
	mCarryLabel( 0 ),
	mGround( mMap.groundZones().data(), static_cast< int >( mMap.groundZones().size() ), kDefaultGround ),
	mFootsteps( &mAudio ),
	mFoley( &mAudio ),
	mCommand(),
	mReady( false )
#ifdef _DEBUG
	, mShowCollision( false )
#endif
	{
	// 読み込みを頼むだけ（非同期）。構築が終わったら placeObjects で並べる。
	mPlayerData = mCache.skinnedMesh( kPlayerFile );
	mWeaponData = mCache.staticMesh( kWeaponFile );
	mZombieData = mCache.skinnedMesh( kZombieFile );
	mBuildingData = mCache.staticMesh( kBuildingFile );
	mPerimeterBuildingData = mCache.staticMesh( kPerimeterBuildingFile );
	mVehicleData = mCache.staticMesh( kVehicleFile );
	mFuelCanData = mCache.staticMesh( kFuelCanFile );
	mCamera.rotate( mMap.playerStart().yawDegree, 0.f ); // キャラの向いている方を見始める
	buildHud();
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
		const TownPlacement& start = mMap.playerStart();
		mPlayer = std::make_unique< PlayerCharacter >( mPlayerData, weapon, &mWorld, toPosition( start ), start.yawDegree );
		mPlayer->addTo( &mWorld, &mHitWorld );
	}
	if ( mZombieData->isReady() ) {
		// 置く（移動・命中の両方の当たり判定への登録は ActorList が行う）。animation が nullptr なら T ポーズ。
		auto addZombie = [ this ]( const TownPlacement& p, const char* animation, float startTime ) {
			mZombies.spawn( std::make_unique< Zombie >( mZombieData, toPosition( p ), p.yawDegree, animation, startTime ) );
		};
		int zombieIndex = 0;
		for ( const TownPlacement& p : mMap.walkingZombies() ) {
			addZombie( p, kZombieAnimation, kAnimationStartStep * zombieIndex );
			++zombieIndex;
		}
		// 当たり判定の確認用の1体。手足が伸びていて、部位の箱の大きさ・位置を見比べやすい。
		addZombie( mMap.tPoseZombie(), nullptr, 0.f );
	}
	mTownGround.place( mMap );
	placeBuildings( mBuildingData, mMap.buildings() );
	placeBuildings( mPerimeterBuildingData, mMap.perimeterBuildings() );
	// 弾を止める物として登録する。mBuildings を並べ終えてから行う（push_back の途中で登録すると、
	// 並べ直しで要素の場所が変わり、名簿が古い場所を指してしまうため）。
	for ( const Building& building : mBuildings ) {
		mHitWorld.addBlocker( &building );
	}
	placeMissionObjects();
	// 始めから視点を回せるようにロックしておく。
	GameLib::Input::Manager::instance().setCursorLocked( true );
}

void TpsTestScene::placeBuildings( const StaticMeshData* data, const std::vector< TownPlacement >& placements ) {
	// 当たり判定を三角形から作るので、構築に失敗した建物は置かない。
	if ( !data->isReady() ) {
		return;
	}
	for ( const TownPlacement& p : placements ) {
		Building building( data, toPosition( p ), p.yawDegree );
		building.addCollidersTo( &mWorld );
		mBuildings.push_back( building );
	}
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

void TpsTestScene::updateBodySounds() {
	if ( !mPlayer ) {
		return;
	}
	const Vector3& feet = mPlayer->position();
	bool stepped = mFootsteps.update( mPlayer->gait(), mPlayer->gaitPhase(), feet, mGround.materialAt( feet ) );
	PlayerFoley::FoleyState foley;
	foley.bodyPosition.set( feet.x, feet.y + kBodySoundHeight, feet.z );
	foley.gait = mPlayer->gait();
	foley.weaponDrawn = mPlayer->isWeaponDrawn();
	foley.aiming = mPlayer->isAiming();
	mFoley.update( foley, stepped );
}

void TpsTestScene::updateListener() {
	// 聞き手は目の位置と視線の向き。首を振ると音の左右が入れ替わる。
	mAudio.setListener( mCamera.eyePosition(), mCamera.viewDirection() );
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

void TpsTestScene::drawWorld() const {
	mTownGround.draw();
	if ( mPlayer ) {
		mPlayer->draw();
	}
	for ( const auto& zombie : mZombies ) {
		zombie->draw();
	}
	for ( const Building& building : mBuildings ) {
		building.draw();
	}
	if ( mEscapeCar ) {
		mEscapeCar->draw();
	}
	for ( const auto& pickup : mPickups ) {
		pickup->draw();
	}
}

void TpsTestScene::placeMissionObjects() {
	// 箱の大きさを頂点から求めるので、構築に失敗したデータは置かない（その場合はクリアできないが、他の確認はできる）。
	if ( mVehicleData->isReady() ) {
		const TownPlacement& p = mMap.escapeCar();
		mEscapeCar = std::make_unique< EscapeCar >( mVehicleData, toPosition( p ), p.yawDegree, kRequiredFuel );
		mEscapeCar->addTo( &mWorld, &mHitWorld );
	}
	if ( mFuelCanData->isReady() ) {
		// 遊ぶたびに違う場所に出す（起動からの時間を種にする）。
		unsigned seed = Framework::instance().time();
		const std::vector< TownPlacement >& spawnPoints = mMap.fuelSpawnPoints();
		for ( int index : SpawnPicker::pick( static_cast< int >( spawnPoints.size() ), kRequiredFuel, seed ) ) {
			const TownPlacement& p = spawnPoints[ index ];
			mPickups.spawn( std::make_unique< Pickup >( mFuelCanData, toPosition( p ), p.yawDegree, ITEM_FUEL ) );
		}
	}
}

void TpsTestScene::buildHud() {
	UiCanvas& canvas = mHud.canvas();
	canvas.add( std::make_unique< UiLabel >( kCrosshair, UiScreen::screenCenter(), UI_TEXT_NORMAL, UI_ALIGN_CENTER, kHudColor ) );
	mPromptLabel = canvas.add( std::make_unique< UiLabel >( "", UiScreen::promptPosition(), UI_TEXT_NORMAL, UI_ALIGN_CENTER, kPromptColor ) );
	mTankLabel = canvas.add( std::make_unique< UiLabel >( "", UiScreen::statusLinePosition( kTankLine ), UI_TEXT_NORMAL, UI_ALIGN_LEFT, kHudColor ) );
	mCarryLabel = canvas.add( std::make_unique< UiLabel >( "", UiScreen::statusLinePosition( kCarryLine ), UI_TEXT_NORMAL, UI_ALIGN_LEFT, kHudColor ) );
}

void TpsTestScene::updateInteraction() {
	if ( !mPlayer ) {
		return;
	}
	// 視線は前回の描画で置いたカメラから撃つ（目の位置・向きは draw で決まるため）。
	mInteractor.update( mCamera.eyePosition(), mCamera.viewDirection(), mPlayer.get(), &mInventory, InputInteract() );
	// 拾われた物は印を付けるだけ。実際に消えるのはティックの最後（update の flush）。
	for ( const auto& pickup : mPickups ) {
		if ( pickup->isTaken() ) {
			mPickups.destroyLater( pickup.get() );
		}
	}
	if ( mEscapeCar && mEscapeCar->isFueled() ) {
		SceneManager::instance().requestChange( SceneManager::SCENE_CLEAR );
	}
}

IInteractable* TpsTestScene::resolveInteractable( const HitWorld::Hit& hit ) {
	if ( !hit.blocker ) {
		return nullptr; // キャラ（ゾンビ）は使えない
	}
	if ( Pickup* pickup = mPickups.find( hit.blocker ) ) {
		return pickup;
	}
	if ( mEscapeCar && mEscapeCar->isBody( hit.blocker ) ) {
		return mEscapeCar.get();
	}
	return nullptr;
}

void TpsTestScene::drawHud() {
	char text[ kHudTextSize ];
	mPromptLabel->setText( mInteractor.prompt().c_str() );
	if ( mEscapeCar ) {
		std::snprintf( text, sizeof( text ), "Car fuel: %d/%d", mEscapeCar->fuelDelivered(), mEscapeCar->fuelRequired() );
		mTankLabel->setText( text );
	}
	int carried = mInventory.count( ITEM_FUEL );
	std::snprintf( text, sizeof( text ), "Carrying: %s", carried > 0 ? itemSpec( ITEM_FUEL ).name : "-" );
	mCarryLabel->setText( text );
	mHud.draw();
}

#ifdef _DEBUG
void TpsTestScene::toggleCollisionDebug() {
	if ( InputDebugCollision() ) {
		mShowCollision = !mShowCollision;
	}
}

bool TpsTestScene::debugRaycast( HitWorld::Hit* hit ) const {
	// 画面中央（視線）へ撃つ。自分の部位には当てない（目の位置は自分の頭の箱の中にあるため）。
	Vector3 delta = mCamera.viewDirection();
	delta *= kDebugRayLength;
	return mHitWorld.raycast( mCamera.eyePosition(), delta, mPlayer.get(), hit );
}

void TpsTestScene::despawnLookedAtZombie() {
	if ( !InputDebugDespawn() ) {
		return;
	}
	HitWorld::Hit hit;
	if ( !debugRaycast( &hit ) || !hit.target ) {
		return;
	}
	// 当たったのがゾンビなら、印を付けるだけ。実際に消えるのはティックの最後（update の flush）。
	const Zombie* zombie = mZombies.find( hit.target );
	if ( zombie ) {
		mZombies.destroyLater( zombie );
	}
}

void TpsTestScene::castDebugRay() {
	HitWorld::Hit hit;
	char text[ kDebugTextSize ];
	if ( debugRaycast( &hit ) ) {
		mCollisionDrawer.setRayHit( hit.point, hit.normal );
		float distance = kDebugRayLength * hit.time;
		if ( hit.target ) {
			std::snprintf( text, sizeof( text ), "Ray: Character %s  %.1fm", bodyPartName( hit.part ), distance );
		} else {
			std::snprintf( text, sizeof( text ), "Ray: World  %.1fm", distance );
		}
	} else {
		std::snprintf( text, sizeof( text ), "Ray: none" );
	}
	DebugScreen::instance().draw( kHelpColumn, kRayRow, text );
}

void TpsTestScene::drawCollisionDebug() {
	if ( mShowCollision ) {
		castDebugRay();
		// 自分の当たり判定は描かない（一人称では自分の体を囲む線が視界を塞ぐ）。
		mCollisionDrawer.draw( mWorld, mHitWorld, mCamera.projectionViewMatrix(), mPlayer.get(), mPlayer.get() );
	}
	DebugScreen::instance().draw( kHelpColumn, kCollisionRow,
		mShowCollision ? "Collision: ON  (F1)  Despawn: F2" : "Collision: OFF (F1)  Despawn: F2" );
}
#endif

void TpsTestScene::beginFrame() {
	// 入力は1回の描画につき1回だけ読む（Main は読まないので、シーンが自分で読む）。
	Pad::instance()->update();

	// 読み込み・構築・テクスチャのロードを進める（構築後もテクスチャのために毎回呼ぶ）。
	bool allBuilt = mCache.update();
	if ( !mReady ) {
		if ( !allBuilt ) {
			return; // ロード完了待ち
		}
		placeObjects();
		mReady = true;
	}

	updateCursorLock();
	// 視点はマウスが動いた分だけ回す。update の回数に合わせると、2回進めた描画で2倍回ってしまう。
	rotateCamera();
	readPlayerCommand( &mCommand );
	updateInteraction();
#ifdef _DEBUG
	toggleCollisionDebug();
	despawnLookedAtZombie();
#endif
}

void TpsTestScene::update() {
	if ( !mReady ) {
		return;
	}
	// 順番に意味がある: 向きを決める（beginFrame）→ その向きでキャラを動かし体を曲げる → 動いた後の頭の位置へ視点を置く（draw）。
	// 逆にすると W が1フレーム前の向きへの前進になり、視点も体に1フレーム遅れて、腕や銃が画面の中で揺れて見える。
	if ( mPlayer ) {
		mPlayer->update( mCommand, mCamera.yaw(), mCamera.pitch() );
		mCommand.holsterPressed = false; // 使ったので消す（同じ描画の間の次の update で、もう一度反応しないように）
	}
	for ( auto& zombie : mZombies ) {
		zombie->update();
	}
	updateBodySounds();
	// このティックで消す印の付いたものを、全員の更新が済んでから消す（名簿からも外れる）。
	mZombies.flush();
	mPickups.flush();
}

void TpsTestScene::draw() {
	if ( !mReady ) {
		return;
	}
	placeCameraAtEye();
	updateListener();
	mPostProcess.beginScene();
	setupLights();
	drawWorld();
#ifdef _DEBUG
	drawCollisionDebug();
#endif
	mPostProcess.endScene();
	drawHud();
	// 直して保存すれば自動で作り直されるので、止めずに理由だけ出しておく。
	// 画面に出せるのは1行なので、写す処理（PostProcess）→ キャラ（SkinningPipeline）の順に、先に見つかった方を出す。
	const char* shaderError = mPostProcess.errorMessage();
	if ( shaderError[ 0 ] == '\0' ) {
		shaderError = mCache.skinning().errorMessage();
	}
	if ( shaderError[ 0 ] != '\0' ) {
		drawErrorLine( kHelpColumn, kShaderErrorRow, shaderError );
	}

	DebugScreen::instance().draw( kHelpColumn, kHelpRow,
		GameLib::Input::Manager::instance().isCursorLocked()
			? "WASD: move  Shift: run  1: draw/holster  RMB: aim (tap=toggle, hold)  Mouse: look  E: use  Esc: release cursor"
			: "Click: lock cursor (mouse look)" );
	if ( mPlayer ) {
		char ground[ kDebugTextSize ];
		std::snprintf( ground, sizeof( ground ), "Floor: %s", groundMaterialName( mGround.materialAt( mPlayer->position() ) ) );
		DebugScreen::instance().draw( kHelpColumn, kGroundRow, ground );
	}
}
