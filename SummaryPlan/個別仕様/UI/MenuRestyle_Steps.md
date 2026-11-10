# MenuRestyle の書き方（段0・段1）

仕様は `MenuRestyle_Plan.txt`。ここは「どのファイルの何をどう書くか」だけ。
段0・段1 では **画面の見た目は変わらない**（タイトルが今と同じに見えれば成功）。

新しいファイルは Visual Studio のソリューションエクスプローラーで足す（`.vcxproj` と `.filters` に自動で入る）。
置き場所は今の部品と同じ：ヘッダーは `GameSrc/include/GameSrc/UI/Widget/`、実装は `GameSrc/src/UI/Widget/`、フィルターは `GameSrc\UI\Widget`。

---

## 段0 コメントを規約に合わせる（5 か所）

**済み**（ユーザーの依頼で Claude が直した。コメントだけなのでビルドの結果は変わらない）。

計画書の名前・★ は、クラス（struct・enum・namespace）の説明に 1 回だけ。履歴の言い方（今までの〜）も書かない。

**① `GameSrc/src/UI/UiRenderer.cpp` 17 行目**
```cpp
// 変更前
	const char* kMincho = "Yu Mincho";    // 静かで重い見た目（RE:2 風のメニュー。TitleScreen_Plan.txt）
// 変更後
	const char* kMincho = "Yu Mincho";    // 静かで重い見た目（RE:2 風のメニュー）
```

**② `GameSrc/src/UI/UiScreen.cpp` 28 行目**
```cpp
// 変更前
	// RE:2 風の左寄せのメニュー（画素。座標の高さ 480 で、モックの 720 の 2/3。TitleScreen_Plan.txt）。
// 変更後
	// RE:2 風の左寄せのメニュー（画素。座標の高さ 480 で、モックの 720 の 2/3）。
```

**③ `GameSrc/src/Scene/Menu/MenuLayout.cpp` 18 行目**
```cpp
// 変更前
	// RE:2 風（MENU_LOOK_SIDE）の見た目。位置は UiScreen、色は MenuPalette。TitleScreen_Plan.txt。
// 変更後
	// RE:2 風（MENU_LOOK_SIDE）の見た目。位置は UiScreen、色は MenuPalette。
```

**④ `GameSrc/include/GameSrc/Scene/Menu/MenuLayout.h` の enum MenuLook**
（enum の説明の行に計画書を書くのはよいが、中の 1 行ずつには書かない。「今までの形」は履歴の言い方）
```cpp
// 変更前
// メニューの最初のページの並べ方（見た目）。
enum MenuLook{
	MENU_LOOK_CENTER, // 見出しを上に、板のボタンを画面の中央に縦に（一時停止。今までの形）
	MENU_LOOK_SIDE,   // RE:2 風。題名・ひと言・板なしの行を左に寄せ、背景の絵の左を暗くする（タイトル。TitleScreen_Plan.txt）
};
// 変更後
// メニューの最初のページの並べ方（見た目）。TitleScreen_Plan.txt。
enum MenuLook{
	MENU_LOOK_CENTER, // 見出しを上に、板のボタンを画面の中央に縦に（一時停止）
	MENU_LOOK_SIDE,   // RE:2 風。題名・ひと言・板なしの行を左に寄せ、背景の絵の左を暗くする（タイトル）
};
```

**⑤ `GameSrc/include/GameSrc/UI/Widget/UiKeyHintRow.h` のクラスの説明**
```cpp
// 変更前
// 色は MenuPalette。TitleScreen_Plan.txt ★3。
// 変更後
// 色は MenuPalette。TitleScreen_Plan.txt。
```

---

## 段1 共通の部品を抜き出す

### 1-1 SelectionMarks（新しいファイル 2 つ）

選んでいる行の印（帯・上下の線・赤いひし形）。今は `UiMenuItem::drawSelectedMarks` にあるが、段3 のスライダーも同じ印を使うので名前空間へ出す。
UiMenuItem.cpp の無名の名前空間の定数のうち、字の間以外はここへ**移す**（2 か所に同じ定数を置かない）。

