#ifndef INCLUDED_TOWN_CACHE_H
#define INCLUDED_TOWN_CACHE_H

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

class NavMesh;
class TownVisibility;
class TownMap;
class CollisionWorld;
class HitWorld;
class Building;

// 町の置き場所から作る重い物を、事前に書き出したファイルから読むか、起動中に作って持つ（GameResources が持つ）
// （出撃のたびに作ると、Debug で数秒かかるため）
//   ・ゾンビの歩ける床（NavMesh）：当たり判定に入る物・脱出の車の置き場所・床の縁を壁から離す長さで決まる
//   ・建物の隠れの表（TownVisibility）：建物の置き場所だけで決まる
// どちらも町の並び（TownMap。手直しを当てた後）から指紋（TownFingerprint）を求め、指紋ごとに 1 つ覚える
// 探す順：覚えた物 → ファイル（TownBakeFile。指紋が合うときだけ）→ 材料から作る
// 手直しで町が変わっても指紋が変わるので、古い床・表は使わない（同じ起動の中で作り直した物も混ざらない）
// 作るための材料（当たり判定の名簿・建物）は、頼んだ世界のものを使う。覚えた物を返すときは材料を見ない
class TownCache{
public:
	TownCache();
	~TownCache();
	TownCache( const TownCache& ) = delete;
	TownCache& operator=( const TownCache& ) = delete;

	// escapeCarSpot（TownMap::escapeCarSpots の番号）に車を置いた map の歩ける床。world: 作るときの材料（その町の動かない形）
	// agentRadius: 床の縁を壁から離す長さ（m。NavMesh::build）。指紋に入るので、F6 で余白を変えた後の出撃では作り直す
	// 返す床はこの持ち主が生きている間使える（作るのに失敗した床も返す。isReady が false で、ゾンビはまっすぐ寄るだけになる）
	NavMesh* navMesh( int escapeCarSpot, const TownMap& map, const CollisionWorld& world, float agentRadius );
	bool isNavMeshFromFile( int escapeCarSpot ) const;	// escapeCarSpot の床として最後に返した物を、ファイルから読んだか（まだ返していなければ false）
	// map の建物の隠れの表。buildings: 作る・読むときの材料（map から建てた建物の並び）
	// Debug では、作った・読んだ後に hitWorld で一度だけ確かめる（TownVisibilityCheck）
	const TownVisibility* visibility( const TownMap& map, const std::vector< Building >& buildings, const HitWorld& hitWorld );

#ifdef ZF_EDITOR
	// escapeCarSpot に車を置いた今の町の歩ける床と、建物の隠れの表を、材料から作り直してファイルへ書く（覚えた物は変えない）
	// agentRadius: 床の縁を壁から離す長さ。Release が使う既定の長さ（ZombieDirector::defaultNavMeshRadius）を渡す
	// Debug では、書いたファイルを読み直して、作った物と同じかをログに出す（bake check）
	// 書けなかった・建物が無くて表を作れなかったら false と、その理由を error に入れる
	bool bake( int escapeCarSpot, const TownMap& map, const CollisionWorld& world, const std::vector< Building >& buildings,
		float agentRadius, std::string* error );
#endif

private:
	// 指紋ごとに覚えた床と、それをファイルから読んだか
	struct CachedNavMesh{
		std::unique_ptr< NavMesh > navMesh;
		bool fromFile = false;
	};

	std::map< std::uint64_t, CachedNavMesh > mNavMeshes;						// 指紋ごとの床
	std::map< int, bool > mNavMeshFromFile;										// 車の置き場所ごと：最後に返した床をファイルから読んだか
	std::map< std::uint64_t, std::unique_ptr< TownVisibility > > mVisibilities;	// 指紋ごとの表
};

#endif
