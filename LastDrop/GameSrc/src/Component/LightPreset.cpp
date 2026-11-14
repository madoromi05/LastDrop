#include "GameSrc/Component/LightPreset.h"
#include "GameSrc/Render/Lighting/LightingParams.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib::Math;

namespace{
	LightSource makeStreetLamp( const Vector3& foot ) {
		const LightingParams& params = lightingParams();
		Vector3 head( foot.x, foot.y + params.streetLightHeight, foot.z ); // 柱の頭（ランタンのガラスの中）
		return pointLight( head, params.streetLightColor, params.streetLightIntensity, params.streetLightRange );
	}

	LightSource makeFlashlight( const Vector3& muzzle, const Vector3& direction ) {
		const LightingParams& params = lightingParams();
		Vector3 position;
		position.setMadd( muzzle, direction, params.flashlightOffset );
		float innerAngle = min( params.flashlightInnerAngle, params.flashlightOuterAngle ); // F6 で内側を外側より大きくしても止まらないよう、内側は外側までに抑える
		LightSource light = spotLight( position, direction, params.flashlightColor, params.flashlightIntensity, params.flashlightRange,
			innerAngle, params.flashlightOuterAngle );
		light.nearDistance = params.flashlightNearDistance;
		return light;
	}
}

bool LightPresets::startsOn( LightPreset preset ) {
	return preset == LightPreset::StreetLamp;
}

bool LightPresets::castsShadow( LightPreset preset ) {
	return preset == LightPreset::Flashlight;
}

bool LightPresets::needsSocket( LightPreset preset ) {
	return preset == LightPreset::Flashlight;
}

LightSource LightPresets::make( LightPreset preset, const Vector3& position, const Vector3& direction ) {
	switch ( preset ) {
	case LightPreset::StreetLamp:
		return makeStreetLamp( position );
	case LightPreset::Flashlight:
		return makeFlashlight( position, direction );
	default:
		ASSERT( false && "LightPresets::make : unknown preset." );
		return LightSource();
	}
}