**`GameSrc/include/GameSrc/UI/Widget/SelectionMarks.h`（新しく作る）**
```cpp
#ifndef INCLUDED_SELECTION_MARKS_H
#define INCLUDED_SELECTION_MARKS_H

#include "GameSrc/UI/UiRect.h"

class UiRenderer;

// RE:2 風のメニューで「選んでいる行」に出す印（左から右へ薄れる帯・上下の細い線・左の赤いひし形）と、行の中で文字を書き始める所。
// 板なしの行（UiMenuItem）とスライダー（UiSlider）が同じ印・同じ文字の位置を使うので 1 か所に置く。色は MenuPalette。MenuRestyle_Plan.txt。
namespace SelectionMarks{
	// row（画面上の範囲）に印を積む。
	void draw( UiRenderer& renderer, const UiRect& row );
	// row の中で、ひし形の右に文字を書き始める x。印を出していない行も同じ所から書く（選び直しても文字が動かない）。
	float labelLeft( const UiRect& row );
}

#endif
```

**`GameSrc/src/UI/Widget/SelectionMarks.cpp`（新しく作る）**
```cpp
#include "GameSrc/UI/Widget/SelectionMarks.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/MenuPalette.h"

using namespace GameLib::Math;

namespace{
	// 行の中の並び（画素。座標の高さ 480 で）。左から 余白 → ひし形 → 間 → 文字。
	const float kPaddingX = 11.f;
	const float kMarkRadius = 3.5f;        // ひし形の中心から角まで
	const float kMarkDiameter = kMarkRadius * 2.f;
	const float kMarkToLabel = 12.f;       // ひし形の右の角から文字まで
	const float kLineThickness = 1.f;      // 上下の細い線の太さ
}

void SelectionMarks::draw( UiRenderer& renderer, const UiRect& row ) {
	renderer.addGradientRect( row, MenuPalette::SELECTED_BAND, MenuPalette::SELECTED_BAND_END );
	UiRect line = row;
	line.height = kLineThickness;
	renderer.addRect( line, MenuPalette::SELECTED_LINE ); // 上の線
	line.top = row.bottom() - kLineThickness;
	renderer.addRect( line, MenuPalette::SELECTED_LINE ); // 下の線
	renderer.addDiamond( Vector2( row.left + kPaddingX + kMarkRadius, row.center().y ), kMarkRadius, MenuPalette::ACCENT );
}

float SelectionMarks::labelLeft( const UiRect& row ) {
	return row.left + kPaddingX + kMarkDiameter + kMarkToLabel;
}
```

### 1-2 UiMenuItem を SelectionMarks に任せ、字の間を引数にする

「操作の一覧」「戻る」のような日本語の行は、英字の大文字ほど字の間を広げると字がばらばらに見えるので、作る側が選べるようにする。

