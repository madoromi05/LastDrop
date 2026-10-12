#ifndef INCLUDED_MESH_BOUNDS_BOX_H
#define INCLUDED_MESH_BOUNDS_BOX_H

#include "GameLib/Math/Vector3.h"
#include "BoxCollider.h"

class StaticMeshInstance;

// 動かないメッシュの頂点を真上から見て囲む箱（移動の当たり判定）を、見た目と同じ置き場所・向きで作る。状態は持たない。
// 足元の薄い物（歩いて越えられる高さの頂点だけでできた物）は囲まない（StaticMeshData::blockingBoundsMin / Max）。
// 置物（StaticProp）と拾える物（Pickup）が同じ作り方をするので、ここ1か所にまとめる（見た目と当たり判定がずれる原因を1か所に絞る）。
// 建物（Building）は箱1つでは粗いので、メッシュの形に沿った当たり判定を使う（MeshWallBuilder）。
class MeshBoundsBox{
public:
	// mesh: 置いた見た目。その頂点の範囲と、置き場所・向き・拡大率から作る（見た目を拡大すれば箱も同じだけ広がる）。
	static BoxCollider make( const StaticMeshInstance& mesh );
};

#endif
