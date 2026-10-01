#include "Generators.h"

using namespace Synth;

// 足音は「かかとが着く → つま先が着く」の2打でできている。どの床でも骨組みは同じで、
// 床ごとに違うのは「何が鳴るか」（低い衝撃・擦れ・響き・粒・泡・破片）とその強さだけなので、
// 骨組みを1つの関数（makeFootstep）にし、床ごとの違いは表（kRecipes）の数値で表す。
namespace{
	const float kStepLength = 0.45f;	// 足音1つの長さ（秒）。水の泡やガラスの破片の余韻まで入る長さ
	const float kFadeOut = 0.05f;		// お尻を切るときのフェード（秒）
	// 揺らぎ（別テイク）ごとのばらつき。
	const float kToeDelayJitter = 0.2f;	// つま先が着くまでの時間を ±この割合
	const float kModeDetune = 0.04f;	// 床の響きの高さを ±この割合（同じ板でも踏む場所で響きが変わる）
	// 仕上げの潰し具合（Synth::saturate）。靴底の「カッ」のピークを少し丸め、体重の低音を前に出す。
	// 強くしすぎると「カッ」の輪郭まで消えて、もっさり（こもって締まりのない音）になる（2.0 では強すぎた）。
	const float kStepDrive = 1.3f;

	struct ThumpLayer{ float frequency, decay, gain; };				// 低い正弦波の一打（かかとの衝撃・体重）
	struct BoomLayer{ float cutoffHz, attack, decay, gain; };		// 低音だけのノイズの「ドスッ」（体重が乗る重さ）
	struct NoiseLayer{ float centerHz, q, attack, decay, gain; };	// 靴底の擦れ・打撃の「カッ」
	struct GrainLayer{ float duration; int count; float centerHz, q, grainDecay, gain; }; // 砂利・草・ガラスの粒
	struct ModeLayer{ const Mode* modes; int count; float gain; };	// 床そのものの響き（金属板・床板）
	struct BubbleLayer{ int count; float minHz, maxHz, rise, duration, window, gain; };	// 水の泡（rise 倍へ上がる）
	struct TinkleLayer{ int count; float minHz, maxHz, minDecay, maxDecay, window, gain; }; // ガラス片の「チリン」

	struct StepRecipe{
		SoundId id;
		ThumpLayer heel;	// かかとが床を打つ（短い）。つま先でも小さく鳴る
		// 体重が乗る重さ（かかとのときだけ）。装備を背負った重い足音は、ここの大きさで決まる。
		// 重くて締まった足音にするには、70〜90Hz を 50ms ほどで切る。
		// 40Hz 台や 100ms 超にすると尾を引いて「ボワン」ともっさり聞こえる。
		ThumpLayer body;
		BoomLayer boom;
		NoiseLayer scuff;
		float toeDelay;		// かかとからつま先までの時間（秒）
		float toeGain;		// つま先の一打の大きさ（かかとに対する割合）
		GrainLayer grains;
		ModeLayer modes;
		BubbleLayer bubbles;
		TinkleLayer tinkles;
		float peak;			// 正規化の大きさ。柔らかい床（草）は小さめにして、同じ音量でも控えめに聞こえるようにする
	};

	// 床の響き。金属板は倍音が整数倍にならない（板の振動）ので金属らしく聞こえる。
	// 一番低い 140Hz は、下が空洞の鉄板（足場・床のハッチ）が「ゴーン」と鳴る胴鳴り。
	const Mode kMetalModes[] = {
		{ 140.f, 0.16f, 0.6f },
		{ 310.f, 0.18f, 1.0f }, { 747.f, 0.12f, 0.6f }, { 1290.f, 0.09f, 0.45f },
		{ 2130.f, 0.06f, 0.3f }, { 3370.f, 0.04f, 0.2f },
	};
	// 床板は低く短く響く（中が空洞の板の「コトッ」）。一番低い 110Hz は床下の空洞の「ドン」。
	const Mode kWoodModes[] = {
		{ 110.f, 0.06f, 1.0f },
		{ 180.f, 0.07f, 0.8f }, { 420.f, 0.05f, 0.4f }, { 870.f, 0.03f, 0.2f },
	};
	const ModeLayer kNoModes = { nullptr, 0, 0.f };
	const GrainLayer kNoGrains = { 0.f, 0, 1.f, 1.f, 1.f, 0.f };
	const BubbleLayer kNoBubbles = { 0, 1.f, 1.f, 1.f, 1.f, 0.f, 0.f };
	const TinkleLayer kNoTinkles = { 0, 1.f, 1.f, 1.f, 1.f, 0.f, 0.f };