**`GameSrc/include/GameSrc/UI/Widget/UiMenuItem.h`**
```cpp
// 変更前
class UiMenuItem : public IUiElement, public IUiSelectable{
public:
	// anchor / rect: 行の範囲（画素。マウスで選べる範囲もこれ）。rect.left は anchor からのずれ、rect.top は画面の上端から。
	// label: 書く文字（英語）。onActivate: 押したときの処理（空は不可）。
	UiMenuItem( UiAnchor anchor, const UiRect& rect, const char* label, std::function< void() > onActivate );
// 変更後
class UiMenuItem : public IUiElement, public IUiSelectable{
public:
	// 字の間（画素）。英字の大文字は広く並べ、日本語は詰め気味にする（日本語を広げると字がばらばらに見える）。
	static constexpr float kLatinSpacing = 5.f;
	static constexpr float kJapaneseSpacing = 2.4f;

	// anchor / rect: 行の範囲（画素。マウスで選べる範囲もこれ）。rect.left は anchor からのずれ、rect.top は画面の上端から。
	// label: 書く文字。onActivate: 押したときの処理（空は不可）。letterSpacing: 字の間（kLatinSpacing か kJapaneseSpacing）。
	UiMenuItem( UiAnchor anchor, const UiRect& rect, const char* label, std::function< void() > onActivate, float letterSpacing );
```
```cpp
// 変更前
	// 今の画面上の範囲。描くときと、カーソルが乗っているかを調べるときに同じ範囲を使う。
	UiRect screenRect() const;
	// 選ばれているときの線・帯・ひし形を積む。
	void drawSelectedMarks( UiRenderer& renderer, const UiRect& rect ) const;

	UiAnchor mAnchor;
	UiRect mRect; // left は mAnchor からのずれ（理由は UiAnchor.h）
	std::string mLabel;
	std::function< void() > mOnActivate;
	bool mSelected;
// 変更後
	// 今の画面上の範囲。描くときと、カーソルが乗っているかを調べるときに同じ範囲を使う。
	UiRect screenRect() const;

	UiAnchor mAnchor;
	UiRect mRect; // left は mAnchor からのずれ（理由は UiAnchor.h）
	std::string mLabel;
	std::function< void() > mOnActivate;
	float mLetterSpacing;
	bool mSelected;
```
クラスの説明の「選ばれている間は、上下の細い線・左から右へ薄れる帯・赤いひし形を出す」は「選ばれている間は SelectionMarks の印を出す」に変える。

**`GameSrc/src/UI/Widget/UiMenuItem.cpp`**
```cpp
// 変更前
#include "GameSrc/UI/Widget/UiMenuItem.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/MenuPalette.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	// 行の中の並び（画素。座標の高さ 480 で。モックの 720 の 2/3）。左から 余白 → ひし形 → 間 → 文字。
	const float kPaddingX = 11.f;
	const float kMarkRadius = 3.5f;   // ひし形の中心から角まで
	const float kMarkToLabel = 12.f;  // ひし形の右の角から文字まで
	const float kLineThickness = 1.f; // 上下の細い線の太さ
	const float kLetterSpacing = 5.f; // 字の間（文字の高さの 0.28 倍くらい。英字の大文字を広く並べる）
}

UiMenuItem::UiMenuItem( UiAnchor anchor, const UiRect& rect, const char* label, std::function< void() > onActivate ) :
	mAnchor( anchor ),
	mRect( rect ),
	mLabel( label ),
	mOnActivate( onActivate ),
	mSelected( false ) {
// 変更後（無名の名前空間はまるごと消す。定数は SelectionMarks.cpp と UiMenuItem.h へ移った）
#include "GameSrc/UI/Widget/UiMenuItem.h"
#include "GameSrc/UI/Widget/SelectionMarks.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/MenuPalette.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

UiMenuItem::UiMenuItem( UiAnchor anchor, const UiRect& rect, const char* label, std::function< void() > onActivate, float letterSpacing ) :
	mAnchor( anchor ),
	mRect( rect ),
	mLabel( label ),
	mOnActivate( onActivate ),
	mLetterSpacing( letterSpacing ),
	mSelected( false ) {
```
```cpp
// 変更前
void UiMenuItem::draw( UiRenderer& renderer ) const {
	UiRect rect = screenRect();
	if ( mSelected ) {
		drawSelectedMarks( renderer, rect );
	}
	const float kTwo = 2.f;
	Vector2 labelPosition( rect.left + kPaddingX + kMarkRadius * kTwo + kMarkToLabel, rect.center().y );
	renderer.addText( mLabel.c_str(), labelPosition, UI_TEXT_MENU, UI_ALIGN_MIDDLE_LEFT,
		mSelected ? MenuPalette::TEXT_SELECTED : MenuPalette::TEXT_DIM, kLetterSpacing );
}

void UiMenuItem::drawSelectedMarks( UiRenderer& renderer, const UiRect& rect ) const {
	…（関数ごと）…
}
// 変更後（drawSelectedMarks は関数ごと消す）
void UiMenuItem::draw( UiRenderer& renderer ) const {
	UiRect rect = screenRect();
	if ( mSelected ) {
		SelectionMarks::draw( renderer, rect );
	}
	Vector2 labelPosition( SelectionMarks::labelLeft( rect ), rect.center().y );
	renderer.addText( mLabel.c_str(), labelPosition, UI_TEXT_MENU, UI_ALIGN_MIDDLE_LEFT,
		mSelected ? MenuPalette::TEXT_SELECTED : MenuPalette::TEXT_DIM, mLetterSpacing );
}
```

