#ifndef INCLUDED_MUZZLE_FLASH_H
#define INCLUDED_MUZZLE_FLASH_H

#include "GameSrc/Render/LitQuadBatch.h"
#include "GameSrc/Render/Flipbook.h"
#include "GameSrc/Render/Lighting/LightList.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Graphics/Texture.h"
#include <random>

// 撃った瞬間に銃口が光る演出（マズルフラッシュ）。自キャラの銃1丁に1つ。
// 炎の形は描かず、銃口に一瞬灯る「光」として見せる。撃ったティックの1ティックだけ光る。
// 撃ち直すと光り始めからやり直す（フルオートでは「光る → 消える」を繰り返して点滅して見える）。
// 見た目は3つ。
//   光の芯     … 銃口の前の、小さくとても明るい丸（画面に平行な板1枚。加算・1 を超える明るさで描く）
//   にじみの板 … 芯のまわりの空気が光って見える、大きく柔らかい板1枚（面は照らさない。面は点光源が照らす）
//   点光源     … 周りの壁・地面と自分の腕・銃を一瞬照らす（主役）。ライトを答えるだけで、一覧（LightList）に足すのはシーン
// 芯の形（4 つの絵から 1 つ。EffectFlipbook_Plan.txt の 4 章）と大きさを1発ごとに乱数で変える（同じ光が続くと機械的に見えるため）。
// 形はとげにはせず、光のふちの出方が少し違うだけ（炎の形は描かない、の決まりのまま）。
// 覗いているときは、照準のすぐ先が白く埋まらないよう、覗き具合に合わせて小さく・薄くする。
// 銃口の位置は持たない（描くたびに今の銃から取り直したものを受け取る。反動・歩きの揺れで銃口から離れないように）。
class MuzzleFlash{
public:
	MuzzleFlash();
	MuzzleFlash( const MuzzleFlash& ) = delete;
	MuzzleFlash& operator=( const MuzzleFlash& ) = delete;

	// 撃った。光り始めからやり直し、芯の大きさを乱数で決め直す。
	// 同じティックの update（このクラスの）の後に呼ぶ（撃ったティックの描画で光るように）。
	void fire();
	// 1ティック進める。消えている間に呼んでも何もしない。
	void update();
	// 光っているか。
	bool isLit() const;

	// 点光源（LightList に足すライト）。消えていれば false（out は変えない）。
	// muzzlePosition / muzzleDirection: 銃口の今の位置（ワールド）と向き（長さ1）。PlayerCharacter::muzzle の値。out: nullptr 不可。
	bool light( const GameLib::Math::Vector3& muzzlePosition, const GameLib::Math::Vector3& muzzleDirection, LightSource* out ) const;
	// 光の芯とにじみの板を描く。消えていれば何もしない。1フレームに1回まで。呼ぶ場所の決まりは LitQuadBatch::draw と同じ。
	// 絵の読み込みが終わるまでは描かない。
	// projectionView: カメラの射影×ビュー行列。viewDirection: 視線の向き（長さ1）。板を画面に平行にするため。
	// muzzlePosition / muzzleDirection: light と同じ。aimWeight: 覗き具合（0〜1。PlayerCharacter::aimWeight）。
	void draw( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& viewDirection,
		const GameLib::Math::Vector3& muzzlePosition, const GameLib::Math::Vector3& muzzleDirection, float aimWeight );

private:
	int mAge;      // 撃ってからのティック数（撃ったティックが 0）。消えている間は光る長さ以上
	float mScale;  // 芯の大きさのばらつき（倍率）
	int mShape;    // 芯の形（mCoreFrames のコマの番号）
	std::mt19937 mRandom;                    // 大きさと形のばらつき
	GameLib::Graphics::Texture mCoreTexture; // 光の芯の絵（Resource/Texture/Effect/muzzle_flash_sheet.tga。2×2 に 4 つの形）
	Flipbook mCoreFrames;                    // 上の絵のコマの並び
	GameLib::Graphics::Texture mGlowTexture; // にじみの板の絵（Resource/Texture/Effect/muzzle_glow.tga）
	LitQuadBatch mCoreQuads; // 絵と明るさの倍率が違うので、芯とにじみの板は別にためて描く
	LitQuadBatch mGlowQuads;
};

#endif
