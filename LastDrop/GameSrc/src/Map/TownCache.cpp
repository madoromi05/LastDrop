#include "GameSrc/Map/TownCache.h"
#ifdef _DEBUG
#include "GameSrc/DebugMode/TownVisibilityCheck.h"
#endif
#include "GameSrc/Map/NavMesh.h"
#include "GameSrc/Map/TownVisibility.h"
#include "GameSrc/Map/TownMap.h"
#include "GameSrc/Map/TownFingerprint.h"
#include "GameSrc/Map/TownBakeFile.h"
#include "GameSrc/Map/Building.h"
#include "GameSrc/Collision/CollisionWorld.h"
#include "GameSrc/Hit/HitWorld.h"

#include "GameLib/GameLib.h"
#include "GameLib/Profiling/Profiler.h"
#ifdef ZF_EDITOR
#include "GameLib/FileIO/Manager.h"
#endif

namespace{
#ifdef ZF_EDITOR
	// 書き出したファイルを使えずに作り直すことを、1 行ログへ出す（Debug・Profile だけ。Release は黙って作る）
	// 無いのは、まだ書き出していないとき。あるのに使えないのは、手直しの後に書き出していない（指紋が違う）か、ファイルが壊れているとき
	void logFallback( const char* path ) {
		const char* reason = GameLib::FileIO::Manager().exists( path ) ? " is stale or broken, building" : " not found, building";
		GameLib::cout << "TownCache: " << path << reason << GameLib::endl;
	}
#endif

	bool loadNavMesh( int escapeCarSpot, std::uint64_t fingerprint, float agentRadius, NavMesh* navMesh ) {	// 事前に書き出した床を navMesh へ読む。ファイルが無い・指紋が合わない・読めないなら false（navMesh は床の無いまま）
		LOAD_SCOPE( "town", "NavMesh::load" );
		const std::string path = TownBakeFile::navMeshPath( escapeCarSpot );
		std::vector< unsigned char > data;
		const bool loaded = TownBakeFile::read( path.c_str(), fingerprint, &data ) && navMesh->load( data, agentRadius );
#ifdef ZF_EDITOR
		if ( !loaded ) {
			logFallback( path.c_str() );
		}
#endif
		return loaded;
	}

	bool loadVisibility( std::uint64_t fingerprint, const std::vector< Building >& buildings, TownVisibility* visibility ) {	// 事前に書き出した表を visibility へ読む（床と同じ）
		LOAD_SCOPE( "town", "TownVisibility::load" );
		std::vector< unsigned char > data;
		const bool loaded = TownBakeFile::read( TownBakeFile::kVisibilityPath, fingerprint, &data ) && visibility->loadTables( buildings, data );
#ifdef ZF_EDITOR
		if ( !loaded ) {
			logFallback( TownBakeFile::kVisibilityPath );
		}
#endif
		return loaded;
	}

#ifdef _DEBUG
	// ── 書いたファイルを読み直す確かめ（遊ばずに、書き出しが次の出撃で読めて、作った物と同じになることをログで見る）──
	void checkNavMeshFile( const std::string& path, std::uint64_t fingerprint, float agentRadius, const NavMesh& built ) {	// 床：読み直した床の多角形の数が、作った床と同じか
		NavMesh loaded;
		std::vector< unsigned char > data;
		const bool same = TownBakeFile::read( path.c_str(), fingerprint, &data ) && loaded.load( data, agentRadius ) &&
			loaded.polygonCount() == built.polygonCount();
		GameLib::cout << "NavMesh: bake check " << path.c_str() << " " << loaded.polygonCount() << " / " << built.polygonCount() <<
			" polygons " << ( same ? "OK" : "MISMATCH" ) << GameLib::endl;
	}

	void checkVisibilityFile( std::uint64_t fingerprint, const std::vector< Building >& buildings, const std::vector< unsigned char >& written ) {	// 表：読み直した表を書き出し直したバイト列が、書いた物と 1 バイトも違わないか
		TownVisibility loaded;
		std::vector< unsigned char > data;
		std::vector< unsigned char > again;
		if ( TownBakeFile::read( TownBakeFile::kVisibilityPath, fingerprint, &data ) && loaded.loadTables( buildings, data ) ) {
			loaded.saveTables( &again );
		}
		const bool same = !written.empty() && again == written;
		GameLib::cout << "TownVisibility: bake check " << TownBakeFile::kVisibilityPath << " " << static_cast< int >( again.size() ) << " / " <<
			static_cast< int >( written.size() ) << " bytes " << ( same ? "OK" : "MISMATCH" ) << GameLib::endl;
	}
#endif
}