**`GameSrc/src/Scene/Menu/MenuLayout.cpp` の buildSide**（引数が 1 つ増えた所。タイトルの行は英字の大文字）
```cpp
// 変更前
			canvas->add( std::make_unique< UiMenuItem >( UI_ANCHOR_LEFT, UiScreen::sideMenuItemRect( itemIndex ), button.label, activateOf( button, openPage ) ) );
// 変更後
			canvas->add( std::make_unique< UiMenuItem >( UI_ANCHOR_LEFT, UiScreen::sideMenuItemRect( itemIndex ), button.label, activateOf( button, openPage ), UiMenuItem::kLatinSpacing ) );
```

### 1-3 KeyCap（新しいファイル 2 つ）

キーの名前を細い枠で囲む所。今は `UiKeyHintRow::drawHint` の中にあるが、段4 の操作の一覧の行も同じ枠を使うので名前空間へ出す。
UiKeyHintRow.cpp の枠の定数（kFramePaddingX・kFramePaddingY・kFrameThickness・kBothSides）はここへ**移す**。

**`GameSrc/include/GameSrc/UI/Widget/KeyCap.h`（新しく作る）**
```cpp
#ifndef INCLUDED_KEY_CAP_H
#define INCLUDED_KEY_CAP_H

class UiRenderer;

// キーの名前を細い枠で囲んで描く（画面の下の案内 UiKeyHintRow と、操作の一覧の行）。字は UI_TEXT_CAPTION、色は MenuPalette。MenuRestyle_Plan.txt。
namespace KeyCap{
	// key を、左端 left・縦の真ん中 middleY の枠に入れて積む。枠の右端の x を返す（右に次の物を並べるため）。
	float draw( UiRenderer& renderer, const char* key, float left, float middleY );
	// key の枠の幅（描かずに測る。右に寄せて並べるとき、先に幅を知るため）。
	float width( UiRenderer& renderer, const char* key );
}

#endif
```

**`GameSrc/src/UI/Widget/KeyCap.cpp`（新しく作る）**
```cpp
#include "GameSrc/UI/Widget/KeyCap.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/MenuPalette.h"

#include "GameLib/GameLib.h"

namespace{
	// 画素（座標の高さ 480 で）。
	const float kFramePaddingX = 5.f;  // 枠の左右の余白
	const float kFramePaddingY = 1.f;  // 枠の上下の余白
	const float kFrameThickness = 1.f; // 枠の線の太さ
	const float kBothSides = 2.f;      // 余白は左右（上下）の 2 か所
	const float kHalf = 0.5f;
	const UiTextSize kTextSize = UI_TEXT_CAPTION;
}

float KeyCap::width( UiRenderer& renderer, const char* key ) {
	ASSERT( key && "KeyCap : key is null." );
	return renderer.textWidth( key, kTextSize ) + kFramePaddingX * kBothSides;
}

float KeyCap::draw( UiRenderer& renderer, const char* key, float left, float middleY ) {
	UiRect frame;
	frame.left = left;
	frame.width = width( renderer, key );
	frame.height = renderer.lineHeight( kTextSize ) + kFramePaddingY * kBothSides;
	frame.top = middleY - frame.height * kHalf;
	// 枠は細い長方形 4 本（上・下・左・右）。
	UiRect edge = frame;
	edge.height = kFrameThickness;
	renderer.addRect( edge, MenuPalette::KEY_FRAME );
	edge.top = frame.bottom() - kFrameThickness;
	renderer.addRect( edge, MenuPalette::KEY_FRAME );
	edge = frame;
	edge.width = kFrameThickness;
	renderer.addRect( edge, MenuPalette::KEY_FRAME );
	edge.left = frame.right() - kFrameThickness;
	renderer.addRect( edge, MenuPalette::KEY_FRAME );
	renderer.addText( key, frame.center(), kTextSize, UI_ALIGN_CENTER, MenuPalette::KEY_TEXT );
	return frame.right();
}
```

