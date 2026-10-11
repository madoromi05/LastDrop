#ifndef INCLUDED_MESH_BOUNDS_BOX_H
#define INCLUDED_MESH_BOUNDS_BOX_H

#include "GameLib/Math/Vector3.h"
#include "BoxCollider.h"

class StaticMeshData;

// 動かないメッシュの頂点を真上から見て囲む箱（移動の当たり判定）を、見た目と同じ置き場所・向きで作る。状態は持たない。
// 足元の薄い物（歩いて越えられる高さの頂点だけでできた物）は囲まない（StaticMeshData::blockingBoundsMin / Max）。
// 置物（StaticProp）と拾える物（Pickup）が同じ作り方をするので、ここ1か所にまとめる（見た目と当たり判定がずれる原因を1か所に絞る）。
// 建物（Building）は箱1つでは粗いので、メッシュの形に沿った当たり判定を使う（MeshWallBuilder）。
class MeshBoundsBox{
public:
	// data: 形。nullptr 不可。isReady() 済みであること（頂点の範囲を使うため）。
	// position / angleY: StaticMeshInstance に渡すのと同じ置き場所と向き（度）。拡大率は 1 の前提
	// （大きさは変換時に揃える。tools/fbx_to_gamelib.py の scale）。
	static BoxCollider make( const StaticMeshData* data, const GameLib::Math::Vector3& position, float angleY );
};

#endif
