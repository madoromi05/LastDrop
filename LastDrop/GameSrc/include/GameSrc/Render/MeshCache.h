#ifndef INCLUDED_MESH_CACHE_H
#define INCLUDED_MESH_CACHE_H

#include "GameLib/PseudoXml/Document.h"
#include "GameSrc/Render/StaticMesh/StaticMeshData.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshData.h"
#include "GameSrc/Render/SkinnedMesh/SkinningPipeline.h"
#include "GameSrc/Render/TextureCache.h"
#include <map>
#include <memory>
#include <string>

// メッシュの「設計図」（StaticMeshData / SkinnedMeshData）をファイル名で管理する倉庫番
// 同じファイルを何度頼まれても読み込みは1回だけで、同じデータを返す
// メッシュが貼るテクスチャも、同じ画像ファイルなら1枚にまとめる（TextureCache）。別々のメッシュが同じ絵を使っていてもよい
// インスタンス（置いた1軒・1体）はここから借りたデータを指すだけにする
//
// 所有と寿命: データはこのキャッシュが単独で所有する（std::unique_ptr）。返すポインタは借用で、
// キャッシュが生きている間だけ有効。持ち主は GameResources（起動から終了まで1つ。SceneManager が持つ）で、
// どのシーンのインスタンスよりも後に破棄される
// 一度読み込んだデータは終了まで持ち続ける（シーンを作り直しても読み直さない）。その代わり、
// 使わなくなったモデルも残る。マップが増えて持ちきれなくなったら、手放す入口をここに足す
class MeshCache{
public:
	MeshCache();
	~MeshCache();
	MeshCache( const MeshCache& ) = delete;
	MeshCache& operator=( const MeshCache& ) = delete;

	// ファイルの読み込みを始め（2回目以降は何もしない）、データを返す。nullptr にはならない
	// 読み込みは非同期なので、update() が true を返すまでデータは isReady() == false
	const StaticMeshData* staticMesh( const char* filename );
	// staticMesh と同じだが、返すデータは GPU へ送った後も CPU 側の頂点を残す（StaticMeshMerger でまとめる元にできる）
	// 残すかどうかは構築のときに決まるので、同じファイルを staticMesh でも頼むなら、こちらを構築より前に呼んでおくこと
	// （構築済みで頂点を残していないデータに対して呼ぶと ASSERT で止まる）。小さい部品だけに使う
	const StaticMeshData* staticMeshForMerging( const char* filename );
	const SkinnedMeshData* skinnedMesh( const char* filename );

	// 読み込み中のファイルを進め、読み終えたものを構築する。テクスチャのロードも進める。毎フレーム呼ぶ
	// 頼まれた全ファイルの構築と、それが貼るテクスチャの展開が終わっていれば true
	// （構築・読み込みに失敗したものも「終わった」に含む）
	bool update();
	float progress() const;	// 頼まれたファイルのうち、構築を終えた割合（0〜1。何も頼んでいなければ 1）。ロード画面の棒に使う（テクスチャの展開は数えない）

	const SkinningPipeline& skinning() const;	// スキンメッシュを描く道具（シェーダ）。エラー表示などで読むだけ

private:
	// 1ファイルぶん。構築が終わるまで文書を持ち、終われば解放する
	template< class TData > struct Entry{
		GameLib::PseudoXml::Document mDocument;
		TData mData;
		bool mBuilt = false;
		long long mRequestedAt = 0; // 読み込みを頼んだ時刻（ロードの計測で、届くまで待った時間を出すため）
	};
	std::unique_ptr< SkinningPipeline > mSkinning;	// スキンメッシュの描き方。データより先に宣言する（＝データより後に破棄される。データが借りて持つため）
	TextureCache mTextures;							// メッシュが貼る絵。同じファイルは1枚（ローダへ貸す）
	using StaticEntry = Entry< StaticMeshData >;
	using SkinnedEntry = Entry< SkinnedMeshData >;

	std::map< std::string, std::unique_ptr< StaticEntry > > mStaticMeshes;
	std::map< std::string, std::unique_ptr< SkinnedEntry > > mSkinnedMeshes;
};

#endif
