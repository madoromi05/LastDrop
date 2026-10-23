#include "Generators.h"

using namespace Synth;

// 着弾音は「弾が当たった瞬間の破裂」と「その後に残る物」（破片が散る・砂が落ちる・金属が響く）でできている。
// どの材質でも骨組みは同じで、違うのは各層の高さと強さだけなので、足音（FootstepSynth.cpp）と同じく
// 骨組みを1つの関数（makeImpact）にし、材質ごとの違いは表（kRecipes）の数値で表す。
// 数値は初期値。ゲームの中で聞いて直す（SummaryPlan/個別仕様/ImpactSound_Plan.txt 6 章）。
namespace{
	const float kFadeOut = 0.05f;		// お尻を切るときのフェード（秒）
	// 揺らぎ（別テイク）ごとのばらつき。連射で同じ所に当たり続けても、毎回少し違う音にする。
	const float kCrackDetune = 0.1f;	// 破裂の高さを ±この割合
	const float kModeDetune = 0.05f;	// 金属の響きの高さを ±この割合（同じ車体でも当たる場所で響きが変わる）

	struct NoiseLayer{ float centerHz, q, attack, decay, gain; };	// 当たった瞬間の破裂（「パシッ」「ボスッ」）
	struct ThumpLayer{ float frequency, decay, gain; };				// 低い正弦波の一打（物の重さ）
	struct BoomLayer{ float cutoffHz, attack, decay, gain; };		// 低音だけのノイズ（土がえぐれる鈍さ）
	struct GrainLayer{ float start, duration; int count; float centerHz, q, grainDecay, gain; }; // 破片・砂が散る粒
	struct ModeLayer{ const Mode* modes; int count; float gain; };	// 物そのものの響き（金属）

	struct ImpactRecipe{
		SoundId id;
		float length;		// 音の長さ（秒）。響き・破片の余韻まで入る長さ
		NoiseLayer crack;
		ThumpLayer thump;
		BoomLayer boom;
		GrainLayer debris;
		ModeLayer modes;
		float peak;			// 正規化の大きさ
	};

	// 金属の響き。倍音が整数倍にならない（板の振動）ので金属らしく聞こえる。
	// 足音の鉄板（下が空洞の「ゴーン」）より高く、薄い車体を弾が叩く「カーン」の高さにする。
	const Mode kMetalModes[] = {
		{ 620.f, 0.3f, 1.0f }, { 1460.f, 0.22f, 0.7f }, { 2730.f, 0.15f, 0.5f }, { 4180.f, 0.1f, 0.35f },
	};
	const ModeLayer kNoModes = { nullptr, 0, 0.f };
	const GrainLayer kNoDebris = { 0.f, 0.f, 0, 1.f, 1.f, 1.f, 0.f };

	// 並びは id, length, crack, thump, boom, debris, modes, peak。
	const ImpactRecipe kRecipes[] = {
		// コンクリート・レンガ: 硬い面で弾が砕ける高い「パシッ」。削れた細かい破片がぱらぱら落ちる。
		{ SOUND_IMPACT_CONCRETE, 0.35f,
			{ 3000.f, 0.9f, 0.0005f, 0.012f, 1.0f }, { 140.f, 0.02f, 0.4f }, { 400.f, 0.001f, 0.03f, 0.4f },
			{ 0.02f, 0.25f, 30, 3500.f, 1.2f, 0.003f, 0.35f }, kNoModes, 0.8f },
		// 土・砂利: 柔らかい地面に弾がめり込む低く鈍い「ボスッ」。跳ねた砂が少し落ちる。
		{ SOUND_IMPACT_DIRT, 0.3f,
			{ 800.f, 0.7f, 0.001f, 0.025f, 0.8f }, { 80.f, 0.04f, 0.8f }, { 250.f, 0.002f, 0.06f, 0.7f },
			{ 0.04f, 0.22f, 25, 2000.f, 1.0f, 0.004f, 0.2f }, kNoModes, 0.8f },
		// 金属: 薄い板を叩く短い「カッ」の後に「カーン」と響く。破片は出ない。
		{ SOUND_IMPACT_METAL, 0.6f,
			{ 2500.f, 1.0f, 0.0005f, 0.008f, 0.9f }, { 120.f, 0.015f, 0.3f }, { 300.f, 0.001f, 0.02f, 0.2f },
			kNoDebris, { kMetalModes, sizeof( kMetalModes ) / sizeof( kMetalModes[ 0 ] ), 0.5f }, 0.8f },
	};

	const ImpactRecipe* findRecipe( SoundId id ) {
		for ( const ImpactRecipe& recipe : kRecipes ) {
			if ( recipe.id == id ) {
				return &recipe;
			}
		}
		return nullptr;
	}

	// 物の響き。揺らぎごとに少し高さを変える。
	void addBodyModes( Buffer* buffer, const ModeLayer& layer, Random* random ) {
		for ( int i = 0; i < layer.count; ++i ) {
			Mode mode = layer.modes[ i ];
			mode.frequency *= 1.f + kModeDetune * random->bipolar();
			addModes( buffer, 0.f, &mode, 1, layer.gain );
		}
	}
}

Buffer makeImpact( SoundId id, Random* random ) {
	const ImpactRecipe* recipe = findRecipe( id );
	if ( !recipe ) {
		return Buffer(); // 表に無い材質（表の書き忘れ）。空で返すと main が知らせる
	}
	Buffer buffer = makeBuffer( recipe->length );
	const NoiseLayer& crack = recipe->crack;
	float crackHz = crack.centerHz * ( 1.f + kCrackDetune * random->bipolar() );
	addNoiseHit( &buffer, 0.f, crack.attack, crack.decay, crackHz, crack.q, crack.gain, random );
	addThump( &buffer, 0.f, recipe->thump.frequency, recipe->thump.decay, recipe->thump.gain );
	const BoomLayer& boom = recipe->boom;
	addBoom( &buffer, 0.f, boom.attack, boom.decay, boom.cutoffHz, boom.gain, random );
	const GrainLayer& debris = recipe->debris;
	if ( debris.count > 0 ) {
		addGrains( &buffer, debris.start, debris.duration, debris.count, debris.centerHz, debris.q, debris.grainDecay,
			debris.gain, random );
	}
	addBodyModes( &buffer, recipe->modes, random );
	fadeEdges( &buffer, 0.f, kFadeOut );
	normalize( &buffer, recipe->peak );
	return buffer;
}
