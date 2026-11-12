#ifndef INCLUDED_LOADING_SCREEN_H
#define INCLUDED_LOADING_SCREEN_H

class UiRenderer;

// 出撃のロード画面（FuelEscapeLoading_Plan.txt 6 章・Q12）。黒い背景の真ん中に見出し（今回の条件「霧の日」）、その下に目的の文（黄）、下に進み具合の棒
// GameScene のロードが終わるまで出す。描くだけで、何を読むかも何を出すかも知らない（読むのは GameLoader、文は呼ぶ側が渡す）
class LoadingScreen{
public:
	explicit LoadingScreen( UiRenderer* renderer ); // 借りるだけ（nullptr 不可）

	// 積んで描く（UiRenderer::draw まで。1 回の描画に 1 回だけ呼ぶ。ほかの画面と一緒には描かない）
	// heading: 見出し（大きい字）。goal: 見出しの下の目的の文（RaidGoal::TEXT）。progress: 進み具合（0〜1）
	void draw( const char* heading, const char* goal, float progress );

private:
	UiRenderer* mRenderer;
};

#endif
