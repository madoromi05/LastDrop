#ifndef INCLUDED_MESH_CACHE_H
#define INCLUDED_MESH_CACHE_H

#include "GameLib/PseudoXml/Document.h"
#include "StaticMeshData.h"
#include "SkinnedMeshData.h"
#include "SkinningPipeline.h"
#include <map>
#include <memory>
#include <string>

// メッシュの「設計図」（StaticMeshData / SkinnedMeshData）をファイル名で管理する倉庫番。
// 同じファイルを何度頼まれても読み込みは1回だけで、同じデータを返す。
// インスタンス（置いた1軒・1体）はここから借りたデータを指すだけにする。
//
// 所有と寿命: データはこのキャッシュが単独で所有する（std::unique_ptr）。返すポインタは借用で、
// キャッシュが生きている間だけ有効。持ち主（今はシーン）は、インスタンスより先にキャッシュを
// 破棄しないこと（メンバの宣言順でキャッシュを先に書けば、後に破棄される）。
// シーンが切り替わればシーンごと解放されるので、前のシーンのモデルが残り続けることはない。
class MeshCache{
public:
	MeshCache();
	~MeshCache();
	MeshCache( const MeshCache& ) = delete;
	MeshCache& operator=( const MeshCache& ) = delete;

	// ファイルの読み込みを始め（2回目以降は何もしない）、データを返す。nullptr にはならない。
	// 読み込みは非同期なので、update() が true を返すまでデータは isReady() == false。
	const StaticMeshData* staticMesh( const char* filename );
	const SkinnedMeshData* skinnedMesh( const char* filename );

	// 読み込み中のファイルを進め、読み終えたものを構築する。テクスチャのロードも進める。毎フレーム呼ぶ。
	// 頼まれた全ファイルの構築が終わっていれば true（構築に失敗したものも「終わった」に含む）。
	bool update();

	// スキンメッシュを描く道具（シェーダ）。エラー表示などで読むだけ。
	const SkinningPipeline& skinning() const;

private:
	// 1ファイルぶん。構築が終わるまで文書を持ち、終われば解放する。
	template< class TData > struct Entry{
		GameLib::PseudoXml::Document mDocument;
		TData mData;
		bool mBuilt = false;
	};
	// スキンメッシュの描き方。データより先に宣言する（＝データより後に破棄される。データが借りて持つため）。
	std::unique_ptr< SkinningPipeline > mSkinning;
	using StaticEntry = Entry< StaticMeshData >;
	using SkinnedEntry = Entry< SkinnedMeshData >;

	std::map< std::string, std::unique_ptr< StaticEntry > > mStaticMeshes;
	std::map< std::string, std::unique_ptr< SkinnedEntry > > mSkinnedMeshes;
};

#endif
