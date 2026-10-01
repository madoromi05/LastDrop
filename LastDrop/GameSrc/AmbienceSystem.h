#ifndef INCLUDED_AMBIENCE_SYSTEM_H
#define INCLUDED_AMBIENCE_SYSTEM_H

#include "AudioEngine.h"
#include <vector>

// マップの環境音を鳴らす係。
//  ・ベース: 風・鳥・虫を位置なし（2D）でずっと鳴らす
//  ・置き物: 発電機を決まった場所から（3D）ずっと鳴らす
//  ・イベント: エアドロップの飛行機の通過と、脱出のサイレンを一定の間隔で鳴らす
// イベントは今は時間だけで起こす（脱出やエアドロップの仕組みがまだ無いため）。仕組みができたら、
// その出来事から startAirplane / playSiren 相当を呼ぶ形に替え、タイマーは消す。
//
// マップごとの違い（どの環境音を鳴らすか・発電機の場所）はまだ持たない（検証用のマップが1つだけのため）。
class AmbienceSystem{
public:
	// audio: 鳴らす先。所有権は受け取らない。この係より長く生きていること。作った時点でベースと置き物を鳴らし始める。
	explicit AmbienceSystem( AudioEngine* audio );
	~AmbienceSystem();
	AmbienceSystem( const AmbienceSystem& ) = delete;
	AmbienceSystem& operator=( const AmbienceSystem& ) = delete;

	// 1フレーム進める（イベントの時間を数え、飛んでいる飛行機を動かす）。
	void update();

private:
	void startAirplane();
	void moveAirplane();	// 飛んでいる間、位置と速度を更新する。通り過ぎたら止める

	AudioEngine* mAudio;
	std::vector< AudioEngine::LoopHandle > mLoops;	// ずっと鳴らしている音（ベースと置き物）
	int mFrame;							// 作ってからのフレーム数
	int mNextAirplaneFrame;				// 次に飛行機を飛ばすフレーム
	int mNextSirenFrame;				// 次にサイレンを鳴らすフレーム
	AudioEngine::LoopHandle mAirplane;	// 飛んでいる飛行機の音（飛んでいなければ kInvalidLoop）
	int mAirplaneFrames;				// 飛行機が飛び始めてからのフレーム数
};

#endif
