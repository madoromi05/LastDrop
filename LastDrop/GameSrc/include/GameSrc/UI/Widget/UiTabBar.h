#ifndef INCLUDED_UI_TAB_BAR_H
#define INCLUDED_UI_TAB_BAR_H

#include "GameSrc/UI/IUiElement.h"
#include "GameSrc/UI/Widget/IUiSelectable.h"
#include "GameSrc/UI/Widget/IUiAdjustable.h"
#include "GameSrc/UI/UiRect.h"
#include "GameSrc/UI/UiRenderer.h"
#include <functional>
#include <string>
#include <vector>

// タブの名前を横に並べた 1 行（設定のページの「音声・感度・操作」と、操作のタブの中の「キーボード・マウス／パッド」）
// 今のタブの名前は明るくして下に赤い線を引き、ほかは暗くする。行が選ばれている間は、ほかの行と同じ印（SelectionMarks）を出す
// タブは値と同じように動かす（IUiAdjustable）。選ばれている間は左右で 1 つずつ（端で反対側へ回る）、クリックした所の名前のタブにする
// タブが変わったときの処理（ページを作り直す・一覧の行を差し替える）は作る側が onChange で渡す。OptionsTabs_Plan.txt
class UiTabBar : public IUiElement, public IUiSelectable, public IUiAdjustable{
public:
	// rect: 行の範囲（画素。左上が原点。マウスで選べる範囲もこれ）。labels: 左から並べるタブの名前。空は不可
	// current: 始めのタブの番号。tabWidth: タブ 1 つの幅（名前はその左端から書く。クリックで選ぶ範囲もこれ）
	// textSize: 名前の文字の大きさ。onChange: タブが変わったときに、新しいタブの番号で呼ぶ。空は不可
	UiTabBar( const UiRect& rect, const std::vector< std::string >& labels, int current, float tabWidth, UiTextSize textSize,
		std::function< void( int ) > onChange );

	void draw( UiRenderer& renderer ) const override;
	bool contains( const GameLib::Math::Vector2& point ) const override;
	void setSelected( bool selected ) override;
	void activate() override;												// 次のタブへ（パッドの決定ボタンだけでも切り替えられるように）
	void adjust( int direction ) override;
	void dragTo( const GameLib::Math::Vector2& cursor ) override;			// cursor の下の名前のタブにする（名前の無い所なら何もしない）

private:
	void change( int index );		// index のタブにし、変わったときだけ onChange を呼ぶ
	float tabLeft( int index ) const;	// index 番目のタブの左端の x（名前を書き始める所）

	UiRect mRect;
	std::vector< std::string > mLabels;
	int mCurrent;
	float mTabWidth;
	UiTextSize mTextSize;
	std::function< void( int ) > mOnChange;
	bool mSelected;
};

#endif
