#include "GameSrc/Render/MeshCache.h"
#include "GameSrc/Render/MeshLoaderUtil.h"
#include "GameSrc/Render/StaticMesh/StaticMeshLoader.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshLoader.h"

#include "GameLib/Base/DebugStream.h"

using namespace GameLib;

namespace{
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
		entry->mDocument = PseudoXml::Document::create( filename );
		TEntry* raw = entry.get();
		( *entries )[ filename ] = std::move( entry );
		return raw;
	}

	// 読み終えた文書からデータを構築する。構築済み・読み込み中なら何もしない。
	// 全部終わっていれば true（失敗も終わりに含める。ずっと待ち続けないように）。
	template< class TEntry, class TLoad >
	bool buildLoaded( std::map< std::string, std::unique_ptr< TEntry > >* entries, TLoad load, TextureCache* textures ) {
		bool allBuilt = true;
		for ( auto& pair : *entries ) {
			TEntry& entry = *pair.second;
			if ( !entry.mBuilt ) {
				if ( entry.mDocument.isReady() ) {
					if ( !load( &entry.mData, MeshLoaderUtil::containerRoot( entry.mDocument.root() ), textures ) ) {
						GameLib::cout << "MeshCache: failed to build " << pair.first.c_str() << GameLib::endl;
					}
					entry.mDocument.release(); // 構築済みなので文書は解放
					entry.mBuilt = true;
				} else {
					allBuilt = false;
				}
			}
			entry.mData.updateLoading(); // テクスチャの非同期ロードを進める
		}
		return allBuilt;
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
	bool staticBuilt = buildLoaded( &mStaticMeshes, StaticMeshLoader::load, &mTextures );
	bool skinnedBuilt = buildLoaded( &mSkinnedMeshes, SkinnedMeshLoader::load, &mTextures );
	return staticBuilt && skinnedBuilt;
}

const SkinningPipeline& MeshCache::skinning() const {
	return *mSkinning;
}
