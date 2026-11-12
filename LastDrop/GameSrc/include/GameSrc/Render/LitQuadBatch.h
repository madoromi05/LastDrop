#ifndef INCLUDED_LIT_QUAD_BATCH_H
#define INCLUDED_LIT_QUAD_BATCH_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Graphics/Texture.h"
#include "GameLib/Graphics/Vertex.h"
#include "GameLib/Graphics/VertexBuffer.h"
#include "GameLib/Graphics/Shader.h"
#include "GameLib/Graphics/ConstantBuffer.h"
#include <vector>

// 板に貼る絵の範囲（UV。絵の左上が 0,0、右下が 1,1）。既定は絵 1 枚まるごと
// 1 枚の絵に並べたコマの 1 つだけを貼るとき（フリップブック。Render/Flipbook）に使う
struct QuadUv{
	float left = 0.f;
	float top = 0.f;
	float right = 1.f;
	float bottom = 1.f;
};

// 絵を貼った四角い板を決まった数までためて、1回で描く。照らし方は作るときに決める（Mode）
// MODE_LIT: 壁や地面と同じライトの一覧（LightList。b2）で照らし、半透明（絵のアルファ×頂点の色のアルファ）で描く
//           シェーダは Resource/Shader/LitQuad.hlsl（粗さ 1・金属 0 の面。粉や穴は光らないため）
//           弾痕（面に貼る板）と着弾の砂ぼこり・血しぶき（画面に平行な板）が使う
// MODE_GLOW: ライト無しで、後ろに足して（加算）描く。自分で光る物（マズルフラッシュ）が使う
//            明るさは 絵の色×頂点の色×アルファ×明るさの倍率（setBrightness）。足すだけなので、黒は透明と同じになり、
//            板どうしの描く順番も気にしなくてよい。シェーダは Resource/Shader/GlowQuad.hlsl（固定パイプラインのライト無しと同じ色）
// どちらも奥行きは読むが書かない。PrimitiveRenderer はライトを必ず切って描くので、照らす板には使えない（光る板もここで揃える）
// 作るときに「薄くし始める奥行きの差」（softDistance）を決めると、壁・地面に刺さった所で板を薄くする（ソフトパーティクル
// SsaoTranslucent_Plan.txt 7 章）。そのときは PostProcess::resumeSceneReadOnlyDepth の間に描くこと（深度を読むため）
// 面に貼る板（弾痕）は 0（薄くしない。薄くすると貼った面で消える）
// ライトの値は持たない。シーンが先に付けたライトの一覧（GameScene::setupLights → LightList::bind）でそのまま照らされる
// 半透明の板は、目の位置を渡して描くと奥から順に並べ替えて描く（Z ソート）
// 奥行きを書かないので、手前の板を先に描くと、後から描いた奥の板が手前の板の上に重なって見えるため
// 使い方: add を何回か → draw を、1フレームに1回
class LitQuadBatch{
public:
	enum Mode{
		MODE_LIT,  // 壁や地面と同じライトで照らし、半透明で混ぜる
		MODE_GLOW, // ライト無しで、加算で足す（自分で光る物）
	};

	// quadCapacity: 1回に描ける板の数。1以上。mode: 照らし方と混ぜ方（作った後は変えない）
	// softDistance: 後ろの物との奥行きの差がこれ（m）より小さい所を薄くする。0 なら薄くしない（作った後は変えない）
	explicit LitQuadBatch( int quadCapacity, Mode mode = MODE_LIT, float softDistance = 0.f );
	LitQuadBatch( const LitQuadBatch& ) = delete;
	LitQuadBatch& operator=( const LitQuadBatch& ) = delete;

	// 板を1枚ためる。容量を超えた板はためない（持ち主が数の上限を決めているので、ふつうは起きない）
	// center: 板の中心。right / up: 中心から右の辺・上の辺までの向きと長さ（ワールド）
	// normal: 照らす向き（長さ1。この向きの面として光が当たる。MODE_GLOW では使わない）
	// color: 頂点の色（VertexColor::pack の並び。絵の色と不透明度に掛かる）
	// uv: 貼る絵の範囲（省けば絵 1 枚まるごと）
	void add( const GameLib::Math::Vector3& center, const GameLib::Math::Vector3& right, const GameLib::Math::Vector3& up,
		const GameLib::Math::Vector3& normal, unsigned color, const QuadUv& uv = QuadUv() );
	// 色に掛ける明るさの倍率（初期値 1）。頂点の色は 1 までしか持てないので、1 を超える明るさで描きたいときに使う
	// 描画先は 1 を超える明るさを持てる（PostProcess の RGBA16F）。今は画面に出すときに 1 で切られるが、
	// ブルームが入ると、1 を超えた所が光としてにじむ。0 以上
	void setBrightness( float brightness );
	// ためた板を texture で描き、ためた板を捨てる。板が無ければ何もしない
	// 物を描いた後、ライトの一覧を付けた後（LightList::bind〜unbind の間）に、同じ描画先（PostProcess の beginScene〜endScene の間）で呼ぶ
	// texture: 読み込み済み（isReady）であること。projectionView: カメラの射影×ビュー行列
	// eye: 目の位置（ワールド）。渡すと板を目から遠い順に並べ替えて描く（半透明の板どうしの重なりを正しくする）
	//      nullptr ならためた順のまま（加算の板・壁に貼った弾痕のように、順番が見た目に効かない物）
	// 描き終えたら、変えた設定（混ぜ方・奥行きの書き込み）を、物を描くときの設定に戻す（後に描く物に半透明・加算が移らないように）
	// 照らし方は戻さない（物を描く側が毎回自分で決めている。MeshMaterial::apply）
	void draw( GameLib::Graphics::Texture texture, const GameLib::Math::Matrix44& projectionView,
		const GameLib::Math::Vector3* eye = nullptr );

private:
	// mVertices を、目から遠い板が先になる順に locked へ写す
	void copySortedFromBack( const GameLib::Math::Vector3& eye, GameLib::Graphics::Vertex* locked );
	// 薄くする距離と、深度から位置を戻す行列を定数に入れて b1 に付ける（SoftQuad.hlsli）
	void setSoftConstants( const GameLib::Math::Matrix44& projectionView );

	GameLib::Graphics::VertexBuffer mVertexBuffer;       // 描くときに mVertices を写す先（毎フレーム中身を入れ替える）
	GameLib::Graphics::Shader mShader;                   // MODE_LIT は LitQuad.hlsl、MODE_GLOW は GlowQuad.hlsl
	GameLib::Graphics::ConstantBuffer mSoftConstants;    // 薄くする距離と行列（SoftQuad.hlsli の b1）
	std::vector< GameLib::Graphics::Vertex > mVertices;  // ためた板の頂点（ワールド座標）。draw で空になる
	std::vector< GameLib::Math::Vector3 > mCenters;      // ためた板の中心（並べ替えの奥行きに使う）。mVertices と同じく draw で空になる
	std::vector< int > mOrder;                           // 並べ替えた板の番号（毎回作り直す。確保し直さないよう持っておく）
	int mVertexCapacity;
	Mode mMode;
	float mBrightness;   // 色に掛ける明るさの倍率
	float mSoftDistance; // 薄くし始める奥行きの差（m。0 で薄くしない）
};

// 板を物の近くで薄くするか（F6 → Render の Soft particles。見比べ用）。false の間はどの板も薄くしない
// 起動中ずっと 1 つ（LitQuadBatch.cpp が持つ。初期値 true。Release では true のまま）
bool& softParticlesEnabled();

#endif