	// 並びは id, heel, body, boom, scuff, toeDelay, toeGain, grains, modes, bubbles, tinkles, peak。
	const StepRecipe kRecipes[] = {
		// コンクリート: 硬い床に体重が「ドッ」と乗る。擦れは乾いて短い、細かい砂の粒が少し。
		{ SOUND_FOOTSTEP_CONCRETE,
			{ 100.f, 0.02f, 0.9f }, { 75.f, 0.05f, 0.7f }, { 220.f, 0.002f, 0.025f, 0.55f },
			{ 1800.f, 0.9f, 0.001f, 0.018f, 0.68f }, 0.035f, 0.5f,
			{ 0.06f, 6, 3000.f, 1.2f, 0.003f, 0.14f }, kNoModes, kNoBubbles, kNoTinkles, 0.8f },
		// 金属板: 踏むと「ドン」の後に板が「ゴワン」と響く。
		{ SOUND_FOOTSTEP_METAL,
			{ 90.f, 0.025f, 0.8f }, { 70.f, 0.055f, 0.6f }, { 240.f, 0.002f, 0.03f, 0.5f },
			{ 2500.f, 1.0f, 0.001f, 0.01f, 0.52f }, 0.04f, 0.4f,
			kNoGrains, { kMetalModes, sizeof( kMetalModes ) / sizeof( kMetalModes[ 0 ] ), 0.35f }, kNoBubbles, kNoTinkles, 0.8f },
		// 木: 床下の空洞で低く「ドン」と鳴る。響きは短い。
		{ SOUND_FOOTSTEP_WOOD,
			{ 110.f, 0.03f, 1.0f }, { 80.f, 0.05f, 0.7f }, { 250.f, 0.002f, 0.03f, 0.55f },
			{ 900.f, 0.8f, 0.001f, 0.02f, 0.42f }, 0.04f, 0.5f,
			kNoGrains, { kWoodModes, sizeof( kWoodModes ) / sizeof( kWoodModes[ 0 ] ), 0.45f }, kNoBubbles, kNoTinkles, 0.8f },
		// 草: 土に体重が沈む鈍い「ドフッ」。葉が擦れて折れる粒が長めに続く。
		{ SOUND_FOOTSTEP_GRASS,
			{ 85.f, 0.025f, 0.6f }, { 70.f, 0.05f, 0.5f }, { 180.f, 0.004f, 0.03f, 0.5f },
			{ 4000.f, 0.5f, 0.015f, 0.08f, 0.42f }, 0.05f, 0.6f,
			{ 0.15f, 40, 5000.f, 1.5f, 0.003f, 0.35f }, kNoModes, kNoBubbles, kNoTinkles, 0.6f },
		// 砂利: 体重で石が沈む「ザクッ」。石同士がこすれる粒が主役。
		{ SOUND_FOOTSTEP_GRAVEL,
			{ 95.f, 0.025f, 0.8f }, { 75.f, 0.05f, 0.55f }, { 220.f, 0.002f, 0.025f, 0.5f },
			{ 2000.f, 0.7f, 0.005f, 0.05f, 0.22f }, 0.05f, 0.7f,
			{ 0.12f, 70, 2800.f, 1.2f, 0.004f, 0.6f }, kNoModes, kNoBubbles, kNoTinkles, 0.8f },
		// 水たまり: 水を押しのける低い「ドボッ」と、「バシャ」のノイズ、はじける泡。
		{ SOUND_FOOTSTEP_WATER,
			{ 80.f, 0.025f, 0.6f }, { 65.f, 0.05f, 0.45f }, { 280.f, 0.003f, 0.04f, 0.65f },
			{ 1500.f, 0.6f, 0.004f, 0.06f, 0.6f }, 0.05f, 0.5f,
			{ 0.1f, 10, 6000.f, 1.0f, 0.004f, 0.15f }, kNoModes,
			{ 7, 500.f, 1500.f, 2.5f, 0.04f, 0.15f, 0.35f }, kNoTinkles, 0.8f },
		// 割れたガラス: 体重で踏み割る「ジャリッ」と、破片が転がる高い「チリン」。
		{ SOUND_FOOTSTEP_GLASS,
			{ 100.f, 0.02f, 0.8f }, { 75.f, 0.05f, 0.55f }, { 220.f, 0.002f, 0.025f, 0.5f },
			{ 3000.f, 1.0f, 0.001f, 0.015f, 0.35f }, 0.04f, 0.6f,
			{ 0.1f, 50, 4000.f, 2.0f, 0.003f, 0.52f }, kNoModes, kNoBubbles,
			{ 8, 3000.f, 9000.f, 0.02f, 0.08f, 0.15f, 0.25f }, 0.8f },
	};

