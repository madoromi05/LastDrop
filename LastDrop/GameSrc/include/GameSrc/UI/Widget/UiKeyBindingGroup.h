#ifndef INCLUDED_UI_KEY_BINDING_GROUP_H
#define INCLUDED_UI_KEY_BINDING_GROUP_H

#include "GameSrc/UI/IUiElement.h"
#include "GameSrc/UI/UiAnchor.h"
#include "GameSrc/UI/UiRect.h"
#include <string>
#include <vector>

// 操作の一覧のまとまり 1 つ（「移動」など）。まとまりの名前を小さく書き、その下に行を並べる
// 行は、上に細い線・左に操作の名前・右に寄せたキーの枠（KeyCap）。描くだけで、選べない
// 横の位置と幅は列（UiColumn）で持ち、描くたびに今の画面の幅から求める
class UiKeyBindingGroup : public IUiElement {
public:
	// 行 1 つ
	struct Row {
		std::string action;              // 操作の名前
		std::vector< std::string > keys; // 枠で囲むキーの名前。左から並べる順
	};
	// column: 置く列。top: まとまりの名前の上端（画面の上端から。画素）
	// name: まとまりの名前。rows: 上から並べる順。空は不可
	UiKeyBindingGroup( const UiColumn& column, float top, const char* name, const std::vector< Row >& rows );

	void draw( UiRenderer& renderer ) const override;
	void setRows( const std::vector< Row >& rows );	// 並べる行を差し替える（操作のタブでキーボード・マウスとパッドを切り替えたとき）。空は不可

private:
	void drawRow( UiRenderer& renderer, const Row& row, const UiRect& rect ) const;	// 行 1 つを rect の範囲に描く

	UiColumn mColumn;
	float mTop;
	std::string mName;
	std::vector< Row > mRows;
};

#endif