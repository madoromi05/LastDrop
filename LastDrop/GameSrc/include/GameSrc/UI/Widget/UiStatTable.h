#ifndef INCLUDED_UI_STAT_TABLE_H
#define INCLUDED_UI_STAT_TABLE_H

#include "GameSrc/UI/IUiElement.h"
#include "GameSrc/UI/UiAnchor.h"
#include "GameSrc/UI/UiRect.h"
#include <string>
#include <vector>

// 名前と値の表（結果の画面の「かかった時間　14:07」など）。行ごとに、左に名前・右に寄せた値を書く
// 行の上に細い線を引き、最後の行は下にも引く。描くだけで、選べない
class UiStatTable : public IUiElement{
public:
	// 行 1 つ
	struct Row{
		std::string name;	// 左に書く名前
		std::string value;	// 右に寄せて書く値
	};
	// anchor / firstRow: 1 行目の範囲（画素）。firstRow.left は anchor からのずれ、firstRow.top は画面の上端から
	// 2 行目からは同じ幅と高さで下へ続ける。rows: 上から並べる順。空は不可
	UiStatTable( UiAnchor anchor, const UiRect& firstRow, const std::vector< Row >& rows );

	void draw( UiRenderer& renderer ) const override;

private:
	void drawRow( UiRenderer& renderer, const Row& row, const UiRect& rect ) const;	// 行 1 つ（上の線・名前・値）を rect の範囲に描く

	UiAnchor mAnchor;
	UiRect mFirstRow; // left は mAnchor からのずれ（理由は UiAnchor.h）
	std::vector< Row > mRows;
};

#endif
