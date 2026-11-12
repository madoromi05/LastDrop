#ifndef INCLUDED_DECAL_BATCH_H
#define INCLUDED_DECAL_BATCH_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Graphics/Texture.h"
#include "GameLib/Graphics/VertexBuffer.h"
#include "GameLib/Graphics/Shader.h"
#include "GameLib/Graphics/ConstantBuffer.h"
#include <vector>

// 面に貼る絵（デカール）を、決まった数までためて描く。シェーダは Resource/Shader/Decal.hlsl。仕様は SsrDecalMaterial_Plan.txt 8 章
// デカール 1 つは、面の上の点を中心にした薄い箱。箱の中にある G-Buffer の面にだけ、絵の色と凹みの法線をアルファで混ぜて書く
// （面そのものに書くので、角や窓枠からはみ出さず、面から浮かせる必要も無い）。粗さ・金属・AO（RT2）は面のまま
// 何を貼るか・いつ消すかは持ち主（BulletHoleList など）が決める。ここは「ためる・描く」だけ（血の跡など別の絵にも使える）
// 使い方: add を何回か → draw を、1 フレームに 1 回。GBuffer::beginDecals〜endDecals の間に呼ぶ
class DecalBatch{
public:
	// 絵のうち、デカール 1 つに使う所（UV。左上と大きさ）。初期値は絵の全部
	// 何種類かの絵を 1 枚に並べ、デカールごとに選ぶため（血の跡の 2×2。BloodStain_Plan.txt Q1）
	struct UvRect{
		float left = 0.f;
		float top = 0.f;
		float width = 1.f;
		float height = 1.f;
	};

	// capacity: 1 回に描けるデカールの数。1 以上
	explicit DecalBatch( int capacity );
	DecalBatch( const DecalBatch& ) = delete;
	DecalBatch& operator=( const DecalBatch& ) = delete;

	// デカールを 1 つためる。容量を超えた物はためない（持ち主が数の上限を決めているので、ふつうは起きない）
	// center: 面の上の点。right / up: 中心から絵の右の辺・上の辺までの向きと長さ（面に沿う。互いに直交）
	// normal: 貼る面の外向き（長さ 1）。halfDepth: 箱の厚さの半分（m。面の前後にこれだけ届く。薄いほど角の奥の別の面まで届かない）
	// opacity: 不透明度（0〜1。古くなったら下げる）。uvRect: 絵のうち使う所（省くと全部）
	void add( const GameLib::Math::Vector3& center, const GameLib::Math::Vector3& right, const GameLib::Math::Vector3& up,
		const GameLib::Math::Vector3& normal, float halfDepth, float opacity, const UvRect& uvRect = UvRect() );
	// ためたデカールを描き、ためた物を捨てる。無ければ何もしない。albedo: 絵（色とアルファ）。normalMap: 凹みの法線（BC5）
	// どちらも読み込み済み（isReady）であること。projectionView: カメラの射影×ビュー行列
	// 描き終えたら、変えた設定（混ぜ方・奥行きの判定と書き込み・裏表）を、物を描くときの設定へ戻す
	void draw( GameLib::Graphics::Texture albedo, GameLib::Graphics::Texture normalMap, const GameLib::Math::Matrix44& projectionView );
	const char* errorMessage() const;	// シェーダのコンパイルエラー。問題が無ければ空文字列

private:
	struct Decal{
		GameLib::Math::Vector3 center;
		GameLib::Math::Vector3 right;
		GameLib::Math::Vector3 up;
		GameLib::Math::Vector3 normal;
		float halfDepth;
		float opacity;
		UvRect uvRect;
	};
	void setDecalConstants( const Decal& decal, const GameLib::Math::Matrix44& inverseProjectionView );	// decal の箱の行列と、箱の中の座標へ戻す定数を書く（1 つ描くたび）

	GameLib::Graphics::VertexBuffer mBox;      // 1 辺 1 の立方体（三角形 12 枚。中心が原点）
	GameLib::Graphics::Shader mShader;         // Resource/Shader/Decal.hlsl
	GameLib::Graphics::ConstantBuffer mConstants;
	std::vector< Decal > mDecals;              // ためたデカール。draw で空になる
	int mCapacity;
};

// デカールを描くか（F6 → Render の Decals。見比べと計測用）。false の間は弾痕が描かれない
// （板の弾痕に戻すのではない）。起動中ずっと 1 つ（DecalBatch.cpp が持つ。初期値 true。Release では true のまま）
bool& decalsEnabled();

#endif
