#ifndef INCLUDED_BUILDING_H
#define INCLUDED_BUILDING_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/Render/StaticMesh/StaticMeshInstance.h"
#include "GameSrc/Render/StaticMesh/StaticMeshLods.h"
#include "GameSrc/Render/Culling/LodSelector.h"
#include "GameSrc/Hit/IRayBlocker.h"

class DrawCulling;
class StaticMeshData;
class CollisionWorld;

// 建物1軒。見た目（StaticMeshInstance）を持ち、移動の当たり判定（WallMesh）をその見た目の置き場所から作って名簿へ渡す
// 置き場所を決める入口がここ1か所なので、見た目と当たり判定がずれない
// 当たり判定は名簿（CollisionWorld）が持つ。ここには残さない（1軒で数百のかけらがあり、二重に持つと建物の数だけ無駄になるため）
// 移動の当たり判定は、見た目の三角形を体の高さの範囲で切り取ったもの（MeshWallBuilder）
// 箱1つで囲むのと違い、へこんだ入口には入れて、斜めの壁には沿って滑る。足元の歩道や頭より上の庇はふさがない
// 中に入れるモデルに替えれば、そのまま中に入れる（壁の三角形だけがふさぐので、中は空いている）
// 弾も見た目の三角形そのもので止める（IRayBlocker）。こちらは高さで切らず、全部の三角形が相手
class Building : public IRayBlocker{
public:
	// shapes: 細かさの段ごとの形と見た目（元の形 shapes.full() で当たり判定を作る）。元の形は isReady() 済みであること
	//         形はこの建物より長く生きていること（MeshCache が持つ）
	// position / angleY: 置き場所（地面の上）と向き（度）
	// material: 壁の材質（着弾音に使う）。何でできた建物かは置く側が決める
	Building( const StaticMeshLods& shapes, const GameLib::Math::Vector3& position, float angleY, GroundMaterial material );

	// 移動の当たり判定を作って world へ渡す（以後は world が持つ）。建物1軒につき1回だけ呼ぶ（呼ぶたびに登録が増える）
	// world: 所有権は受け取らない。nullptr 不可
	void addCollidersTo( CollisionWorld* world ) const;
	void draw( const DrawCulling& culling ) const;						// culling（映る範囲・描く距離・小ささ。DrawCulling）で描くと決まった時だけ描く
	bool shows( const DrawCulling& culling ) const;						// culling で描くと決まっているか。決まっていれば mesh() を StaticMeshBatch へ足してまとめて描く
	const StaticMeshInstance& mesh() const;								// 見た目（元の形と置き場所）。まとめて描く（StaticMeshBatch::add）ために読む
	const StaticMeshData* shapeFor( const DrawCulling& culling ) const;	// culling で描くときの細かさの段の形（DrawCulling::chooseLod）。目から見る絵なら段を選び直す
	const GameLib::Math::Vector3& position() const;						// 置き場所（地面の上）。描く順番を目に近い順に並べるのに使う
	// 弾の当たり判定（見た目の三角形で判定する）
	virtual bool raycast( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta,
		float* time, GameLib::Math::Vector3* normal ) const;
	virtual GroundMaterial surfaceMaterial( const GameLib::Math::Vector3& point ) const;	// 作るときに受け取った材質（どこに当たっても同じ）

private:
	StaticMeshInstance mMesh;
	StaticMeshLods mShapes;
	// 今描く細かさの段。描く側だけが使う状態（ゲームの進行には使わない）なので、const の draw から選び直せるように mutable
	mutable LodSelector mLod;
	GroundMaterial mMaterial;
};

#endif
