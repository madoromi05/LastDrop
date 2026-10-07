#ifndef INCLUDED_BODY_FOOTPRINT_H
#define INCLUDED_BODY_FOOTPRINT_H

#include "GameLib/Math/Vector3.h"
#include "BoxCollider.h"

class SkinnedMeshData;

// キャラの移動の当たり判定（体を真上から見て囲む長方形）。体の向きに合わせて回る。
// 大きさは、腰・胴・脚の骨に一番強く付いている頂点を、レスト姿勢で真上から囲んで決める（モデルを差し替えても合う）。
// 腕は T ポーズで横に伸びているので含めない（含めると横幅が腕の長さまで広がる）。
// 主人公もゾンビも同じ求め方。骨の名前は「頭の共通部分（"mixamorig:" など）＋骨の名前」で引く。
class BodyFootprint{
public:
	// data: 形と骨。所有権は受け取らない。nullptr 不可、isReady() 済み。大きさを求めるのに構築中だけ使う。
	// boneNamePrefix: 骨の名前の頭の共通部分。nullptr 不可。
	// 対象の頂点が見つからなければ、デバッグ出力して既定の大きさにする（キャラが出せなくならないように）。
	BodyFootprint( const SkinnedMeshData* data, const char* boneNamePrefix );

	// 足元の位置 position・向き angleY（度。SkinnedMeshInstance::setAngleY と同じ約束）に置いた長方形。
	BoxCollider place( const GameLib::Math::Vector3& position, float angleY ) const;

private:
	GameLib::Math::Vector3 mLocalCenter; // 長方形の中心（モデルの空間。足元から見た位置。y は使わない）
	float mHalfWidthX;                    // モデルの X 方向（体の横）の幅の半分（m）
	float mHalfWidthZ;                    // モデルの Z 方向（体の前後）の幅の半分（m）
	float mHeight;                        // 足元から頭のてっぺんまで（m。レスト姿勢）。当たり判定の表示にだけ使う
};

#endif
