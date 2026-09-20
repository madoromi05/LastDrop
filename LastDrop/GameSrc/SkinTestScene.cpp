#include "SkinTestScene.h"
#include "SkinnedMeshLoader.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/RefString.h"
#include "GameLib/Scene/Animation.h"
#include "GameLib/PseudoXml/Element.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/Enum.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Math/Vector3.h"

using namespace GameLib;

namespace{
	// ── 検証対象の設定。本物モデルを見るときはここだけ書き換える ──
	const char* kDataFile     = "Resource/character.txt"; // ゾンビFBXを変換したデータ
	const char* kTreeName     = "skel";                   // <Tree name>（コンバータ既定は "skel"）
	const char* kAnimName     = "walk"; // コンバータの anim_name。無ければレスト姿勢
	// ── カメラ。モデルの大きさに合わせて調整する（このモデルは身長約1.82・Y=0立ち）──
	const float kViewHeight   = 0.9f;   // 注視点の高さ（体の中央あたり）
	const float kViewDistance = 4.0f;   // 視点までの距離
	const float kModelYaw     = 0.0f;   // モデルをY回転させて見る角度(度)。背中が見えたら180
}

SkinTestScene::SkinTestScene() :
	mTime( 0.f ),
	mReady( false ) {
	mDocument = PseudoXml::Document::create( kDataFile );
}

SkinTestScene::~SkinTestScene() {
}

void SkinTestScene::buildOnce() {
	PseudoXml::ConstElement root = mDocument.root();
	// root() は <Container> を包む文書ルートのことがある。
	// engine の Container ローダと同じ規則で <Container> 要素まで降りる。
	if ( root.childNumber() == 1 && RefString( "Container" ) != root.name() ) {
		root = root.child( 0 );
	}
	// 同じ要素から Container(骨格/アニメ) と SkinnedMesh(頂点/重み) を構築する。
	mContainer = Scene::Container::create( root );
	mSkeleton = mContainer.createTree( kTreeName );
	SkinnedMeshLoader::load( &mMesh, root );                       // <SkinnedMesh> を読む
	mMesh.bindPose( mSkeleton );                                   // レスト姿勢で bindInverse を確定
	mSkeleton.setAnimation( mContainer.animation( kAnimName ) );   // 実行アニメ（無ければレスト姿勢）
	mDocument.release();                                       // 構築済みなので文書は解放
}

void SkinTestScene::update() {
	if ( !mReady ) {
		if ( !mDocument.isReady() ) {
			return; // ロード完了待ち
		}
		buildOnce();
		mReady = true;
	}

	// カメラ（帯全体が入る固定視点）。
	Graphics::Manager gm = Graphics::Manager::instance();
	Matrix44 projectionView;
	projectionView.setPerspectiveTransform( 45.f,
		static_cast< float >( gm.width() ),
		static_cast< float >( gm.height() ),
		1.f, 1000.f );
	Matrix34 view;
	view.setViewTransform(
		Vector3( 0.f, kViewHeight, kViewDistance ), // 視点
		Vector3( 0.f, kViewHeight, 0.f ),           // 注視点
		Vector3( 0.f, 1.f, 0.f ) );                 // 上方向
	projectionView *= view;
	gm.setProjectionViewMatrix( projectionView );
	gm.enableDepthTest( true );
	gm.enableDepthWrite( true );
	gm.setCullMode( Graphics::CULL_NONE ); // 裏返っても見えるように両面描画

	// ライティング（環境光＋1灯）。無テクスチャでも陰影で立体が見える。
	gm.setEyePosition( Vector3( 0.f, kViewHeight, kViewDistance ) );
	gm.setAmbientColor( Vector3( 0.30f, 0.30f, 0.35f ) );
	gm.setLightColor( 0, Vector3( 1.f, 1.f, 1.f ) );
	gm.setLightPosition( 0, Vector3( 3.f, 5.f, 5.f ) );
	gm.setLightIntensity( 0, 12.f ); // 明るさ ≒ 強度/距離
	for ( int i = 1; i < 4; ++i ) {
		gm.setLightIntensity( i, 0.f ); // 他のライトは消す
	}

	// モデルの向き（正面確認用にY回転できる）。
	mSkeleton.setAngle( Vector3( 0.f, kModelYaw, 0.f ) );

	// アニメ更新 → CPUスキニング → 描画。
	mSkeleton.updateAnimation();
	mMesh.update( mSkeleton );
	mMesh.draw();

	mTime += 1.f;
}
