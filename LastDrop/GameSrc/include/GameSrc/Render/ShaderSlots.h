#ifndef INCLUDED_SHADER_SLOTS_H
#define INCLUDED_SHADER_SLOTS_H

#include "GameLib/Graphics/Manager.h"

// 複数のパスが同じ番号で読むテクスチャ（t）と定数バッファ（b）の割り当ての、ただ1か所
// Resource/Shader の register の番号と揃える。番号を足す・ずらすときは、ここと HLSL の両方を直す
// 1 つのパスの中だけで使う番号は、そのパスの .cpp が持つ（全画面のパスの入力の t2、パスごとの定数の b1）
namespace ShaderSlots{
	// ── テクスチャ（t）──
	// G-Buffer を読む側（GBuffer.hlsli）。ライティング・SSAO・SSR・TAA・デカール・半透明の板が同じ番号で読む
	const int kGBufferAlbedo = GameLib::Graphics::Manager::FIRST_USER_TEXTURE_SLOT;	// t2 色
	const int kGBufferNormal = kGBufferAlbedo + 1;									// t3 法線。a は動きの種類
	const int kGBufferMaterial = kGBufferNormal + 1;								// t4 AO・粗さ・金属・発光
	const int kSceneDepth = kGBufferMaterial + 1;									// t5 シーンの深度
	// G-Buffer を読む番号のすぐ後ろ（t6）。ライティングのパスでは影のマップ、SSR・TAA では自分の入力に使う
	// 同じパスで両方を付けることは無いので、番号が重なってよい
	const int kAfterGBuffer = kSceneDepth + 1;

	// ライティングのパス（LightList.hlsli・DeferredLighting.hlsl・SsrComposite.hlsl）
	const int kShadowMapNumber = 4;								// 影のマップの数。LightList::kMaxShadowMaps と同じ
	const int kFirstShadowMap = kAfterGBuffer;					// t6〜t9 影のマップ
	const int kOcclusion = kFirstShadowMap + kShadowMapNumber;	// t10 SSAO の結果
	// G-Buffer へ書く側（GBuffer.hlsli）。読む側と同じシェーダに入るので、重ならない番号にする
	const int kOrmMap = kOcclusion + 1;							// t11 材質の画像
	const int kPuddleMask = kOrmMap + 1;						// t12 水たまりの模様
	const int kReflection = kPuddleMask + 1;					// t13 SSR の結果
	const int kMoonTexture = kReflection + 1;					// t14 月の模様（SkyDetail.hlsli）
	const int kHiZView = kMoonTexture + 1;						// t15 表示の Hi-Z
	static_assert( kHiZView < GameLib::Graphics::Manager::MAX_TEXTURE_SLOT, "ShaderSlots : texture slot is out of range." );

	// ── 定数バッファ（b）──
	// b1 は各パスが自分の定数に使う（全画面のパス・キャラの骨・インスタンスの置き場所）。その後ろを、パスをまたぐ定数に割り当てる
	const int kLightConstants = GameLib::Graphics::Manager::FIRST_USER_CONSTANT_SLOT + 1;	// b2 ライトの一覧（LightList.hlsli）
	const int kSurfaceConstants = kLightConstants + 1;										// b3 G-Buffer の材質値（GBuffer.hlsli の GBufferMaterial）
	const int kGBufferPassConstants = kSurfaceConstants + 1;								// b4 G-Buffer のパスの定数（GBuffer.hlsli の GBufferPassConstants）
	static_assert( kGBufferPassConstants < GameLib::Graphics::Manager::MAX_CONSTANT_SLOT, "ShaderSlots : constant slot is out of range." );
}

#endif
