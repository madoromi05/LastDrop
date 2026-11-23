#ifndef INCLUDED_ESCAPED_PAGE_H
#define INCLUDED_ESCAPED_PAGE_H

#include <functional>

class UiCanvas;
struct RaidResult;

// 脱出成功の画面（背景の絵・題名の上の小さいひと言「町を出た。」・ESCAPED と赤い線・結果の表・「タイトルへ」の行・下の操作の案内）
// 見た目はタイトルと同じ RE:2 風の左寄せ。何を並べるか（文言・表の行）はここだけに書く
namespace EscapedPage{
	// canvas に部品を足す。canvas: nullptr 不可。result: 表に書く結果（脱出した出撃の物）
	// toTitle: 「タイトルへ」が押されたときに呼ぶ。空は不可
	void build( UiCanvas* canvas, const RaidResult& result, std::function< void() > toTitle );
}

#endif
