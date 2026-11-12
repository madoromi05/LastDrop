#ifndef INCLUDED_SOUNDGEN_SYNTH_H
#define INCLUDED_SOUNDGEN_SYNTH_H

#include <vector>

// 音を合成する部品。どれも「バッファの start 秒目から足し込む」形にして、部品を重ねるだけで1つの音を組み立てられるようにする
// （例: 足音 = かかとの低い衝撃 + 靴底の擦れのノイズ + 床の響き）
// バッファはモノラル・float（-1〜1）。書き出しは WaveWriter
namespace Synth{

const int kSampleRate = 44100;
typedef std::vector< float > Buffer;

// 種から毎回同じ列を出す乱数（xorshift32）。生成し直しても同じ音になり、差分で音が変わったかを確かめられる
class Random{
public:
	explicit Random( unsigned seed );
	float unit();						// 0〜1（1 は含まない）
	float bipolar();					// -1〜1（1 は含まない）
	float range( float low, float high );
	int index( int count );				// 0〜count-1
private:
	unsigned mState;
};

// 1極のローパス（高音を削る）
class OnePoleLowPass{
public:
	explicit OnePoleLowPass( float cutoffHz );
	void setCutoff( float cutoffHz );
	float process( float input );
private:
	float mCoefficient;
	float mState;
};

// バンドパス（中心の周波数のまわりだけ通す。RBJ の双2次フィルタ）
// q が大きいほど通す幅が狭く、ノイズが「その高さの音」に聞こえる
class BandPass{
public:
	BandPass( float centerHz, float q );
	void setCenter( float centerHz );	// 鳴らしている途中で動かしてもよい（衣擦れの「シュッ」の上がり下がり）
	float process( float input );
private:
	float mQ;
	float mB0, mB2, mA1, mA2;
	float mX1, mX2, mY1, mY2;
};

// 減衰する正弦波1本（金属や木の響きの成分）
struct Mode{
	float frequency;	// Hz
	float decay;		// 振幅が 1/e になるまでの秒数
	float amplitude;
};

int toSamples( float seconds );
Buffer makeBuffer( float seconds );
float attackDecay( float time, float attack, float decay );	// 立ち上がり attack 秒で 1 まで上がり、その後 decay 秒で 1/e ずつ下がる包絡

// ── 足し込む部品（バッファからはみ出す分は捨てる）──
// 帯域を絞ったノイズの一打（擦れ・ぶつかる瞬間の「ザッ」「カッ」）
void addNoiseHit( Buffer*, float start, float attack, float decay, float centerHz, float q, float gain, Random* );
void addNoiseSweep( Buffer*, float start, float duration, float fromHz, float toHz, float q, float gain, Random* );					// 中心を fromHz から toHz へ動かしながら鳴らすノイズ（衣擦れの「シュッ」）
void addModes( Buffer*, float start, const Mode* modes, int count, float gain );													// 減衰する正弦波の束（床や金属の響き）
void addThump( Buffer*, float start, float frequency, float decay, float gain );													// 少し音程が下がる低い一打（かかとが床を打つ「ドッ」）
void addBoom( Buffer*, float start, float attack, float decay, float cutoffHz, float gain, Random* );								// 低い帯域だけ残したノイズの一打（体重が乗る「ドスッ」）。正弦波の一打（addThump）より音程感が無く、重さだけが出る
void addGrains( Buffer*, float start, float duration, int count, float centerHz, float q, float grainDecay, float gain, Random* );	// 短いノイズの粒をばらまく（砂利・ガラス・草の「ジャリッ」）
void addChirp( Buffer*, float start, float fromHz, float toHz, float duration, float gain );										// 音程が上がりながら消える正弦波（水の泡がはじける「ポコッ」）
void addEcho( Buffer*, float delay, float gain, float cutoffHz );																	// 遅れて小さく返ってくる音を重ねる（遠くで鳴っている感じ・空間の広さ）。高音を削って返す

// ── 仕上げ ──
// 大きい所ほど緩やかに潰す（tanh のソフトサチュレーション）。drive が大きいほど強く潰す（1 未満は使わない）
// 鋭い一瞬のピーク（擦れ・金具の「カッ」）が丸まるので、normalize で同じピークにそろえたとき低音の長い成分が大きく残る
void saturate( Buffer*, float drive );
void normalize( Buffer*, float peak );					// 一番大きい所が peak になるよう全体の大きさをそろえる（音ごとの大きさはゲーム側の音量で決めるため）
void fadeEdges( Buffer*, float fadeIn, float fadeOut );	// 頭とお尻を滑らかにする（切れ目のプツッという音を消す）
// ループで鳴らしても継ぎ目が聞こえないよう、お尻の crossfade 秒を頭に重ねて畳む（長さはそのぶん短くなる）
// ノイズのように周期を持たない音に使う（周期のある音は、ループの長さに整数回ぴったり収まる周波数にする）
void makeLoopSeamless( Buffer*, float crossfade );
void mix( Buffer* dst, const Buffer& src, float gain );	// src を dst に gain 倍で足す（長さは短い方に合わせる）

} // namespace Synth

#endif
