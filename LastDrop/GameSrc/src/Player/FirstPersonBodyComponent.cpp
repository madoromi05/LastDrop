#include "GameSrc/Player/FirstPersonBodyComponent.h"
#include "GameSrc/Component/SkinnedModelComponent.h"
#include "GameSrc/World/GameObject.h"

using namespace GameLib::Math;

FirstPersonBodyComponent::FirstPersonBodyComponent( const SkinnedMeshData* data ) :
	mBody( data ),
	mModel( nullptr ) {
}

void FirstPersonBodyComponent::start() {
	SkinnedModelComponent* model = requireComponent< SkinnedModelComponent >();
	model->mesh().setPoseModifier( &mBody );
	mModel = model;
}

FirstPersonBody& FirstPersonBodyComponent::body() {
	return mBody;
}

const FirstPersonBody& FirstPersonBodyComponent::body() const {
	return mBody;
}

Vector3 FirstPersonBodyComponent::eyePosition() const {
	// FirstPersonBody が骨格の根の空間で求めた目を、置き場所・向きでワールドへ置く（頂点と同じ式）。
	Vector3 eye;
	mModel->mesh().rootMatrix().mul( &eye, mBody.eyePosition() );
	return eye;
}

void FirstPersonBodyComponent::setWeaponSway( float yawDegree, float pitchDegree ) {
	mBody.setSway( yawDegree, pitchDegree );
}
