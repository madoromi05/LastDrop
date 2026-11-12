#ifndef INCLUDED_MESH_MATERIAL_H
#define INCLUDED_MESH_MATERIAL_H

#include "GameLib/Graphics/Texture.h"
#include "GameLib/Math/Vector3.h"

// メッシュ表面の見た目（テクスチャ・法線マップ・材質値）をまとめて持つ部品
// SkinnedMesh / StaticMesh がメンバとして持ち（継承ではなく委譲）、描画直前に apply() する
// 描画ステートの設定順（setTexture の後に setNormalMap）の制約はこのクラスの中に閉じ込める
// 既定値はゾンビで調整済みの値。データに材質値が無ければこの見た目になる
class MeshMaterial{
public:
	MeshMaterial();

	void setTexture( GameLib::Graphics::Texture );				// アルベド（省略時は無地）
	void setNormalMap( GameLib::Graphics::Texture );			// 法線マップ（省略時は凹凸なし）
	// 材質の画像（r AO・g 粗さ・b 金属）。G-Buffer で粗さ・金属に掛ける（省略時は数値のまま）
	void setOrmMap( GameLib::Graphics::Texture );
	void setDiffuseColor( const GameLib::Math::Vector3& );		// テクスチャに乗算する色
	void setSpecularColor( const GameLib::Math::Vector3& );		// ツヤの色と強さ（今の描き方＝フォワードだけ。G-Buffer では使わない）
	void setSpecularSharpness( float );							// ツヤの鋭さ（大きいほど小さく鋭いハイライト。同上）
	void setNormalMapStrength( float );							// 凹凸の強さ（0で無効、1で標準）
	// PBR の材質値。G-Buffer へ描くときだけ使う（RenderPass::setSurface）
	void setRoughness( float );									// 粗さ（0 つるつる〜1 ざらざら）
	void setMetallic( float );									// 金属（0 か 1。中間は境目だけ）
	void setEmission( float );									// 発光の強さ（0〜1。色は「アルベド × 強さ」）
	void setPuddle( bool );										// 雨上がりの水たまりを書くか（道路だけ）
	// 裏の面も描くか。既定は描かない（背面カリング）
	// 閉じた形なら裏の面はいつも表の面に隠れるので、描くのは無駄になる。板 1 枚の物・裏返った面がある物だけ true にする
	// （メッシュのファイルに doublesided="1" と書く。変換ツールの設定の material_params / parts にも書ける）
	void setDoubleSided( bool );

	// 非同期ロード中のテクスチャの読み込みを進める。完了するまで毎フレーム呼ぶ
	// 貼る絵が全部展開済みなら true（読めなかった絵も「終わった」に含める。ずっと待ち続けないように）
	bool updateLoading();
	// 描画ステートへ反映する。頂点/インデックスバッファの設定と描画は呼び出し側が行う
	void apply() const;
	// 法線マップが設定されているか（自作シェーダで描くとき、無ければ凹凸の強さを0にするのに使う）
	bool hasNormalMap() const;

private:
	GameLib::Graphics::Texture mTexture;
	GameLib::Graphics::Texture mNormalMap;
	GameLib::Graphics::Texture mOrmMap;
	GameLib::Math::Vector3 mDiffuseColor;
	GameLib::Math::Vector3 mSpecularColor;
	float mSpecularSharpness;
	float mNormalMapStrength;
	float mRoughness;
	float mMetallic;
	float mEmission;
	float mPuddle;   // 0 か 1（G-Buffer のシェーダへそのまま渡す）
	bool mDoubleSided;
};

#endif
