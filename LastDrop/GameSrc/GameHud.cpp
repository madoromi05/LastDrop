#include "GameHud.h"
#include "UiLabel.h"
#include "EscapeCar.h"
#include "Inventory.h"
#include "VertexColor.h"

#include "GameLib/GameLib.h"

#include <cstdio>
#include <memory>

namespace{
	const char* kCrosshair = "+";
	const unsigned kHudColor = VertexColor::pack( 0.95f, 0.95f, 0.95f );
	const unsigned kPromptColor = VertexColor::pack( 1.f, 0.85f, 0.3f ); // 燃料の色（黄）
	const int kTankLine = 1;  // 左下から数えた行（0 が一番下）
	const int kCarryLine = 0;
	const int kHudTextSize = 64;
}

GameHud::GameHud( UiRenderer* renderer ) :
	mScreen( renderer ),
	mPromptLabel( nullptr ),
	mTankLabel( nullptr ),
	mCarryLabel( nullptr ) {
	UiCanvas& canvas = mScreen.canvas();
	canvas.add( std::make_unique< UiLabel >( kCrosshair, UiScreen::screenCenter(), UI_TEXT_NORMAL, UI_ALIGN_CENTER, kHudColor ) );
	mPromptLabel = canvas.add( std::make_unique< UiLabel >( "", UiScreen::promptPosition(), UI_TEXT_NORMAL, UI_ALIGN_CENTER, kPromptColor ) );
	mTankLabel = canvas.add( std::make_unique< UiLabel >( "", UiScreen::statusLinePosition( kTankLine ), UI_TEXT_NORMAL, UI_ALIGN_LEFT, kHudColor ) );
	mCarryLabel = canvas.add( std::make_unique< UiLabel >( "", UiScreen::statusLinePosition( kCarryLine ), UI_TEXT_NORMAL, UI_ALIGN_LEFT, kHudColor ) );
}

void GameHud::draw( const char* prompt, const EscapeCar* car, const Inventory& inventory ) {
	ASSERT( prompt && "GameHud : prompt is null." );
	char text[ kHudTextSize ];
	mPromptLabel->setText( prompt );
	if ( car ) {
		std::snprintf( text, sizeof( text ), "Car fuel: %d/%d", car->fuelDelivered(), car->fuelRequired() );
		mTankLabel->setText( text );
	}
	int carried = inventory.count( ITEM_FUEL );
	std::snprintf( text, sizeof( text ), "Carrying: %s", carried > 0 ? itemSpec( ITEM_FUEL ).name : "-" );
	mCarryLabel->setText( text );
	mScreen.draw();
}
