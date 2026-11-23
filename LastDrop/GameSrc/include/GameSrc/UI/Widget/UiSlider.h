#ifndef INCLUDED_UI_SLIDER_H
#define INCLUDED_UI_SLIDER_H

#include "GameSrc/UI/IUiElement.h"
#include "GameSrc/UI/Widget/IUiSelectable.h"
#include "GameSrc/UI/Widget/IUiAdjustable.h"
#include "GameSrc/UI/UiAnchor.h"
#include "GameSrc/UI/UiRect.h"
#include <functional>
#include <string>

// 名前・バー・数字を横に並べた 1 行（設定画面の音量・感度）。値は整数で、最小〜最大を刻みで動かす
// 値の持ち主は作る側（GameSettings など）で、ここは getter / setter の関数で読み書きするだけ
// （どの設定を変えるかを知らないので、項目が増えてもこのクラスは変わらない。UiMenuItem の onActivate と同じ考え方）
// 板は描かない。選ばれている間はメニューの行と同じ印（SelectionMarks）を出す。バーは細い線とひし形、色は MenuPalette
// 左右キー・バーのクリックとドラッグで値を変える（いつ変えるかは UiCanvas）
class UiSlider : public IUiElement, public IUiSelectable, public IUiAdjustable{
public:
	// 値の範囲と刻み
	struct Range{
		int minValue;
		int maxValue;
		int step;
	};

	// anchor / rect: 行の範囲（画素。rect.left は anchor からのずれ、rect.top は画面の上端から）
	// label: 左に書く名前。range: 値の範囲（minValue < maxValue、step > 0）
	// getValue: 今の値を返す。setValue: 新しい値（範囲に収めて刻みに揃えた物）を受け取る
	// formatValue: 右に書く数字の文字を作る（音量なら "80"、感度なら "1.00"）。どれも空は不可
	UiSlider( UiAnchor anchor, const UiRect& rect, const char* label, const Range& range,
		std::function< int() > getValue, std::function< void( int ) > setValue, std::function< std::string( int ) > formatValue );

	void draw( UiRenderer& renderer ) const override;
	bool contains( const GameLib::Math::Vector2& point ) const override;
	void setSelected( bool selected ) override;
	void activate() override;
	void adjust( int direction ) override;
	void dragTo( const GameLib::Math::Vector2& cursor ) override;

private:
	UiRect screenRect() const; // 行の今の画面上の範囲（理由は UiAnchor.h）
	UiRect barRect() const;    // バーの今の画面上の範囲（名前の欄と数字の欄の間。クリックで取れるよう、線より縦に広い）
	void setClamped( int value ); // 範囲に収め、刻みに揃えてから setValue へ渡す

	UiAnchor mAnchor;
	UiRect mRect;
	std::string mLabel;
	Range mRange;
	std::function< int() > mGetValue;
	std::function< void( int ) > mSetValue;
	std::function< std::string( int ) > mFormatValue;
	bool mSelected;
};

#endif
