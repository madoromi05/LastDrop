#include "GameSrc/Scene/Game/Hud/AmmoHud.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/UiAnchor.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/GameLib.h"
#include "Framework.h"
#include <cstdio>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 並べ方（座標の高さ 480 のときの画素。AmmoHud_Plan.txt Q5）。数の文字の幅は外から測れないので、
	// 「弾倉の数」「/」「予備の数」をそれぞれ決まった列の真ん中に置く（弾倉は 2 桁・予備は 3 桁が収まる間隔）。
	const float kMarginRight = 16.f;   // 板の右端から画面の右端まで
	const float kMarginBottom = 16.f;  // 板の下端から画面の下端まで
	const float kBoardWidth = 176.f;
	const float kBoardHeight = 56.f;
	const float kIconLeft = 8.f;       // 絵の左端（板の左端から）
	const float kIconSize = 48.f;      // 絵は正方形（カバンの絵と同じ 256×256 を縮める）
	const float kRoundsColumn = 88.f;  // 弾倉の数の真ん中（板の左端から）
	const float kSlashColumn = 124.f;  // 「/」の真ん中
	const float kReserveColumn = 150.f; // 予備の数の真ん中
	const float kTwice = 2.f;          // 画面の右端 = 中央の 2 倍
	const float kHalf = 0.5f;
	const int kTextSize = 16;

	// 色（r, g, b と、一番濃いときの不透明度。濃さ opacity を掛けて使う）。
	struct Color{
		float r;
		float g;
		float b;
		float a;
	};
	const Color kBoardColor = { 0.f, 0.f, 0.f, 0.45f };
	const Color kIconColor = { 1.f, 1.f, 1.f, 0.9f };
	const Color kTextColor = { 0.95f, 0.95f, 0.95f, 0.9f };
	const Color kFullColor = { 0.4f, 0.95f, 0.45f, 0.9f };  // 弾倉が満タン（ユーザー指定 2026-11-08）
	const Color kEmptyColor = { 0.95f, 0.25f, 0.2f, 0.9f }; // 弾倉が空（RaidHud の残り時間の警告と同じ赤）

	const char* kSlashText = "/";
	const char* kNumberFormat = "%d";

	unsigned packWithOpacity( const Color& color, float opacity ) {
		return VertexColor::pack( color.r, color.g, color.b, color.a * opacity );
	}

	// 弾倉の数の色（満タン → 緑、空 → 赤、それ以外 → 白）。
	const Color& roundsColor( int rounds, int capacity ) {
		if ( rounds >= capacity ) {
			return kFullColor;
		}
		return ( rounds <= 0 ) ? kEmptyColor : kTextColor;
	}
}

AmmoHud::AmmoHud( UiRenderer* renderer ) :
	mRenderer( renderer ) {
	ASSERT( mRenderer && "AmmoHud : renderer is null." );
}

void AmmoHud::add( const Ammo& ammo, float opacity ) {
	if ( opacity <= 0.f ) {
		return;
	}
	float screenRight = UiLayout::anchorX( UI_ANCHOR_CENTER ) * kTwice;
	float screenBottom = static_cast< float >( Framework::instance().height() );

	UiRect board;
	board.left = screenRight - kMarginRight - kBoardWidth;
	board.top = screenBottom - kMarginBottom - kBoardHeight;
	board.width = kBoardWidth;
	board.height = kBoardHeight;
	mRenderer->addRect( board, packWithOpacity( kBoardColor, opacity ) );

	// 絵は読み終えるまで出さない（数だけ出す）。
	if ( const Graphics::Texture* icon = mIcons.icon( ammo.weapon ) ) {
		UiRect iconRect;
		iconRect.left = board.left + kIconLeft;
		iconRect.top = board.top + ( kBoardHeight - kIconSize ) * kHalf;
		iconRect.width = kIconSize;
		iconRect.height = kIconSize;
		mRenderer->addImage( iconRect, *icon, packWithOpacity( kIconColor, opacity ) );
	}

	float middle = board.top + kBoardHeight * kHalf;
	char text[ kTextSize ];
	std::snprintf( text, sizeof( text ), kNumberFormat, ammo.rounds );
	mRenderer->addText( text, Vector2( board.left + kRoundsColumn, middle ), UI_TEXT_LARGE, UI_ALIGN_CENTER,
		packWithOpacity( roundsColor( ammo.rounds, ammo.capacity ), opacity ) );
	mRenderer->addText( kSlashText, Vector2( board.left + kSlashColumn, middle ), UI_TEXT_NORMAL, UI_ALIGN_CENTER,
		packWithOpacity( kTextColor, opacity ) );
	std::snprintf( text, sizeof( text ), kNumberFormat, ammo.reserve );
	mRenderer->addText( text, Vector2( board.left + kReserveColumn, middle ), UI_TEXT_NORMAL, UI_ALIGN_CENTER,
		packWithOpacity( kTextColor, opacity ) );
}
