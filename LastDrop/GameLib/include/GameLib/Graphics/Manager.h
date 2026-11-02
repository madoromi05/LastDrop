#ifndef INCLUDED_GAMELIB_GRAPHICS_MANAGER_H
#define INCLUDED_GAMELIB_GRAPHICS_MANAGER_H

#include "GameLib/Graphics/Enum.h"

namespace GameLib{

namespace Math{
	class Matrix44;
	class Matrix34;
	class Vector2;
	class Vector3;
	class Vector4;
}
using namespace Math;

namespace Graphics{

class Texture;
class VertexBuffer;
class IndexBuffer;
class RenderTarget;
class DepthTarget;
class Shader;
class ConstantBuffer;

class Manager{
public:
	///自作シェーダから使う番号の決まり
	enum{
		FIRST_USER_CONSTANT_SLOT = 1, ///setConstantBuffer の最初の番号（b0 は固定パイプラインの定数。自作シェーダからも読める）
		MAX_CONSTANT_SLOT        = 14,///定数バッファの番号の上限（この値は使えない）
		FIRST_USER_TEXTURE_SLOT  = 2, ///setShaderResource の最初の番号（t0 は setTexture、t1 は setNormalMap）
		MAX_TEXTURE_SLOT         = 16,///テクスチャの番号の上限（この値は使えない）
		CLAMP_SAMPLER_SLOT       = 1, ///s1: 端で止める線形補間（s0 は setTextureFilter で決まる繰り返し）
		SHADOW_SAMPLER_SLOT      = 2, ///s2: 影の比較（SampleCmp。深度 <= マップの値なら1）
		MAX_RENDER_TARGET        = 4, ///setRenderTargets で同時に描ける色の数
	};

	Manager();
	static Manager instance();

	void setVertexBuffer( VertexBuffer );
	void setIndexBuffer( IndexBuffer );
	void setTexture( Texture );
	///法線マップをセット（スロットt1）。セットするとノーマルマップ用シェーダで描く。
	void setNormalMap( Texture );
	///色の画像と法線マップ（空なら無し）を一度にセット。今と同じ物は付け直さない。
	///setTexture → setNormalMap は同じ材質でも毎回付け直すので、パーツごとに材質を設定する所はこちらを使う。
	void setTextures( Texture texture, Texture normalMap );

	void enableDepthTest( bool );
	void enableDepthWrite( bool );
	void setBlendMode( BlendMode );
	void setCullMode( CullMode );
	void setTextureFilter( TextureFilter );
	///透視変換ビュー変換行列セット。
	void setProjectionViewMatrix( const Matrix44& );
	///ワールド変換行列セット
	void setWorldMatrix( const Matrix34& );
	///ビューポート設定
	void setViewport( int x, int y, int width, int height );
	///ビューポート取得(0でないものにだけ入れるので欲しいものだけポインタをセット)
	void getViewport( int* xOut, int* yOut, int* widthOut, int* heightOut );

	///設定したテクスチャ、頂点バッファ、インデクスバッファで描画
	void drawIndexed(
		int offset, 
		int primitiveNumber,
		PrimitiveType = PRIMITIVE_TRIANGLE );
	///drawIndexed と同じ物を instanceNumber 回まとめて描く（インスタンシング）。
	///何回目かはシェーダが SV_InstanceID で受け取り、置き場所などは自作の定数（setConstantBuffer）から読む
	void drawIndexedInstanced(
		int offset,
		int primitiveNumber,
		int instanceNumber,
		PrimitiveType = PRIMITIVE_TRIANGLE );
	///設定したテクスチャ、頂点バッファで描画
	void draw(
		int offset,
		int primitiveNumber,
		PrimitiveType = PRIMITIVE_TRIANGLE );
	///draw と同じだが、始まりを頂点の番号で指定する（draw の offset は三角形・線・点の番号）。
	///三角形・線・点が混ざった頂点バッファで、途中から描くときに使う。
	void drawFromVertex(
		int firstVertex,
		int primitiveNumber,
		PrimitiveType = PRIMITIVE_TRIANGLE );
	///全画面にテクスチャを貼り付け
	void blendToScreen( Texture );
	///フルスクリーン化
	void enableFullScreen( bool );
	///描画バッファ幅を返す
	int width() const;
	///描画バッファ高さを返す
	int height() const;
	///バックバッファ（実際の画面）の高さを返す（画素）。height() はゲームの座標の高さで固定、こちらはウィンドウの大きさで変わる。
	///2D の絵（文字など）を実際の画面の細かさで作るときに、height() との比を使う。
	int backBufferHeight() const;
	///フルスクリーン時のウィンドウ幅を返す
	int fullScreenWindowWidth() const;
	///フルスクリーン時のウィンドウ高さを返す
	int fullScreenWindowHeight() const;
	///フレーム番号を返す
	unsigned frameId() const;
	///画面の更新を待つか（VSync）を切り替える。次の Present から効く。
	///CPU と GPU のどちらが重いかを見分けるときに切る（VSync 中はどちらも画面の更新待ちに見えるため）。
	void setVSync( bool );
	bool isVSync() const;
	///スクリーンショットを取る(拡張子でDDS,TGA自動判別)。保存タイミングは描画終了後。
	void captureScreen( const char* filename );

