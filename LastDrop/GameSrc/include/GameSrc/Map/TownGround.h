#ifndef INCLUDED_TOWN_GROUND_H
#define INCLUDED_TOWN_GROUND_H

#include "GameSrc/Map/TownMap.h"
#include "GameSrc/Render/StaticMesh/StaticMeshInstance.h"
#include "GameSrc/Render/StaticMesh/StaticMeshData.h"
#include "GameSrc/Hit/IRayBlocker.h"
#include "GameSrc/Audio/IGroundSurface.h"
#include "GameSrc/Collision/IGroundHeight.h"
#include "GameSrc/Hit/GroundGrid.h"
#include <memory>
#include <vector>

class MeshCache;
class ViewFrustum;

// 町の地面（道路・歩道・ブロックの床・町の外の土）。TownMap の並べ方どおりに部品を置いて描く
// 部品は小さい板が何百枚もあるので、描くときは部品の種類ごとに1つのメッシュへまとめた物（StaticMeshMerger）を使う
// （1枚ずつ描くと、頂点は少ないのに描画の回数だけが部品の数になる）。置いた後は変えられない
// 1枚ずつの置き場所（mTiles）は、当たり判定と足元の材質のために残してある
// 移動を横から止める当たり判定は持たない（どれも歩いて越えられる高さなので移動を止めない）
// 代わりに、キャラが乗る面の高さを答える（IGroundHeight）。歩道の段差に乗るのに使う
// 弾・視線は、部品の見た目の三角形で止める（IRayBlocker）。部品ごとではなく、地面全体で1つの物として答える
// （どの部品に当たったかを使う処理が無いので、名簿を部品の数だけ増やさない）
// 足元の材質も答える（IGroundSurface）。足元にある部品の種類で決まる
// 部品の種類ごとの決めごと（ファイル・材質）はここだけが知っている（TownMap は「どの種類をどこに」だけを決める）
class TownGround : public IRayBlocker, public IGroundSurface, public IGroundHeight{
public:
	// 部品の読み込みを頼むだけ（非同期）。cache->update() が true を返してから place を呼ぶ
	// cache: 部品の設計図を借りる倉庫。所有権は受け取らない。nullptr 不可。この地面より長く生きていること
	explicit TownGround( MeshCache* cache );

	void place( const TownMap& map );				// map の並べ方どおりに部品を置く。構築に失敗した種類の部品は置かない（理由は MeshCache がデバッグ出力済み）
	void draw( const ViewFrustum& frustum ) const;	// 部品の種類ごとに、まとめた物を1回で描く。frustum: カメラに映る範囲（まとめた物がまるごと映らなければ描かない）
	// 弾の当たり判定（見た目の三角形で判定する）。線分が上から見て通るますにかかる部品だけを調べる（GroundGrid）
	bool raycast( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta,
		float* time, GameLib::Math::Vector3* normal ) const override;
	GroundMaterial surfaceMaterial( const GameLib::Math::Vector3& point ) const override;					// 弾が当たった所の材質。足元の材質（materialBelow）と同じ答え。部品のすき間なら町の外の土と同じ材質
	bool materialBelow( const GameLib::Math::Vector3& position, GroundMaterial* material ) const override;	// 足元の材質。position を通る縦の線分で、一番上にある部品を探し、その種類から決める。重さは raycast と同じ
	bool heightBelow( const GameLib::Math::Vector3& position, float* height ) const override;				// 足元の面の高さ。position を通る縦の線分（足元の MoveHeight::kStepOver 上から下）で、一番上の面を探す。重さは raycast と同じ

private:
	// 置いた部品1枚（当たり判定・足元の材質用。描くのには使わない）
	struct Tile{
		StaticMeshInstance mesh;
		TownPiece piece;
	};

	// 線分 origin〜origin+delta が一番早く当たる部品。当たらなければ nullptr
	// time / normal: 当たったときだけ書く出力先（意味は IRayBlocker::raycast と同じ）。nullptr 不可
	const Tile* nearestTile( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta,
		float* time, GameLib::Math::Vector3* normal ) const;
	// tile が線分に当たり、今までの一番早い当たり（nearest・time・normal）より早ければ書き換える
	void keepNearer( const Tile& tile, const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta,
		const Tile** nearest, float* time, GameLib::Math::Vector3* normal ) const;

	const StaticMeshData* mPieceData[ TOWN_PIECE_NUM ]; // MeshCache から借りる。添え字は TownPiece
	std::vector< Tile > mTiles;
	GroundGrid mGrid; // mTiles の添え字を、上から見たますに分けた物（place で 1 回作る）
	mutable std::vector< int > mCandidates; // nearestTile が使う置き場（聞くたびに確保し直さないため）
	// 描く用に、同じ種類の部品を1つへまとめたメッシュ（種類ごとに1つ。置けた種類のぶんだけ）
	// mMergedMeshes が借りるので、先に宣言する（＝後に破棄される）
	std::vector< std::unique_ptr< StaticMeshData > > mMergedData;
	std::vector< StaticMeshInstance > mMergedMeshes; // mMergedData を原点・向き 0・等倍に置いたもの（頂点がもうワールド座標のため）
};

#endif
