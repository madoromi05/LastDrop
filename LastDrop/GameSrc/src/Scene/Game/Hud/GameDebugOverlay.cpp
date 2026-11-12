#include "GameSrc/Scene/Game/Hud/GameDebugOverlay.h"
#include "GameSrc/Scene/Game/World/GameWorld.h"
#include "GameSrc/Map/TownVisibility.h"
#include "GameSrc/Map/NavMesh.h"
#include "GameSrc/Player/PlayerHealthComponent.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/Player/Camera/FirstPersonCamera.h"
#include "GameSrc/UI/UiScreen.h"
#ifdef ZF_EDITOR
#include "GameSrc/DebugMode/DebugSwitches.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/ScreenProjection.h"
#include "GameSrc/Render/VertexColor.h"
#include "GameSrc/Interaction/Loot/LootContainerComponent.h"
#include "GameSrc/Component/TransformComponent.h"
#include <string>
#include <vector>
#endif

#include "GameLib/GameLib.h"
#include "Framework.h" // DebugScreen.h が使う Framework の宣言
#include "GameLib/DebugScreen.h"
#include "GameLib/Math/Vector3.h"

#include <cstdio>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// ── DebugScreen の文字位置（左上は体の図を置くので、左下に出す） ──
	// 左下に、下から順に積む。行は下から数えた番号（0 が一番下。0 は使っていない）
	const int kHelpColumn = 1;
	const int kShaderErrorLine = 1; // シェーダのコンパイルエラー（あるときだけ）
	const int kDebugRowHeight = 16; // DebugScreen の 1 行の高さ（画素。Framework::drawDebugString）
	const int kStatusLines = 2;     // 左下の状態表示の行の数（この上に積む）
	const int kErrorTextSize = 160; // コンパイラのエラーはファイル名と行番号が付いて長い
#ifdef ZF_EDITOR
	const int kDebugTextSize = 128;
	const int kRayLine = 2;     // 画面中央から撃った確認用の線分が何に当たったか（当たり判定の表示中だけ）
	const int kSpawnerLine = 3; // ゾンビの立っている数・目標の数・次に湧くまで（ゾンビの体力の表示中だけ）
	const int kPvsLine = 4;     // 簡易 PVS の目のいるますと、隠した建物の数（ゾンビの体力の表示中だけ）
	const float kDebugRayLength = 100.f; // 確認用の線分の長さ（m）。通りの先の建物まで届く長さ
	// 自キャラの体力の数字と状態（F3）。画面の左上に 1 行（普段の画面には体力を出さないので、確認用にだけ見せる）
	const float kPlayerHealthLeft = 16.f; // 画面の左端からの間（画素）
	const float kPlayerHealthTop = 16.f;  // 画面の上端からの間（画素）
	const unsigned kPlayerHealthColor = VertexColor::pack( 0.95f, 0.95f, 0.95f );
	// 漁れる場所の中身（F3）。文字を出す高さ（足元から m）
	const float kLootLabelHeight = 1.6f;   // 車・ゴミ箱・木箱（車の屋根より少し上）
	const char* kLootEmptyText = "（空）";
	const char* kLootSeparator = "・";
	const char* kLootSearchedMark = "[済] "; // 開けた所
	const unsigned kLootTextColor = VertexColor::pack( 0.95f, 0.85f, 0.3f );