	const StepRecipe* findRecipe( SoundId id ) {
		for ( const StepRecipe& recipe : kRecipes ) {
			if ( recipe.id == id ) {
				return &recipe;
			}
		}
		return nullptr;
	}

	// かかと・つま先のどちらも同じ層を重ねる（つま先は gain 倍で小さく）。
	void addImpact( Buffer* buffer, const StepRecipe& recipe, float start, float gain, Random* random ) {
		addThump( buffer, start, recipe.heel.frequency, recipe.heel.decay, recipe.heel.gain * gain );
		const NoiseLayer& scuff = recipe.scuff;
		addNoiseHit( buffer, start, scuff.attack, scuff.decay, scuff.centerHz, scuff.q, scuff.gain * gain, random );
		const GrainLayer& grains = recipe.grains;
		if ( grains.count > 0 ) {
			addGrains( buffer, start, grains.duration, grains.count, grains.centerHz, grains.q, grains.grainDecay,
				grains.gain * gain, random );
		}
	}

	// 床の響き。揺らぎごとに少し高さを変える。
	void addFloorModes( Buffer* buffer, const ModeLayer& layer, float start, Random* random ) {
		for ( int i = 0; i < layer.count; ++i ) {
			Mode mode = layer.modes[ i ];
			mode.frequency *= 1.f + kModeDetune * random->bipolar();
			addModes( buffer, start, &mode, 1, layer.gain );
		}
	}

	void addBubbles( Buffer* buffer, const BubbleLayer& layer, Random* random ) {
		for ( int i = 0; i < layer.count; ++i ) {
			float from = random->range( layer.minHz, layer.maxHz );
			addChirp( buffer, random->unit() * layer.window, from, from * layer.rise, layer.duration, layer.gain );
		}
	}

	void addTinkles( Buffer* buffer, const TinkleLayer& layer, Random* random ) {
		for ( int i = 0; i < layer.count; ++i ) {
			Mode tinkle = { random->range( layer.minHz, layer.maxHz ), random->range( layer.minDecay, layer.maxDecay ), 1.f };
			addModes( buffer, random->unit() * layer.window, &tinkle, 1, layer.gain );
		}
	}
}

Buffer makeFootstep( SoundId id, Random* random ) {
	const StepRecipe* recipe = findRecipe( id );
	if ( !recipe ) {
		return Buffer(); // 表に無い床（表の書き忘れ）。空で返すと main が知らせる
	}
	Buffer buffer = makeBuffer( kStepLength );
	float toeDelay = recipe->toeDelay * ( 1.f + kToeDelayJitter * random->bipolar() );
	addImpact( &buffer, *recipe, 0.f, 1.f, random );
	addImpact( &buffer, *recipe, toeDelay, recipe->toeGain, random );
	// 体重はかかとに乗る瞬間に一度だけ（つま先では乗り切っているので鳴らさない）。
	addThump( &buffer, 0.f, recipe->body.frequency, recipe->body.decay, recipe->body.gain );
	const BoomLayer& boom = recipe->boom;
	addBoom( &buffer, 0.f, boom.attack, boom.decay, boom.cutoffHz, boom.gain, random );
	addFloorModes( &buffer, recipe->modes, 0.f, random );
	addBubbles( &buffer, recipe->bubbles, random );
	addTinkles( &buffer, recipe->tinkles, random );
	fadeEdges( &buffer, 0.f, kFadeOut );
	saturate( &buffer, kStepDrive );
	normalize( &buffer, recipe->peak );
	return buffer;
}