### 1-4 UiKeyHintRow を KeyCap に任せる

**`GameSrc/src/UI/Widget/UiKeyHintRow.cpp`**
```cpp
// 変更前
#include "GameSrc/UI/Widget/UiKeyHintRow.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/MenuPalette.h"
…
namespace{
	// 画素（座標の高さ 480 で。モックの 720 の 2/3）。
	const float kFramePaddingX = 5.f;   // 枠の左右の余白
	const float kFramePaddingY = 1.f;   // 枠の上下の余白
	const float kFrameThickness = 1.f;  // 枠の線の太さ
	const float kKeyToDescription = 7.f; // 枠の右から説明まで
	const float kHintGap = 19.f;        // 説明の右から次の案内の枠まで
	const float kHalf = 0.5f;
	const float kBothSides = 2.f;       // 余白は左右（上下）の 2 か所
	const UiTextSize kTextSize = UI_TEXT_CAPTION;
}
// 変更後
#include "GameSrc/UI/Widget/UiKeyHintRow.h"
#include "GameSrc/UI/Widget/KeyCap.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/MenuPalette.h"
…
namespace{
	// 画素（座標の高さ 480 で。モックの 720 の 2/3）。
	const float kKeyToDescription = 7.f; // 枠の右から説明まで
	const float kHintGap = 19.f;        // 説明の右から次の案内の枠まで
	const UiTextSize kTextSize = UI_TEXT_CAPTION; // 説明の字（枠の中の字と同じ大きさ）
}
```
```cpp
// 変更前
float UiKeyHintRow::drawHint( UiRenderer& renderer, const Hint& hint, float x, float middleY ) const {
	const float keyWidth = renderer.textWidth( hint.key.c_str(), kTextSize );
	UiRect frame;
	…（枠を描く所。renderer.addText( hint.key.c_str(), … ) まで）…
	const float descriptionX = frame.right() + kKeyToDescription;
	renderer.addText( hint.description.c_str(), Vector2( descriptionX, middleY ), kTextSize, UI_ALIGN_MIDDLE_LEFT, MenuPalette::TEXT_DIM );
	return descriptionX + renderer.textWidth( hint.description.c_str(), kTextSize ) + kHintGap;
}
// 変更後
float UiKeyHintRow::drawHint( UiRenderer& renderer, const Hint& hint, float x, float middleY ) const {
	const float descriptionX = KeyCap::draw( renderer, hint.key.c_str(), x, middleY ) + kKeyToDescription;
	renderer.addText( hint.description.c_str(), Vector2( descriptionX, middleY ), kTextSize, UI_ALIGN_MIDDLE_LEFT, MenuPalette::TEXT_DIM );
	return descriptionX + renderer.textWidth( hint.description.c_str(), kTextSize ) + kHintGap;
}
```

---

## 確かめ方
1. Debug・Release・Profile（x64）でビルドが通る。
2. タイトルを開き、**今と同じに見える**こと：選んでいる行の帯・線・赤いひし形、START などの字の間、下の [Enter] [Esc] の枠。
3. 書けたら Claude に「段1 書いた」と伝える → Claude がファイルを読んで、差分と規約（定数の置き場所・コメント）を確かめる。

## 参考
- Extract Function（同じ処理を 1 か所に出す）：https://refactoring.com/catalog/extractFunction.html
- C++ の `static constexpr` メンバー（C++17 から inline 変数として定義なしで使える）：https://en.cppreference.com/w/cpp/language/static#Constant_static_members
