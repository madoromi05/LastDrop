#include "GameSrc/Component/LightComponent.h"
#include "GameSrc/Component/ILightSocket.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/Math/AngleMath.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

LightComponent::LightComponent( LightPreset preset ) :
	mPreset( preset ),
	mOn( LightPresets::startsOn( preset ) ),
	mToggledThisTick( false ),
	mSocket( nullptr ) {
}

void LightComponent::update( bool togglePressed ) {
	mToggledThisTick = togglePressed;
	if ( togglePressed ) {
		mOn = !mOn;
	}
}

bool LightComponent::toggledThisTick() const {
	return mToggledThisTick;
}

bool LightComponent::isOn() const {
	return mOn;
}

void LightComponent::setOn( bool on ) {
	mOn = on;
}

void LightComponent::setSocket( const ILightSocket* socket ) {
	mSocket = socket;
}

LightPreset LightComponent::preset() const {
	return mPreset;
}

bool LightComponent::castsShadow() const {
	return LightPresets::castsShadow( mPreset );
}

bool LightComponent::lightSource( LightSource* out ) const {
	ASSERT( out && "LightComponent : out is null." );
	if ( !mOn ) {
		return false;
	}
	Vector3 position;
	Vector3 direction;
	if ( mSocket ) {
		if ( !mSocket->lightSocket( &position, &direction ) ) {
			return false;
		}
	} else {
		if ( LightPresets::needsSocket( mPreset ) ) {
			return false; // 持たれていない（地面に落とした銃は、置き場所があっても照らさない）
		}
		const TransformComponent* transform = getComponent< TransformComponent >(); // 外せる部品（無ければ光の出る所が無い）
		if ( !transform ) {
			return false;
		}
		position = transform->position();
		direction = AngleMath::forwardOf( transform->angleY() );
	}
	*out = LightPresets::make( mPreset, position, direction );
	return true;
}
