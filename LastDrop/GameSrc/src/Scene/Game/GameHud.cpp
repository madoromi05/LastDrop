#include "GameSrc/Scene/Game/GameHud.h"
#include "GameSrc/UI/UiLabel.h"
#include "GameSrc/Interaction/FuelTankComponent.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/GameLib.h"

#include <cstdio>
#include <memory>

namespace{
	const unsigned kHudColor = VertexColor::pack( 0.95f, 0.95f, 0.95f );
	const int kTankLine = 0;  // 左下から数えた行（0 が一番下）
	const int kHudTextSize = 64;
}

GameHud::GameHud( UiRenderer* renderer ) :
	mScreen( renderer ),
	mTankLabel( nullptr ) {
	UiCanvas& canvas = mScreen.canvas();
	mTankLabel = canvas.add( std::make_unique< UiLabel >( "", UI_ANCHOR_LEFT, UiScreen::statusLinePosition( kTankLine ), UI_TEXT_NORMAL, UI_ALIGN_LEFT, kHudColor ) );
}

void GameHud::draw( const FuelTankComponent* car ) {
	char text[ kHudTextSize ];
	if ( car ) {
		std::snprintf( text, sizeof( text ), "Car fuel: %d/%d", car->fuelDelivered(), car->fuelRequired() );
		mTankLabel->setText( text );
	}
	mScreen.draw();
}
