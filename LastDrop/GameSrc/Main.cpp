#include "Framework.h"
#include "GameLib/DebugScreen.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Math/Vector4.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Vector2.h"
#include "GameLib/Math/Functions.h"
#include "GameLib/Scene/PrimitiveRenderer.h"
#include "GameLib/Graphics/Manager.h"
using namespace GameLib;

#include <limits>
using namespace std;

#include "Pad.h"
#include "Player.h"
#include "Target.h"
#include "Arena.h"
#include "Stage.h"

// ────────────────────────────────────────────────
// 企画書 v4「ワイヤーを使ったアクション部分」の検証シーン。
//   ・Player：地上移動＋回避＋拘束ワイヤー引き寄せ＋射撃で弱化（新規キャラコン）
//   ・Target：静止ダミー（対象の最小スタンドイン）
//   ・Arena ：壁・床のアンカー判定（見た目は Stage が描画）
// 実装しないもの：捕縛（拘束）／任務分岐／ボスAI。
// ────────────────────────────────────────────────
Player* gPlayer;
Target* gTarget;
Stage*  gStage;
Arena   gArena;
int  gTime;
bool gInitialized = false;
const int TIME_LIMIT = 90; // 90秒。

Vector3 gAmbient( 0.2f, 0.3f, 0.4f );
GameLib::Scene::PrimitiveRenderer gPrimitiveRenderer;

const Vector3 kPlayerStart( 0.0f, 0.0f, 0.0f );
const Vector3 kTargetStart( 0.0f, 0.0f, 30.0f );

void drawRect( Scene::PrimitiveRenderer& pr, Vector2* p, unsigned c1, unsigned c2 ){
	Vector4 p4[ 4 ];
	for ( int i = 0; i < 4; ++i ){
		p4[ i ].x = p[ i ].x;
		p4[ i ].y = p[ i ].y;
		p4[ i ].z = 0.0;
		p4[ i ].w = 1.0;
	}
	pr.addTransformedTriangle( p4[ 0 ], p4[ 1 ], p4[ 2 ], c1, c1, c2 );
	pr.addTransformedTriangle( p4[ 3 ], p4[ 1 ], p4[ 2 ], c2, c1, c2 );
}

// シーンの生成。ゲームオーバー後の再挑戦でも呼ぶ。
void resetScene(){
	SAFE_DELETE( gPlayer );
	SAFE_DELETE( gTarget );
	gPlayer = NEW Player( kPlayerStart );
	gTarget = NEW Target( kTargetStart );
	gTime = 0;
}

