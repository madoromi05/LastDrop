#ifndef INCLUDED_LOADING_SCREEN_H
#define INCLUDED_LOADING_SCREEN_H

class UiRenderer;

// 出撃のロード画面（FuelEscapeLoading_Plan.txt 6 章・Q12）。黒い背景の真ん中に「DAY 2」、その下に今している段階の名前、下に進み具合の棒。
// 出撃を押したフレーム（BaseScene）から、GameScene のロードが終わるまで出す。描くだけで、何を読むかは知らない（GameLoader が決める）。
class LoadingScreen{
public:
	explicit LoadingScreen( UiRenderer* renderer ); // 借りるだけ（nullptr 不可）

	// 積んで描く（UiRenderer::draw まで。1 回の描画に 1 回だけ呼ぶ。ほかの画面と一緒には描かない）。
	// day: 何日目か。label: 今している段階の名前。progress: 進み具合（0〜1）。
	void draw( int day, const char* label, float progress );

private:
	UiRenderer* mRenderer;
};

#endif
