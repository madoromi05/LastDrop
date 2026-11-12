#ifndef INCLUDED_SPAWN_PICKER_H
#define INCLUDED_SPAWN_PICKER_H

#include <vector>

// 置き場所の候補から、重ならないようにいくつか選ぶ（ガソリンを候補点10個のうち3個に出す、など）。状態は持たない
// 選ぶ数を候補の数と同じにすれば、全部の候補を使う（＝固定の場所に出す）。ランダムと固定を同じ入口で扱える
class SpawnPicker{
public:
	// 0〜candidateCount-1 の番号から pickCount 個を、重ならないように選んで返す（順番もばらばら）
	// pickCount は 0〜candidateCount。seed が同じなら同じ結果になる（不具合を同じ配置で再現できるように）
	// 部分的な Fisher–Yates シャッフル（先頭から pickCount 個だけ混ぜる）で選ぶので、偏りが無い
	static std::vector< int > pick( int candidateCount, int pickCount, unsigned seed );
};

#endif