TownCache::TownCache() {
}

// NavMesh・TownVisibility の定義が見える所で破棄する
TownCache::~TownCache() {
}

NavMesh* TownCache::navMesh( int escapeCarSpot, const TownMap& map, const CollisionWorld& world, float agentRadius ) {
	const std::uint64_t fingerprint = TownFingerprint::navMesh( map, escapeCarSpot, agentRadius );
	CachedNavMesh& cached = mNavMeshes[ fingerprint ];
	if ( !cached.navMesh ) {
		cached.navMesh = std::make_unique< NavMesh >();
		cached.fromFile = loadNavMesh( escapeCarSpot, fingerprint, agentRadius, cached.navMesh.get() );
		if ( !cached.fromFile ) {
			LOAD_SCOPE( "town", "NavMesh::build" );
			cached.navMesh->build( world, agentRadius ); // 失敗しても持つ（同じ指紋で毎回作り直して、毎回失敗しないように）
		}
	}
	mNavMeshFromFile[ escapeCarSpot ] = cached.fromFile;
	return cached.navMesh.get();
}

bool TownCache::isNavMeshFromFile( int escapeCarSpot ) const {
	auto found = mNavMeshFromFile.find( escapeCarSpot );
	return found != mNavMeshFromFile.end() && found->second;
}

const TownVisibility* TownCache::visibility( const TownMap& map, const std::vector< Building >& buildings, const HitWorld& hitWorld ) {
	const std::uint64_t fingerprint = TownFingerprint::visibility( map );
	std::unique_ptr< TownVisibility >& visibility = mVisibilities[ fingerprint ];
	if ( !visibility ) {
		visibility = std::make_unique< TownVisibility >();
		if ( !loadVisibility( fingerprint, buildings, visibility.get() ) ) {
			visibility->build( buildings );
		}
#ifdef _DEBUG
		LOAD_SCOPE( "town", "TownVisibility::verify" );
		TownVisibilityCheck::run( *visibility, buildings, hitWorld ); // 3D の線で表を確かめる（ファイルから読んだ表も。DebugMode）
#else
		( void )hitWorld; // 確かめるのは Debug だけ
#endif
	}
	return visibility.get();
}

#ifdef ZF_EDITOR
bool TownCache::bake( int escapeCarSpot, const TownMap& map, const CollisionWorld& world, const std::vector< Building >& buildings,
float agentRadius, std::string* error ) {
	ASSERT( error && "TownCache::bake : error is null." );
	// ── 歩ける床 ──
	NavMesh navMesh;
	std::vector< unsigned char > navData;
	if ( !navMesh.build( world, agentRadius ) || !navMesh.saveData( &navData ) ) {
		*error = "Cannot build the NavMesh (see the log: NavMesh: failed to ...)";
		return false;
	}
	const std::uint64_t navFingerprint = TownFingerprint::navMesh( map, escapeCarSpot, agentRadius );
	const std::string navPath = TownBakeFile::navMeshPath( escapeCarSpot );
	if ( !TownBakeFile::write( navPath.c_str(), navFingerprint, navData, error ) ) {
		return false;
	}
	GameLib::cout << "TownCache: wrote " << navPath.c_str() << GameLib::endl;
#ifdef _DEBUG
	checkNavMeshFile( navPath, navFingerprint, agentRadius, navMesh );
#endif

	// ── 隠れの表。建物が無ければ表を作らない（build と同じ）ので書かない ──
	TownVisibility visibility;
	visibility.build( buildings );
	std::vector< unsigned char > tables;
	visibility.saveTables( &tables );
	if ( tables.empty() ) {
		*error = std::string( "No buildings: " ) + TownBakeFile::kVisibilityPath + " is not written";
		return false;
	}
	const std::uint64_t visibilityFingerprint = TownFingerprint::visibility( map );
	if ( !TownBakeFile::write( TownBakeFile::kVisibilityPath, visibilityFingerprint, tables, error ) ) {
		return false;
	}
	GameLib::cout << "TownCache: wrote " << TownBakeFile::kVisibilityPath << GameLib::endl;
#ifdef _DEBUG
	checkVisibilityFile( visibilityFingerprint, buildings, tables );
#endif
	return true;
}
#endif
