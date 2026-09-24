#include "SkinTestScene.h"
#include "SkinnedMeshLoader.h"
#include "StaticMeshLoader.h"
#include "MeshLoaderUtil.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"
#include "GameLib/Scene/Animation.h"
#include "GameLib/PseudoXml/Element.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/Enum.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Math/Vector3.h"

using namespace GameLib;

namespace{
	// ── ゾンビ（身長約1.82・Y=0立ち・原点）──
	const char* kDataFile     = "Resource/character.txt"; // ゾンビFBXを変換したデータ（tools/configs/zombie.json）
	const char* kTreeName     = "skel";                   // <Tree name>（コンバータ既定は "skel"）
	const char* kAnimName     = "walk"; // コンバータの anim_name。無ければレスト姿勢
	const float kModelYaw     = 0.0f;   // モデルをY回転させて見る角度(度)。変換後のモデルは正面が +Z（カメラ側）

	// ── 建物（幅約14.3・奥行き約14.2・高さ約25.4(アンテナ込み)。底がY=0、中心が原点）──
	const char* kBuildingFile = "Resource/building.txt";  // 建物FBXを変換したデータ（tools/configs/building.json）
	// 正面の壁（中心から約7.2）がゾンビの約6m後ろに来るよう奥へ置く。
	const Vector3 kBuildingPosition( 0.f, 0.f, -13.f );
	// モデルの正面(窓と店の入口がある面)は +X 向き（Blender で面の向きを数えて一番作り込まれた面）。
	// rotateY(-90) で +X が +Z（カメラ側）を向く。
	const float kBuildingYaw   = -90.f;
	const float kBuildingScale = 1.f;   // FBX が実寸(m)なのでそのまま

	// ── カメラ。ゾンビを手前に、建物全体を背景に入れる構図 ──
	const Vector3 kEyePosition( 0.f, 1.6f, 16.f ); // 目線の高さ。建物全体が入るよう引く
	const Vector3 kLookAt( 0.f, 7.f, -6.f );       // 建物の正面の中ほど。少し見上げる
	const float kFieldOfViewY = 45.f;
	const float kNearClip     = 1.f;
	const float kFarClip      = 1000.f;
}

SkinTestScene::SkinTestScene() :
	mBuilding( &mBuildingData ),
	mTime( 0.f ),
	mReady( false ) {
	mDocument = PseudoXml::Document::create( kDataFile );
	mBuildingDocument = PseudoXml::Document::create( kBuildingFile );
}

SkinTestScene::~SkinTestScene() {
}

bool SkinTestScene::isAllLoaded() {
	// 両方の isReady() を毎回呼んでロードを並行して進める（&& の短絡で片方が止まらないように）。
	bool zombieLoaded = mDocument.isReady();
	bool buildingLoaded = mBuildingDocument.isReady();
	return zombieLoaded && buildingLoaded;
}

void SkinTestScene::buildOnce() {
	// ゾンビ: 同じ要素から Container(骨格/アニメ) と SkinnedMesh(頂点/重み) を構築する。
	PseudoXml::ConstElement root = MeshLoaderUtil::containerRoot( mDocument.root() );
	mContainer = Scene::Container::create( root );
	mSkeleton = mContainer.createTree( kTreeName );
	SkinnedMeshLoader::load( &mMesh, root );                       // <SkinnedMesh> を読む
	mMesh.bindPose( mSkeleton );                                   // レスト姿勢で bindInverse を確定
	Scene::Animation animation = mContainer.animation( kAnimName );
	if ( !animation ) {
		// 名前違いでもエラーにならず静止したままになるので、ここで知らせる。
		GameLib::cout << "SkinTestScene: animation not found: " << kAnimName << GameLib::endl;
	}
	mSkeleton.setAnimation( animation );                           // 実行アニメ（無ければレスト姿勢）
	mDocument.release();                                           // 構築済みなので文書は解放

	// 建物: 形と見た目を読み、置き場所を決める。
	StaticMeshLoader::load( &mBuildingData, MeshLoaderUtil::containerRoot( mBuildingDocument.root() ) );
	mBuilding.setPosition( kBuildingPosition );
	mBuilding.setAngleY( kBuildingYaw );
	mBuilding.setScale( kBuildingScale );
	mBuildingDocument.release();
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

void SkinTestScene::update() {
	if ( !mReady ) {
		if ( !isAllLoaded() ) {
			return; // ロード完了待ち
		}
		buildOnce();
		mReady = true;
	}

	setupCameraAndLights();

	// モデルを自動回転させて全周を見せる（回転が効いているかの確認も兼ねる）。
	// kModelYaw を基準に、時間で回す。
	mSkeleton.setAngle( Vector3( 0.f, kModelYaw + mTime, 0.f ) );

	// ゾンビ: アニメ更新 → CPUスキニング → 描画。
	mSkeleton.updateAnimation();
	mMesh.update( mSkeleton );
	mMesh.draw();

	// 建物: 動かないので描くだけ（テクスチャの非同期ロードは進める）。
	mBuildingData.updateLoading();
	mBuilding.draw();

	mTime += 1.f;
}