	//ライティング関係
	///ピクセルシェーダを有効化する
	void setLightingMode( LightingMode );
	///ライト位置
	void setLightPosition( int index, const Vector3& position );
	///ライト強度(100なら100mの距離で1の明るさになる。強度Iとして、明るさは「I/距離」で決まる)
	void setLightIntensity( int index, float intensity );
	///ライト色
	void setLightColor( int index, const Vector3& );
	///視点位置
	void setEyePosition( const Vector3& position );
	///ディフューズカラー
	void setDiffuseColor( const Vector3& );
	///スペキュラ色
	void setSpecularColor( const Vector3& );
	///発光色
	void setEmissionColor( const Vector3& );
	///アンビエント色
	void setAmbientColor( const Vector3& );
	///透明度設定
	void setTransparency( float );
	///スペキュラの鋭さ(大きいほど鋭い。数字の意味は別の本を読もう)
	void setSpecularSharpness( float );
	///法線マップの凹凸の強さ(0で無効、1で標準)。setNormalMap していない時は影響しない
	void setNormalMapStrength( float );
	///true の間は、ディフューズ・スペキュラ・発光・アンビエント・ライトの色と頂点色を、画面の色（sRGB）として
	///リニアへ直してから計算する（自作シェーダの b0 にも直した値が入る）。1 を超える色は「1 以下の色×倍率」とみなし、倍率は直さない。
	///リニアの描画先へ 3D を描く間だけ true にし、画面へ直接描く HUD・文字の前に false へ戻す（LinearToneMap_Plan.txt 5 章）。既定は false
	void enableLinearColor( bool );

	//描画先・自作シェーダ
	///colors（count 枚。0枚なら深度だけ）と depth（空なら深度なし）へ描く。描画範囲は描画先の全体になる。
	///大きさはすべて揃っていること。t2 以降のテクスチャはここで外れる（描画先を読みながら書かないように）
	void setRenderTargets( const RenderTarget* colors, int count, DepthTarget depth );
	///上と同じ。readOnlyDepth が true なら depth（空は不可）を奥行きの判定だけに使い、書き込まない（深度の書き込みの設定は無視される）。
	///その間も depth を setShaderResource で読める（半透明の板を物の近くで薄くする・デカール）
	void setRenderTargets( const RenderTarget* colors, int count, DepthTarget depth, bool readOnlyDepth );
	///target の段 mip だけへ描く（深度なし。段が 2 つ以上の描画先。Hi-Z の縮小など）。描画範囲はその段の大きさになる。
	///t2 以降のテクスチャは setRenderTargets と同じく外れるので、読む段（setShaderResource の mip 付き）は後から付ける
	void setRenderTargetMip( RenderTarget target, int mip );
	///画面（バックバッファ）へ描く状態に戻す。描画範囲は setViewport で決めた範囲に戻る
	void resetRenderTargets();
	///描画先を色で塗りつぶす
	void clear( RenderTarget, const Vector4& color );
	///深度を一番奥（1）で埋める
	void clear( DepthTarget );
	///自作シェーダを付ける。空の Shader を渡すと固定パイプライン（setLightingMode 等）に戻る
	void setShader( Shader );
	///定数を b slot に付ける（頂点・ピクセルの両方）。slot は FIRST_USER_CONSTANT_SLOT 以上。空なら外す
	void setConstantBuffer( int slot, ConstantBuffer );
	///ピクセルシェーダの t slot から読めるようにする。slot は FIRST_USER_TEXTURE_SLOT 以上。空なら外す
	void setShaderResource( int slot, RenderTarget );
	void setShaderResource( int slot, DepthTarget );
	void setShaderResource( int slot, Texture );
	///target の段 mip だけを t slot から読めるようにする（シェーダからは段 0 として見える）。段が 2 つ以上の描画先だけ。
	///同じ描画先の別の段へ描きながら読める（縮小のパスが 1 つ上の段を読む）
	void setShaderResource( int slot, RenderTarget target, int mip );
	///頂点バッファを使わず、描画範囲の全体を今の自作シェーダで描く（ポストプロセス用）。
	///VSMain は SV_VertexID（0,1,2）だけから位置を作ること（Resource/Shader/Common.hlsli の fullScreenVertex）
	void drawFullScreen();

	//以下知らなくていい何か
	static void create( 
		void* windowHandle,
		int width,
		int height,
		bool fullScreen,
		bool vSync,
		bool antiAlias );
	static void destroy();
	void beginDraw();
	void endDraw();
	bool canRender() const; //デバイスロストしていればfalseが返る。
	void restore(); //デバイスロストから復帰する
	void getPointerModifier( float* scale, Vector2* offset ) const;
};

} //namespace Graphics
} //namespace GameLib

#endif
