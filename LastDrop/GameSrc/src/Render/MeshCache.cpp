#include "GameSrc/Render/MeshCache.h"
#include "GameSrc/Render/MeshLoaderUtil.h"
#include "GameSrc/Render/StaticMesh/StaticMeshLoader.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshLoader.h"

#include "GameLib/Base/DebugStream.h"
#include "GameLib/Profiling/Profiler.h"
#include <chrono>

using namespace GameLib;

namespace{
	// 1 回の update で構築してよい長さ（ミリ秒）。読み終えたファイルを 1 回で全部構築すると、Debug では 1 フレームが数秒になり、
	// ロード画面が止まって見えた。超えたら残りは次の update へ回す（1 つは必ず構築する）。
	const long long kBuildBudgetMs = 50;
	using BuildClock = std::chrono::steady_clock;

	// 無ければ読み込みを始めて登録し、あればそのまま返す（Static / Skinned 共通）。
	template< class TEntry >
	TEntry* findOrLoad( std::map< std::string, std::unique_ptr< TEntry > >* entries, const char* filename ) {
		auto found = entries->find( filename );
		if ( found != entries->end() ) {
			return found->second.get();
		}
		// ここに来るのはファイルごとに1回だけ。使い回せているかはこの出力の回数で確かめられる。
		GameLib::cout << "MeshCache: load " << filename << GameLib::endl;
		auto entry = std::make_unique< TEntry >();
		entry->mRequestedAt = Profiling::now();
		entry->mDocument = PseudoXml::Document::create( filename );
		TEntry* raw = entry.get();
		( *entries )[ filename ] = std::move( entry );
		return raw;
	}

	// 読み終えた文書からデータを構築し、貼るテクスチャの展開を進める。構築済み・読み込み中なら構築はしない。
	// 構築もテクスチャの展開も全部終わっていれば true（失敗も終わりに含める。ずっと待ち続けないように）。
	// テクスチャまで待つのは、展開（大きい絵で 1 枚 約 40ms）をロードの中で済ませ、
	// ゲームが始まった最初の描画にまとめて来させないため（計測で 243ms のひっかかりが出ていた）。
	template< class TEntry, class TLoad >
	bool buildLoaded( std::map< std::string, std::unique_ptr< TEntry > >* entries, TLoad load, TextureCache* textures,
	const BuildClock::time_point& deadline, bool* builtOne ) {
		bool allReady = true;
		for ( auto& pair : *entries ) {
			TEntry& entry = *pair.second;
			if ( !entry.mBuilt && *builtOne && BuildClock::now() >= deadline ) {
				allReady = false; // 今回の長さを使い切った。次の update で構築する
			} else if ( !entry.mBuilt ) {
				// 読み終えた文書は、最初に isReady が true を返すときにメインスレッドで解析される。
				// 終わってから解析だったと分かるので、呼ぶ前の時刻を取っておく。
				long long polledAt = Profiling::now();
				if ( entry.mDocument.isReady() ) {
					const char* filename = pair.first.c_str();
					Profiling::addLoadWait( filename, entry.mRequestedAt );
					Profiling::addLoadWork( "parse", filename, polledAt );
					LOAD_SCOPE( "build", filename );
					if ( !load( &entry.mData, MeshLoaderUtil::containerRoot( entry.mDocument.root() ), textures ) ) {
						GameLib::cout << "MeshCache: failed to build " << pair.first.c_str() << GameLib::endl;
					}
					entry.mDocument.release(); // 構築済みなので文書は解放
					entry.mBuilt = true;
					*builtOne = true;
				} else {
					allReady = false;
				}
			}
			// テクスチャの非同期ロードを進める（構築前のデータはパーツが無いので true が返る）。
			if ( !entry.mData.updateLoading() ) {
				allReady = false;
			}
		}
		return allReady;
	}
}

MeshCache::MeshCache() :
	mSkinning( std::make_unique< SkinningPipeline >() ) {
}

MeshCache::~MeshCache() {
}

const StaticMeshData* MeshCache::staticMesh( const char* filename ) {
	return &findOrLoad( &mStaticMeshes, filename )->mData;
}

const StaticMeshData* MeshCache::staticMeshForMerging( const char* filename ) {
	StaticEntry* entry = findOrLoad( &mStaticMeshes, filename );
	if ( !entry->mBuilt ) {
		entry->mData.keepSourceVertices();
	}
	ASSERT( entry->mData.keepsSourceVertices() && "MeshCache : already built without source vertices. call staticMeshForMerging first." );
	return &entry->mData;
}

const SkinnedMeshData* MeshCache::skinnedMesh( const char* filename ) {
	SkinnedEntry* entry = findOrLoad( &mSkinnedMeshes, filename );
	entry->mData.setSkinning( mSkinning.get() ); // 全スキンメッシュで同じ描き方を共有する
	return &entry->mData;
}

bool MeshCache::update() {
	// 両方を必ず呼ぶ（&& の短絡で片方のロードが止まらないように）。
	const BuildClock::time_point deadline = BuildClock::now() + std::chrono::milliseconds( kBuildBudgetMs );
	bool builtOne = false;
	bool staticReady = buildLoaded( &mStaticMeshes, StaticMeshLoader::load, &mTextures, deadline, &builtOne );
	bool skinnedReady = buildLoaded( &mSkinnedMeshes, SkinnedMeshLoader::load, &mTextures, deadline, &builtOne );
	return staticReady && skinnedReady;
}

float MeshCache::progress() const {
	int total = 0;
	int built = 0;
	for ( const auto& pair : mStaticMeshes ) {
		++total;
		built += pair.second->mBuilt ? 1 : 0;
	}
	for ( const auto& pair : mSkinnedMeshes ) {
		++total;
		built += pair.second->mBuilt ? 1 : 0;
	}
	return ( total > 0 ) ? static_cast< float >( built ) / static_cast< float >( total ) : 1.f;
}

const SkinningPipeline& MeshCache::skinning() const {
	return *mSkinning;
}
