#ifndef INCLUDED_GAME_OVER_PAGE_H
#define INCLUDED_GAME_OVER_PAGE_H

#include <functional>

class UiCanvas;
struct RaidResult;

// ゲームオーバーの画面（背景の絵・真ん中に赤い GAME OVER と細い線・経過と倒した数の 1 行・横に並べた「もう一度」「タイトルへ」・下の操作の案内）
// 脱出成功と違い、全部を真ん中に寄せる。何を並べるか（文言）はここだけに書く
namespace GameOverPage{
	// canvas に部品を足す。canvas: nullptr 不可。result: 1 行に書く結果（死んだ出撃の物）
	// retry: 「もう一度」、toTitle: 「タイトルへ」が押されたときに呼ぶ。どちらも空は不可
	void build( UiCanvas* canvas, const RaidResult& result, std::function< void() > retry, std::function< void() > toTitle );
}

#endif
