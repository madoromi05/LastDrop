#ifndef INCLUDED_KEY_CAP_H
#define INCLUDED_KEY_CAP_H

class UiRenderer;

// キーの名前を細い枠で囲んで描く（画面の下の案内 UiKeyHintRow と、操作の一覧の行）。字は UI_TEXT_CAPTION、色は MenuPalette。MenuRestyle_Plan.txt
namespace KeyCap {
	// key を、左端 left・縦の真ん中 middleY の枠に入れて積む。枠の右端の x を返す（右に次の物を並べるため）
	float draw( UiRenderer& renderer, const char* key, float left, float middleY );
	// key の枠の幅（描かずに測る。右に寄せて並べるとき、先に幅を知るため）
	float width( UiRenderer& renderer, const char* key );
}

#endif