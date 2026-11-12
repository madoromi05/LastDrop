#ifndef INCLUDED_BODY_FOOTPRINT_H
#define INCLUDED_BODY_FOOTPRINT_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/Collision/BoxCollider.h"

class SkinnedMeshData;

// キャラの移動の当たり判定（体を真上から見て囲む長方形）。体の向きに合わせて回る
// 大きさは、腰・胴・脚の骨に一番強く付いている頂点を、レスト姿勢で真上から囲んで決める（モデルを差し替えても合う）
// 腕は T ポーズで横に伸びているので含めない（含めると横幅が腕の長さまで広がる）
// 主人公もゾンビも同じ求め方。骨の名前は「頭の共通部分（"mixamorig:" など）＋骨の名前」で引く
class BodyFootprint{
public:
	// data: 形と骨。所有権は受け取らない。nullptr 不可、isReady() 済み。大きさを求めるのに構築中だけ使う
	// boneNamePrefix: 骨の名前の頭の共通部分。nullptr 不可
	// padding: 体を囲んだ長方形の四辺を、それぞれ外へ広げる幅（m。0 で体ぴったり）。体と体が触れるより手前で止めたいときに使う
	// 対象の頂点が見つからなければ、デバッグ出力して既定の大きさにする（キャラが出せなくならないように）。既定の大きさにも padding を足す
	BodyFootprint( const SkinnedMeshData* data, const char* boneNamePrefix, float padding );

	// 足元の位置 position・向き angleY（度。SkinnedMeshInstance::setAngleY と同じ約束）に置いた長方形
	BoxCollider place( const GameLib::Math::Vector3& position, float angleY ) const;
	// 上と同じだが、長方形の前の辺を、少なくとも forwardReach の所まで伸ばす（後ろ・横の辺は変えない）
	// forwardReach: 足元から体の前方へ測った長さ（m。モデルの空間の +Z）。体の前の辺より手前なら、上と同じ長方形になる
	// 一人称の目は体（腰・胴・脚）より前にあるので、目が壁を越えないよう、目の先まで当たり判定を伸ばすために使う
	BoxCollider place( const GameLib::Math::Vector3& position, float angleY, float forwardReach ) const;
	// 上と同じだが、体の長方形と、範囲 low〜high（モデルの空間。x と z だけ使う）の両方を囲む長方形にする
	// 体の外にはみ出した物（一人称で手に持った銃）にも、移動の当たり判定を持たせるために使う
	// 長方形は体の向きに合わせて回るので、範囲もモデルの空間（体の向き基準）で渡す
	BoxCollider place( const GameLib::Math::Vector3& position, float angleY,
		const GameLib::Math::Vector3& low, const GameLib::Math::Vector3& high ) const;

private:
	GameLib::Math::Vector3 mLocalCenter; // 長方形の中心（モデルの空間。足元から見た位置。y は使わない）
	float mHalfWidthX;                    // モデルの X 方向（体の横）の幅の半分（m）
	float mHalfWidthZ;                    // モデルの Z 方向（体の前後）の幅の半分（m）
};

#endif
