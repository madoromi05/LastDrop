#ifndef INCLUDED_UI_KEY_HINT_ROW_H
#define INCLUDED_UI_KEY_HINT_ROW_H

#include "GameSrc/UI/IUiElement.h"
#include "GameSrc/UI/UiAnchor.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameLib/Math/Vector2.h"
#include <string>
#include <vector>

// 画面の下の操作の案内の列（「[Enter] 決定  [Esc] 戻る」）。キーの名前を細い枠で囲み、その右に説明を書き、横に並べる
// 案内の幅は文字の幅で決まるので、並べる位置はここで描くときに求める（作る側は列の左端か真ん中の位置だけを渡す）
class UiKeyHintRow : public IUiElement{
public:
	// 案内 1 つ。key: 枠の中に書くキーの名前。description: その右に書く説明
	struct Hint{
		std::string key;
		std::string description;
	};
	// anchor / position: 列の縦の真ん中の位置（画素）。x は anchor からのずれ、y は画面の上端から
	// hints: 左から並べる順。空は不可
	// align: position が列のどこか。UI_ALIGN_MIDDLE_LEFT（左端）か UI_ALIGN_CENTER（真ん中）
	UiKeyHintRow( UiAnchor anchor, const GameLib::Math::Vector2& position, const std::vector< Hint >& hints,
		UiAlign align = UI_ALIGN_MIDDLE_LEFT );

	virtual void draw( UiRenderer& renderer ) const;

private:
	float drawHint( UiRenderer& renderer, const Hint& hint, float x, float middleY ) const;	// 1 つの案内を x から描き、次の案内を始める x を返す
	float hintWidth( UiRenderer& renderer, const Hint& hint ) const;						// 1 つの案内の幅（枠・間・説明）
	float rowWidth( UiRenderer& renderer ) const;											// 列全体の幅

	UiAnchor mAnchor;
	GameLib::Math::Vector2 mPosition;
	std::vector< Hint > mHints;
	UiAlign mAlign;
};

#endif
