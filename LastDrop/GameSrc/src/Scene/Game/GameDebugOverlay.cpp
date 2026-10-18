#include "GameSrc/Scene/Game/GameDebugOverlay.h"
#include "GameSrc/Scene/Game/GameWorld.h"
#include "GameSrc/Player/PlayerCharacter.h"
#include "GameSrc/Player/FirstPersonCamera.h"
#include "GameSrc/Core/Pad.h"

#include "GameLib/GameLib.h"
#include "Framework.h" // DebugScreen.h が使う Framework の宣言
#include "GameLib/DebugScreen.h"
#include "GameLib/Math/Vector3.h"

#include <cstdio>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// ── DebugScreen の文字位置 ──
	const int kHelpColumn = 1;
	const int kHelpRow = 1;
	const int kGroundRow = 2;
	const int kShaderErrorRow = 4; // シェーダのコンパイルエラー（あるときだけ）
	const int kDebugTextSize = 96; // 表示の切り替えの1行（F1〜F3）が収まる長さ
	const int kErrorTextSize = 160; // コンパイラのエラーはファイル名と行番号が付いて長い
#ifdef _DEBUG
	const int kCollisionRow = 3; // 当たり判定の表示（F1）・落ちている物の箱の表示（F2）・ゾンビの体力の表示（F3）の ON/OFF
	const int kRayRow = 5;       // 画面中央から撃った確認用の線分が何に当たったか（当たり判定の表示中だけ）
	const float kDebugRayLength = 100.f; // 確認用の線分の長さ（m）。通りの先の建物まで届く長さ
#endif

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
}

GameDebugOverlay::GameDebugOverlay()
#ifdef _DEBUG
	: mShowCollision( false ),
	mShowPickups( false ),
	mShowZombieHealth( false )
#endif
	{
}

void GameDebugOverlay::drawText( const TextState& state ) const {
	// 直して保存すれば自動で作り直されるので、止めずに理由だけ出しておく。
	if ( state.shaderError[ 0 ] != '\0' ) {
		drawErrorLine( kHelpColumn, kShaderErrorRow, state.shaderError );
	}
	DebugScreen::instance().draw( kHelpColumn, kHelpRow,
		state.cursorLocked
			? "OnCursor"
			: "Click: lock cursor" );
	if ( state.floorMaterial ) {
		char ground[ kDebugTextSize ];
		std::snprintf( ground, sizeof( ground ), "Floor: %s", state.floorMaterial );
		DebugScreen::instance().draw( kHelpColumn, kGroundRow, ground );
	}
}

#ifdef _DEBUG
void GameDebugOverlay::handleInput() {
	if ( InputDebugCollision() ) {
		mShowCollision = !mShowCollision;
	}
	if ( InputDebugPickupCollision() ) {
		mShowPickups = !mShowPickups;
	}
	if ( InputDebugZombieHealth() ) {
		mShowZombieHealth = !mShowZombieHealth;
	}
}

void GameDebugOverlay::addZombieHealth( const GameWorld& world, const FirstPersonCamera& camera, UiRenderer* renderer ) const {
	if ( mShowZombieHealth ) {
		mZombieHealthBars.add( world, camera, renderer );
	}
}

bool GameDebugOverlay::raycast( const GameWorld& world, const FirstPersonCamera& camera, HitWorld::Hit* hit ) const {
	Vector3 delta = camera.viewDirection();
	delta *= kDebugRayLength;
	return world.hitWorld().raycast( camera.eyePosition(), delta, world.player(), hit );
}

void GameDebugOverlay::castRay( const GameWorld& world, const FirstPersonCamera& camera ) {
	HitWorld::Hit hit;
	char text[ kDebugTextSize ];
	if ( raycast( world, camera, &hit ) ) {
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

void GameDebugOverlay::drawCollision( const GameWorld& world, const FirstPersonCamera& camera ) {
	if ( mShowCollision ) {
		castRay( world, camera );
		// 自分の当たり判定は描かない（一人称では自分の体を囲む線が視界を塞ぐ）。
		mCollisionDrawer.draw( world.collisionWorld(), world.hitWorld(), camera.projectionViewMatrix(), world.player(), world.player() );
	}
	if ( mShowPickups ) {
		// 落ちている物の箱を、手前の物に隠れない線で描く（壁の向こうの缶の場所が分かる）。拾われた物は世界が渡さない。
		mPickupBoxes.clear();
		world.appendPickupColliders( &mPickupBoxes );
		mCollisionDrawer.drawSeeThrough( mPickupBoxes, camera.projectionViewMatrix() );
	}
	char text[ kDebugTextSize ];
	std::snprintf( text, sizeof( text ), "Collision: %s (F1)  Fuel boxes: %s (F2)  Zombie HP: %s (F3)",
		mShowCollision ? "ON " : "OFF", mShowPickups ? "ON " : "OFF", mShowZombieHealth ? "ON " : "OFF" );
	DebugScreen::instance().draw( kHelpColumn, kCollisionRow, text );
}
#endif
