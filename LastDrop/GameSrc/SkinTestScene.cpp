#include "SkinTestScene.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/Enum.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Math/Vector3.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 置き場所（地面 Y=0 の上）と向き。
	struct Placement{
		float x;
		float z;
		float yawDegree; // 変換後のモデルは正面が +Z（カメラ側）。0 でカメラを向く
	};

	// ── ゾンビ（身長約1.82・Y=0立ち）──
	const char* kZombieFile = "Resource/character.txt"; // tools/configs/zombie.json で変換
	const char* kZombieAnimation = "walk";               // コンバータの anim_name
	const Placement kZombiePlacements[] = {
		{  0.0f,  0.0f,   0.f },
		{ -2.5f, -2.0f,  25.f },
		{  2.8f, -3.0f, -30.f },
	};
	// 体ごとに歩き出しの時刻をずらす（フレーム）。揃って歩くと同じモデルの使い回しが目立つため。
	const float kAnimationStartStep = 11.f;

	// ── 建物（幅約7.9・奥行き約7.8・屋根の高さ約10.8。足元の歩道は幅約11.2。底がY=0）──
	const char* kBuildingFile = "Resource/building.txt"; // tools/configs/building.json で変換
	// 正面(窓と店の入口)は +X 向きなので、rotateY(-90) で +Z（カメラ側）へ向ける。
	const float kBuildingYaw = -90.f;
	// 建物は tools/configs/building.json の scale で、ドアが人の背丈に合う大きさ（ドアの高さ約 2.0m）に縮めてある。
	// 足元の歩道の板は 11.16m 四方。板どうしが重なると同じ高さの面がちらつくので、それより少し広い 11.3m おきに並べる。
	// 奥行きは、歩道の手前の端が z = -2.85 に来る所（板の半分 5.58m ぶん奥）。
	const float kBuildingRowZ = -8.43f;
	const Placement kBuildingPlacements[] = {
		{  -22.6f, kBuildingRowZ, kBuildingYaw },
		{  -11.3f, kBuildingRowZ, kBuildingYaw },
		{    0.0f, kBuildingRowZ, kBuildingYaw },
		{   11.3f, kBuildingRowZ, kBuildingYaw },
		{   22.6f, kBuildingRowZ, kBuildingYaw },
	};

	// ── カメラ。手前にゾンビ、奥に建物の並んだ通りを見る ──
	const Vector3 kEyePosition( 0.f, 1.8f, 20.f );
	const Vector3 kLookAt( 0.f, 6.f, -6.f );
	const float kFieldOfViewY = 45.f;
	const float kNearClip     = 1.f;
	const float kFarClip      = 1000.f;
}

SkinTestScene::SkinTestScene() :
	mZombieData( 0 ),
	mBuildingData( 0 ),
	mReady( false ) {
	// 読み込みを頼むだけ（非同期）。同じファイルを何度頼んでも読み込みは1回。
	mZombieData = mCache.skinnedMesh( kZombieFile );
	mBuildingData = mCache.staticMesh( kBuildingFile );
}

SkinTestScene::~SkinTestScene() {
}

void SkinTestScene::placeObjects() {
	// 構築に失敗したデータ（理由は MeshCache がデバッグ出力済み）は置かない。
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
}

void SkinTestScene::setupCameraAndLights() {
	Graphics::Manager gm = Graphics::Manager::instance();
	Matrix44 projectionView;
	projectionView.setPerspectiveTransform( kFieldOfViewY,
		static_cast< float >( gm.width() ),
		static_cast< float >( gm.height() ),
		kNearClip, kFarClip );
	Matrix34 view;
	view.setViewTransform( kEyePosition, kLookAt, Vector3( 0.f, 1.f, 0.f ) );
	projectionView *= view;
	gm.setProjectionViewMatrix( projectionView );
	gm.enableDepthTest( true );
	gm.enableDepthWrite( true );
	gm.setCullMode( Graphics::CULL_NONE ); // 裏返っても見えるように両面描画

	// ライティング（環境光＋2灯）。暗い肌テクスチャでも見えるよう明るめにする。
	gm.setEyePosition( kEyePosition );
	gm.setAmbientColor( Vector3( 0.45f, 0.45f, 0.45f ) ); // 環境光
	gm.setLightColor( 0, Vector3( 1.f, 1.f, 1.f ) );
	gm.setLightPosition( 0, Vector3( 2.f, 4.f, 6.f ) );   // カメラ側（正面）から
	gm.setLightIntensity( 0, 18.f );
	gm.setLightColor( 1, Vector3( 0.5f, 0.5f, 0.5f ) ); // 無彩色フィル（青被り防止）
	gm.setLightPosition( 1, Vector3( -3.f, 3.f, -3.f ) ); // 反対側からのフィル
	gm.setLightIntensity( 1, 10.f );
	gm.setLightIntensity( 2, 0.f );
	gm.setLightIntensity( 3, 0.f );
}

void SkinTestScene::beginFrame() {
	// 読み込み・構築・テクスチャのロードを進める（構築後もテクスチャのために毎回呼ぶ）。
	bool allBuilt = mCache.update();
	if ( !mReady && allBuilt ) {
		placeObjects();
		mReady = true;
	}
}

void SkinTestScene::update() {
	// ゾンビ: 1体ずつアニメを進める（頂点の変形は描くときに1回だけ行われる）。
	for ( auto& zombie : mZombies ) {
		zombie->update();
	}
}

void SkinTestScene::draw() {
	if ( !mReady ) {
		return; // ロード完了待ち
	}
	setupCameraAndLights();
	for ( const auto& zombie : mZombies ) {
		zombie->draw();
	}
	// 建物: 動かないので描くだけ。
	for ( const StaticMeshInstance& building : mBuildings ) {
		building.draw();
	}
}
