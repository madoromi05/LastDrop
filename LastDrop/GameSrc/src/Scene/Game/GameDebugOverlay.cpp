#include "GameSrc/Scene/Game/GameDebugOverlay.h"
#include "GameSrc/Scene/Game/GameWorld.h"
#include "GameSrc/Player/PlayerCharacter.h"
#include "GameSrc/Player/FirstPersonCamera.h"
#include "GameSrc/Core/Pad.h"
#include "GameSrc/UI/UiScreen.h"
#ifdef ZF_EDITOR
#include "GameSrc/Scene/Game/PlayerBodyFigure.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/Render/VertexColor.h"
#endif

#include "GameLib/GameLib.h"
#include "Framework.h" // DebugScreen.h が使う Framework の宣言
#include "GameLib/DebugScreen.h"
#include "GameLib/Math/Vector3.h"

#include <cstdio>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// ── DebugScreen の文字位置（2026-10-21 ユーザー指定：左上から左下へ。左上は体の図を置く）──
	// 左下の状態表示（Car fuel・Carrying。GameHud）の上に、下から順に積む。行は下から数えた番号（0 が一番下）。
	const int kHelpColumn = 1;
	const int kHelpLine = 0;
	const int kShaderErrorLine = 1; // シェーダのコンパイルエラー（あるときだけ）
	const int kDebugRowHeight = 16; // DebugScreen の 1 行の高さ（画素。Framework::drawDebugString）
	const int kStatusLines = 2;     // 左下の状態表示の行の数（この上に積む）
	const int kErrorTextSize = 160; // コンパイラのエラーはファイル名と行番号が付いて長い
#ifdef ZF_EDITOR
	const int kDebugTextSize = 128;
	const int kRayLine = 2;     // 画面中央から撃った確認用の線分が何に当たったか（当たり判定の表示中だけ）
	const int kSpawnerLine = 3; // ゾンビの立っている数・目標の数・次に湧くまで（ゾンビの体力の表示中だけ）
	const float kDebugRayLength = 100.f; // 確認用の線分の長さ（m）。通りの先の建物まで届く長さ
	// 自キャラの部位ごとの体力の数字（F3）。体の図の右に、上から PlayerBodyZone の順に並べる。
	const float kPlayerHealthGap = 8.f;         // 体の図の右端から数字までの間（画素）
	const float kPlayerHealthLineHeight = 18.f; // 1 行の高さ（画素）
	const unsigned kPlayerHealthColor = VertexColor::pack( 0.95f, 0.95f, 0.95f );
#endif

	// 下から line 番目（0 が一番下）の DebugScreen の行。状態表示の一番上の行の上端より上の、まるごと入る行から上へ数える。
	int rowFromBottom( int line ) {
		int statusTop = static_cast< int >( UiScreen::statusLinePosition( kStatusLines - 1 ).y );
		return statusTop / kDebugRowHeight - 1 - line;
	}

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
#ifdef ZF_EDITOR
	: mShowCollision( false ),
	mShowPickups( false ),
	mShowZombieHealth( false )
#endif
	{
}

void GameDebugOverlay::drawText( const TextState& state ) const {
	// 直して保存すれば自動で作り直されるので、止めずに理由だけ出しておく。
	if ( state.shaderError[ 0 ] != '\0' ) {
		drawErrorLine( kHelpColumn, rowFromBottom( kShaderErrorLine ), state.shaderError );
	}
	DebugScreen::instance().draw( kHelpColumn, rowFromBottom( kHelpLine ),
		state.cursorLocked
			? "OnCursor"
			: "Click: lock cursor" );
}

#ifdef ZF_EDITOR
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
	if ( !mShowZombieHealth ) {
		return;
	}
	mZombieHealthBars.add( world, camera, renderer );
	// 湧く仕組みの様子（立っている数 / 目標の数・次に湧くまで）。足りている間の秒数は補充の間隔のまま止まっている。
	const ZombieSpawner* spawner = world.zombieSpawner();
	if ( spawner ) {
		char text[ kDebugTextSize ];
		std::snprintf( text, sizeof( text ), "Zombies: %d/%d  Next spawn: %.1fs",
			spawner->standingCount(), spawner->targetCount(), spawner->secondsUntilNext() );
		DebugScreen::instance().draw( kHelpColumn, rowFromBottom( kSpawnerLine ), text );
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
	DebugScreen::instance().draw( kHelpColumn, rowFromBottom( kRayLine ), text );
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
	if ( mShowZombieHealth ) {
		mZombiePathDrawer.draw( world.zombies(), world.streets(), camera.projectionViewMatrix() );
	}
	// 表示の切り替えのキー（F1〜F4・F9）の一覧は出さない（2026-10-21 ユーザー指定）。キーはそのまま効く。
}
#endif

#ifdef ZF_EDITOR
void GameDebugOverlay::addPlayerHealth( const GameWorld& world, UiRenderer* renderer ) const {
	ASSERT( renderer && "GameDebugOverlay : renderer is null." );
	const PlayerCharacter* player = world.player();
	if ( !mShowZombieHealth || !player ) {
		return;
	}
	const PlayerHealth& health = player->health();
	UiRect figure = PlayerBodyFigure::bounds();
	Vector2 position( figure.right() + kPlayerHealthGap, figure.top );
	char text[ kDebugTextSize ];
	for ( int i = 0; i < static_cast< int >( PlayerBodyZone::Count ); ++i ) {
		PlayerBodyZone zone = static_cast< PlayerBodyZone >( i );
		std::snprintf( text, sizeof( text ), "%s %.0f/%.0f", playerBodyZoneName( zone ), health.part( zone ), health.maxPart( zone ) );
		renderer->addText( text, position, UI_TEXT_NORMAL, UI_ALIGN_LEFT, kPlayerHealthColor );
		position.y += kPlayerHealthLineHeight;
	}
}
#endif
