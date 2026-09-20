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
	// カメラの注視高さ。帯の中央あたり（skintest.txt の帯は y=0〜4）。
	const float kViewHeight = 2.0f;
}

SkinTestScene::SkinTestScene() :
	mTime( 0.f ),
	mReady( false ) {
	mDocument = PseudoXml::Document::create( "Resource/skintest.txt" );
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
	mSkeleton = mContainer.createTree( "skel" );
	SkinnedMeshLoader::load( &mMesh, root );                   // 手書き <SkinnedMesh> を読む
	mMesh.bindPose( mSkeleton );                               // レスト姿勢で bindInverse を確定
	mSkeleton.setAnimation( mContainer.animation( "bend" ) );  // 実行アニメを設定
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
		Vector3( 0.f, kViewHeight, 12.f ), // 視点
		Vector3( 0.f, kViewHeight, 0.f ),  // 注視点
		Vector3( 0.f, 1.f, 0.f ) );        // 上方向
	projectionView *= view;
	gm.setProjectionViewMatrix( projectionView );
	gm.enableDepthTest( true );
	gm.enableDepthWrite( true );
	gm.setCullMode( Graphics::CULL_NONE ); // 裏返っても見えるように両面描画

	// アニメ更新 → CPUスキニング → 描画。
	mSkeleton.updateAnimation();
	mMesh.update( mSkeleton );
	mMesh.draw();

	mTime += 1.f;
}