namespace GameLib{

void Framework::update(){
	if ( !gInitialized ){
		gPrimitiveRenderer = Scene::PrimitiveRenderer::create( 1000, 100 );
		Pad::create();
		gStage = NEW Stage();
		gPlayer = 0;
		gTarget = 0;
		resetScene();
		gInitialized = true;
	}

	int hp        = gPlayer->getHitPoint();
	int targetHp  = gTarget->getHitPoint();
	bool gameOver = ( hp <= 0 ) || gTarget->isDown() || ( gTime >= 60 * TIME_LIMIT );

	if ( !gameOver ){
		Pad::instance()->update();
		gPlayer->update( gArena, gTarget );
		++gTime;
	}

	// カメラ行列（プレイヤー基準）。
	Matrix44 pvm;
	pvm.setPerspectiveTransform( 45.f,
		static_cast< float >( width() ),
		static_cast< float >( height() ),
		1.f,
		10000.f );
	Matrix34 vm;
	Vector3 eyePosition;
	gPlayer->getView( &vm, &eyePosition );
	pvm *= vm;

	// グローバルライティング設定。
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setProjectionViewMatrix( pvm );
	gm.setEyePosition( eyePosition );
	gm.setLightingMode( Graphics::LIGHTING_PER_PIXEL );
	gm.setAmbientColor( gAmbient );
	gm.setLightColor( 0, Vector3( 0.7f, 0.4f, 0.2f ) );
	Vector3 lightPos[ 4 ];
	lightPos[ 0 ].set( 100000.f, 100000.f, 100000.f );
	gm.setLightIntensity( 0, lightPos[ 0 ].length() );

	// プレイヤーの弾の位置から近い順にライト3つを選ぶ（弾を光らせる）。
	float nearestDistance[ 3 ];
	int   nearestIndex[ 3 ];
	for ( int i = 0; i < 3; ++i ){
		nearestDistance[ i ] = numeric_limits< float >::max();
		nearestIndex[ i ] = -1;
	}
	for ( int j = 0; j < gPlayer->getBulletNumber(); ++j ){
		const Vector3* p = gPlayer->getBulletPosition( j );
		if ( !p ){ continue; }
		Vector3 t;
		t.setSub( *p, eyePosition );
		float d = t.squareLength();
		if ( d < nearestDistance[ 0 ] ){
			nearestDistance[ 2 ] = nearestDistance[ 1 ]; nearestDistance[ 1 ] = nearestDistance[ 0 ]; nearestDistance[ 0 ] = d;
			nearestIndex[ 2 ] = nearestIndex[ 1 ]; nearestIndex[ 1 ] = nearestIndex[ 0 ]; nearestIndex[ 0 ] = j;
		}else if ( d < nearestDistance[ 1 ] ){
			nearestDistance[ 2 ] = nearestDistance[ 1 ]; nearestDistance[ 1 ] = d;
			nearestIndex[ 2 ] = nearestIndex[ 1 ]; nearestIndex[ 1 ] = j;
		}else if ( d < nearestDistance[ 2 ] ){
			nearestDistance[ 2 ] = d; nearestIndex[ 2 ] = j;
		}
	}
	for ( int i = 0; i < 3; ++i ){
		if ( nearestIndex[ i ] != -1 ){
			lightPos[ i + 1 ] = *gPlayer->getBulletPosition( nearestIndex[ i ] );
			gm.setLightColor( i + 1, Vector3( 0.2f, 0.4f, 1.f ) );
			gm.setLightIntensity( i + 1, 10.f );
		}else{
			gm.setLightIntensity( i + 1, 0.f );
		}
	}
	for ( int i = 0; i < 4; ++i ){
		gm.setLightPosition( i, lightPos[ i ] );
	}
	gm.setCullMode( Graphics::CULL_BACK );
	gStage->draw();
	gPlayer->draw();
	gTarget->draw();

	// ── ワイヤーを3D空間に太いビームで描く（発射〜引き寄せ〜残像の間）──
	// 1pxの線だと視認できないため、視線に正対する幅を持たせた帯（三角形2枚）で描く。
	gm.setDiffuseColor( Vector3( 1.f, 1.f, 1.f ) );
	gPrimitiveRenderer.setTransform( pvm );
	gPrimitiveRenderer.enableDepthTest( true );
	gPrimitiveRenderer.enableDepthWrite( false );
	gPrimitiveRenderer.setBlendMode( Graphics::BLEND_ADDITIVE );
	Vector3 wireFrom, wireTo;
	if ( gPlayer->getWireEndpoints( &wireFrom, &wireTo ) ){
		const float wireHalfWidth = 0.2f; // ワイヤーの半幅（メートル）。
		Vector3 seg;  seg.setSub( wireTo, wireFrom );
		Vector3 mid;  mid.setAdd( wireFrom, wireTo ); mid *= 0.5f;
		Vector3 view; view.setSub( mid, eyePosition );
		// 帯の幅方向＝ワイヤーと視線の両方に直交する向き（カメラに正対させる）。
		Vector3 side; side.setCross( seg, view );
		float sideLen = side.length();
		if ( sideLen < 0.0001f ){ // ワイヤーが視線と平行なときは上方向で代用する。
			side.setCross( seg, Vector3( 0.f, 1.f, 0.f ) );
			sideLen = side.length();
		}
		if ( sideLen > 0.0001f ){
			side *= wireHalfWidth / sideLen;
			Vector3 a0, a1, b0, b1;
			a0.setSub( wireFrom, side ); a1.setAdd( wireFrom, side );
			b0.setSub( wireTo,   side ); b1.setAdd( wireTo,   side );
			const unsigned cHand   = 0xff33ff88; // 手元側の色。
			const unsigned cAnchor = 0xffccffee; // アンカー側の色。
			gPrimitiveRenderer.addTriangle( a0, a1, b0, cHand, cHand, cAnchor );
			gPrimitiveRenderer.addTriangle( b0, a1, b1, cAnchor, cHand, cAnchor );
		}
	}

	// ── フロントエンド（HUD）──
	gPrimitiveRenderer.enableDepthTest( false );
	gPrimitiveRenderer.enableDepthWrite( false );
	gPrimitiveRenderer.setBlendMode( Graphics::BLEND_LINEAR );

	Vector2 p[ 4 ];
	unsigned c1, c2;
	float length;

	// プレイヤー体力バー（左上）。
	p[ 0 ].set( -0.9f, 0.95f ); p[ 1 ].set( -0.9f, 0.88f );
	p[ 2 ].set(  0.1f, 0.95f ); p[ 3 ].set(  0.1f, 0.88f );
	c1 = c2 = 0xff406080; drawRect( gPrimitiveRenderer, p, c1, c2 );
	length = static_cast< float >( hp ) / static_cast< float >( Player::mMaxHitPoint );
	p[ 2 ].set( -0.9f + length, 0.95f ); p[ 3 ].set( -0.9f + length, 0.88f );
	c1 = 0xff2288ff; c2 = 0xffccffff; drawRect( gPrimitiveRenderer, p, c1, c2 );

	// 対象（ダミー）体力バー＝弱化ゲージ（右上）。
	p[ 0 ].set( -0.1f, 0.95f ); p[ 1 ].set( -0.1f, 0.88f );
	p[ 2 ].set(  0.9f, 0.95f ); p[ 3 ].set(  0.9f, 0.88f );
	c1 = c2 = 0xff604040; drawRect( gPrimitiveRenderer, p, c1, c2 );
	length = static_cast< float >( targetHp ) / static_cast< float >( Target::mMaxHitPoint );
	p[ 0 ].set( 0.9f - length, 0.95f ); p[ 1 ].set( 0.9f - length, 0.88f );
	p[ 2 ].set( 0.9f, 0.95f );          p[ 3 ].set( 0.9f, 0.88f );
	c1 = 0xffff8822; c2 = 0xffffddcc; drawRect( gPrimitiveRenderer, p, c1, c2 );

	// 画面中央の照準（ワイヤー／射撃の狙い基準）。
	gPrimitiveRenderer.setBlendMode( Graphics::BLEND_ADDITIVE );
	c1 = c2 = 0x88ffffff;
	p[ 0 ].set( -0.012f, 0.02f ); p[ 1 ].set( -0.012f, -0.02f );
	p[ 2 ].set(  0.012f, 0.02f ); p[ 3 ].set(  0.012f, -0.02f );
	drawRect( gPrimitiveRenderer, p, c1, c2 );
	p[ 0 ].set( -0.03f, 0.008f ); p[ 1 ].set( -0.03f, -0.008f );
	p[ 2 ].set(  0.03f, 0.008f ); p[ 3 ].set(  0.03f, -0.008f );
	drawRect( gPrimitiveRenderer, p, c1, c2 );

	// 残り時間バー（下）。
	gPrimitiveRenderer.setBlendMode( Graphics::BLEND_LINEAR );
	length = 1.9f * static_cast< float >( 60 * TIME_LIMIT - gTime ) / static_cast< float >( 60 * TIME_LIMIT );
	p[ 0 ].set( -0.95f, -0.95f ); p[ 1 ].set( -0.95f, -0.9f );
	p[ 2 ].set(  0.95f, -0.95f ); p[ 3 ].set(  0.95f, -0.9f );
	c1 = c2 = 0x80404040; drawRect( gPrimitiveRenderer, p, c1, c2 );
	p[ 2 ].set( -0.95f + length, -0.95f ); p[ 3 ].set( -0.95f + length, -0.9f );
	gPrimitiveRenderer.setBlendMode( Graphics::BLEND_ADDITIVE );
	c1 = 0x80ff8888; c2 = 0x8088ffff; drawRect( gPrimitiveRenderer, p, c1, c2 );

	// 操作ガイド。
	DebugScreen::instance().draw( 1, 1, "WASD:move  Q/RightClick:wire  Space:dodge  LeftClick:fire" );

	// ゲームオーバー／クリア判定。
	if ( gameOver ){
		if ( gTarget->isDown() ){
			DebugScreen::instance().draw( 30, 5, "TARGET DOWN. press FIRE" );
		}else{
			DebugScreen::instance().draw( 30, 5, "Game Over. press FIRE" );
		}
		if ( Pad::instance()->isTriggered( Pad::FIRE, 0 ) ){
			resetScene(); // 次フレームの各 Batch::draw が頂点/インデクス/テクスチャを貼り直す。
		}
	}
	gPrimitiveRenderer.draw();

	// 終了処理。
	if ( isEndRequested() ){
		if ( gStage ){
			Pad::destroy();
			SAFE_DELETE( gPlayer );
			SAFE_DELETE( gTarget );
			SAFE_DELETE( gStage );
			gPrimitiveRenderer.release();
		}
	}
}
}