#endif

	// 下から line 番目（0 が一番下）の DebugScreen の行。状態表示の一番上の行の上端より上の、まるごと入る行から上へ数える
	int rowFromBottom( int line ) {
		int statusTop = static_cast< int >( UiScreen::statusLinePosition( kStatusLines - 1 ).y );
		return statusTop / kDebugRowHeight - 1 - line;
	}

	// コンパイラのエラー文の1行目を、ファイル名から先だけ画面に出す
	// 1行目は「フォルダ\ファイル名(行,列): error ...」の形で、フォルダまで出すと画面の幅が足りず、肝心の行番号と理由が見えないため
	// DebugScreen は改行を扱わないので2行目以降は出さない（全文はデバッグ出力にある）
	void drawErrorLine( int column, int row, const char* text ) {
		int lineEnd = 0;
		while ( text[ lineEnd ] != '\0' && text[ lineEnd ] != '\n' ) {
			++lineEnd;
		}
		// ファイル名の始まり＝行番号の括弧より前にある、最後のフォルダの区切りの次
		// 括弧より後（エラーの理由）にある '/' はファイル名と関係ないので見ない
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

GameDebugOverlay::GameDebugOverlay() {
}

void GameDebugOverlay::drawShaderError( const char* shaderError ) const {
	ASSERT( shaderError && "GameDebugOverlay : shaderError is null." );
	// 直して保存すれば自動で作り直されるので、止めずに理由だけ出しておく
	if ( shaderError[ 0 ] != '\0' ) {
		drawErrorLine( kHelpColumn, rowFromBottom( kShaderErrorLine ), shaderError );
	}
}

#ifdef ZF_EDITOR
void GameDebugOverlay::addZombieHealth( const GameWorld& world, const FirstPersonCamera& camera, UiRenderer* renderer ) const {
	if ( !debugSwitches().showZombieInfo ) {
		return;
	}
	mZombieHealthBars.add( world, camera, renderer );
	// 湧く仕組みの様子（立っている数 / 目標の数・次に湧くまで）。足りている間の秒数は補充の間隔のまま止まっている
	const ZombieSpawner* spawner = world.zombies().spawner();
	if ( spawner ) {
		char text[ kDebugTextSize ];
		std::snprintf( text, sizeof( text ), "Zombies: %d/%d  Next spawn: %.1fs",
			spawner->standingCount(), spawner->targetCount(), spawner->secondsUntilNext() );
		DebugScreen::instance().draw( kHelpColumn, rowFromBottom( kSpawnerLine ), text );
	}
	// 簡易 PVS。ますの外（町の外・建物の足元・高い所）では使わない
	const TownVisibility& visibility = world.town().visibility();
	char pvsText[ kDebugTextSize ];
	int cell = visibility.cellAt( camera.eyePosition() );
	if ( !pvsEnabled() ) {
		std::snprintf( pvsText, sizeof( pvsText ), "PVS: off" );
	} else if ( cell == TownVisibility::kNoCell ) {
		std::snprintf( pvsText, sizeof( pvsText ), "PVS: no cell (draw all)" );
	} else {
		int hidden = 0;
		int buildingNumber = static_cast< int >( world.town().buildings().size() );
		for ( int b = 0; b < buildingNumber; ++b ) {
			hidden += visibility.showsBuilding( cell, b ) ? 0 : 1;
		}
		std::snprintf( pvsText, sizeof( pvsText ), "PVS: cell %d  hidden buildings %d/%d", cell, hidden, buildingNumber );
	}
	DebugScreen::instance().draw( kHelpColumn, rowFromBottom( kPvsLine ), pvsText );
}

bool GameDebugOverlay::raycast( const GameWorld& world, const FirstPersonCamera& camera, HitWorld::Hit* hit ) const {
	Vector3 delta = camera.viewDirection();
	delta *= kDebugRayLength;
	return world.hitWorld().raycast( camera.eyePosition(), delta, world.playerTarget(), hit );
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
	if ( debugSwitches().showCollision ) {
		castRay( world, camera );
		mCollisionDrawer.draw( world.collisionWorld(), world.hitWorld(), camera.projectionViewMatrix(), world.playerBody(), world.playerTarget() );	// 自分の当たり判定は描かない（一人称では自分の体を囲む線が視界を塞ぐ）
	}
	if ( debugSwitches().showPickups ) {
		// 落ちている物の箱を、手前の物に隠れない線で描く（壁の向こうの缶の場所が分かる）。拾われた物は世界が渡さない
		mPickupBoxes.clear();
		world.interaction().appendPickupColliders( &mPickupBoxes );
		mCollisionDrawer.drawSeeThrough( mPickupBoxes, camera.projectionViewMatrix() );
	}
	if ( debugSwitches().showZombieInfo ) {
		mZombiePathDrawer.draw( world.objects(), world.town().navMesh(), camera.projectionViewMatrix() );
	}
}

void GameDebugOverlay::drawLights( const LightList& lights, const FirstPersonCamera& camera ) {
	if ( debugSwitches().showZombieInfo ) {
		mLightDrawer.draw( lights, camera.projectionViewMatrix() );
	}
}
#endif

#ifdef ZF_EDITOR
void GameDebugOverlay::addPlayerHealth( const GameWorld& world, UiRenderer* renderer ) const {
	ASSERT( renderer && "GameDebugOverlay : renderer is null." );
	const GameObject* player = world.player();
	const PlayerHealthComponent* healthComponent = player ? player->getComponent< PlayerHealthComponent >() : nullptr;
	if ( !debugSwitches().showZombieInfo || !healthComponent ) {
		return; // 自キャラが居ない・体力の部品を外した（死なない）
	}
	const PlayerHealth& health = healthComponent->health();
	char text[ kDebugTextSize ];
	std::snprintf( text, sizeof( text ), "Health %.0f/%.0f %s", health.current(), health.maxHealth(), playerConditionName( health.condition() ) );
	renderer->addText( text, Vector2( kPlayerHealthLeft, kPlayerHealthTop ), UI_TEXT_NORMAL, UI_ALIGN_LEFT, kPlayerHealthColor );
}

void GameDebugOverlay::addLootContents( const GameWorld& world, const FirstPersonCamera& camera, UiRenderer* renderer ) const {
	ASSERT( renderer && "GameDebugOverlay : renderer is null." );
	if ( !debugSwitches().showZombieInfo ) {
		return;
	}
	Matrix44 projectionView = camera.projectionViewMatrix();
	world.objects().forEach< LootContainerComponent >( [ & ]( const LootContainerComponent& loot ) {
		const TransformComponent* transform = loot.getComponent< TransformComponent >();
		if ( !transform ) {
			return;
		}
		Vector3 position = transform->position();
		position.y += kLootLabelHeight;
		Vector2 screen;
		if ( !ScreenProjection::worldToScreen( projectionView, position, &screen ) ) {
			return; // カメラの後ろ
		}
		// 中身の名前を「・」でつなぐ（壁越しに出す。道順を考えるときの確認用）
		std::string text = loot.isSearched() ? kLootSearchedMark : "";
		std::vector< ItemStack > stacks = loot.contents().stacks();
		if ( stacks.empty() ) {
			text += kLootEmptyText;
		}
		for ( size_t i = 0; i < stacks.size(); ++i ) {
			if ( i > 0 ) {
				text += kLootSeparator;
			}
			text += itemSpec( stacks[ i ].item ).name;
		}
		renderer->addText( text.c_str(), screen, UI_TEXT_NORMAL, UI_ALIGN_CENTER, kLootTextColor );
	} );
}
#endif
