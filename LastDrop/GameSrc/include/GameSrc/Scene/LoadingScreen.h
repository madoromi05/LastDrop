#ifndef INCLUDED_LOADING_SCREEN_H
#define INCLUDED_LOADING_SCREEN_H

class UiRenderer;

// 出撃のロード画面（FuelEscapeLoading_Plan.txt 6 章・Q12）。黒い背景の真ん中に見出し（今回の条件「霧の日」）、その下に今している段階の名前、下に進み具合の棒。
// GameScene のロードが終わるまで出す（2026-11-09 に 5 日間をやめ、見出しを「DAY 2」から条件の名前にした。SimpleRun_Plan.txt）。描くだけで、何を読むかは知らない（GameLoader が決める）。
class LoadingScreen{
public:
	explicit LoadingScreen( UiRenderer* renderer ); // 借りるだけ（nullptr 不可）

	// 積んで描く（UiRenderer::draw まで。1 回の描画に 1 回だけ呼ぶ。ほかの画面と一緒には描かない）。
	// heading: 見出し（大きい字）。label: 今している段階の名前。progress: 進み具合（0〜1）。
	void draw( const char* heading, const char* label, float progress );

private:
	UiRenderer* mRenderer;
};

#endif
