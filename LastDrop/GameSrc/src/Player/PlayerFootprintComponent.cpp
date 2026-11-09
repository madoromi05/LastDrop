#include "GameSrc/Player/PlayerFootprintComponent.h"
#include "GameSrc/Player/Body/FirstPersonBodyComponent.h"
#include "GameSrc/Player/Body/HoldComponent.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/SkinnedModelComponent.h"
#include "GameSrc/Component/CharacterColliderComponent.h"
#include "GameSrc/World/GameObject.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 骨の名前の頭の共通部分（移動の当たり判定で骨を引くのに使う）。
	const char* kBoneNamePrefix = "mixamorig:";
	const float kFootprintPadding = 0.f; // 移動の当たり判定は体ぴったり（狭い所を通れるように）
	// 目から、移動の当たり判定の前の辺までの余白（m）。これより近くに壁が来ないので、視点が壁を越えて建物の中が見えることが無い。
	// カメラは目の前 5cm（FirstPersonCamera の kNearClip）より近くを描かず、その面の隅は目から約 7.7cm 離れている
	// （縦の視野角 60 度・16:9）。斜めに壁を見ても隅が壁に入らないよう、それより少し大きくする。
	const float kEyeToWallMargin = 0.1f;
	// 手に持った銃を囲む箱から、移動の当たり判定の辺までの余白（m）。銃口を壁に入れないため（銃口の光やフラッシュが壁の向こうへ出ないように）。
	// 銃口の少し前に置くもの（マズルフラッシュの光）まで壁の手前に収まるよう、目の余白と同じだけ取る。
	const float kWeaponToWallMargin = 0.1f;
}

PlayerFootprintComponent::PlayerFootprintComponent() :
	mTransform( nullptr ),
	mBody( nullptr ) {
}

void PlayerFootprintComponent::awake() {
	mFootprint = std::make_unique< BodyFootprint >( requireComponent< SkinnedModelComponent >()->data(), kBoneNamePrefix, kFootprintPadding );
}

void PlayerFootprintComponent::start() {
	mTransform = requireComponent< TransformComponent >();
	mBody = requireComponent< FirstPersonBodyComponent >();
	requireComponent< CharacterColliderComponent >()->setShape( this ); // 移動の当たり判定の形はここが答える
	// CharacterCollider が自分のポインタを覚えたので、自分にも外せない印を付ける。印は requireComponent で引かれた側にしか
	// 付かないため、自分で自分を引いて付ける（外すと、CharacterCollider が消えた形を指したまま移動を判定してしまう）。
	requireComponent< PlayerFootprintComponent >();
}

BoxCollider PlayerFootprintComponent::collider() const {
	const Vector3& position = mTransform->position();
	float angleY = mTransform->angleY();
	// 目は体（腰・胴・脚）より前にあり、見下ろすとさらに前へ出る。前の辺を、今の目の先（＋余白）まで伸ばす。
	// 目は骨格の根の空間（前方が +Z）で持っているので、z がそのまま足元から前への長さになる。
	float eyeReach = mBody->body().eyePosition().z + kEyeToWallMargin;
	// 持っている銃は体より前へ突き出ているので、銃を囲む箱（＋余白）も長方形に含める。銃口が壁に入らなくなる。
	// 銃の箱は今のポーズ（覗く・抱える・反動）から求めたものなので、構えを変えると長方形の大きさも変わる。
	// 持つ部品は外せる（無ければ素手として、体と目だけで囲む）。
	const HoldComponent* hold = getComponent< HoldComponent >();
	Vector3 low;
	Vector3 high;
	if ( !hold || !hold->isWeaponInHand() || !mBody->body().weaponBounds( &low, &high ) ) {
		return mFootprint->place( position, angleY, eyeReach );
	}
	low.x -= kWeaponToWallMargin;
	low.z -= kWeaponToWallMargin;
	high.x += kWeaponToWallMargin;
	high.z = Math::max( high.z + kWeaponToWallMargin, eyeReach );
	return mFootprint->place( position, angleY, low, high );
}
