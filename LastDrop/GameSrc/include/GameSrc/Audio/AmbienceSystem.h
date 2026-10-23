#ifndef INCLUDED_AMBIENCE_SYSTEM_H
#define INCLUDED_AMBIENCE_SYSTEM_H

#include "GameSrc/Audio/AudioEngine.h"

// マップの環境音を鳴らす係。今は脱出のサイレンだけを、遠くで一定の間隔で鳴らす。
// 風・鳥・虫・発電機・エアドロップの飛行機は 2026-10-23 に消した（ユーザー指定）。
// サイレンは今は時間だけで鳴らす（脱出の仕組みと結び付いていないため）。結び付けるときは、
// その出来事からサイレンを鳴らす形に替え、タイマーは消す。
//
// マップごとの違い（サイレンの場所）はまだ持たない（検証用のマップが1つだけのため）。
class AmbienceSystem{
public:
	// audio: 鳴らす先。所有権は受け取らない。この係より長く生きていること。
	explicit AmbienceSystem( AudioEngine* audio );
	AmbienceSystem( const AmbienceSystem& ) = delete;
	AmbienceSystem& operator=( const AmbienceSystem& ) = delete;

	// 1フレーム進める（サイレンの時間を数える）。
	void update();

private:
	AudioEngine* mAudio;
	int mFrame;							// 作ってからのフレーム数
	int mNextSirenFrame;				// 次にサイレンを鳴らすフレーム
};

#endif
