#ifndef INCLUDED_UI_SCREEN_H
#define INCLUDED_UI_SCREEN_H

#include "GameSrc/UI/UiCanvas.h"
#include "GameSrc/UI/UiRect.h"
#include "GameSrc/UI/UiAnchor.h"
#include "GameLib/Math/Vector2.h"

class UiRenderer;

// 部品を並べた1画面（タイトル・クリア画面・本編の HUD）。部品の持ち主（UiCanvas）・描く道具（UiRenderer。借りる）
// 入力の読み取りを1つにまとめ、シーンは「何を並べるか」だけを書けばよいようにする（シーンが持つ部品として使う）
// 並べる位置の決まり（見出しの位置・ボタンの縦の並び）もここに置き、画面ごとに数字を書かないようにする
class UiScreen{
public:
	// renderer: 描く道具（フォントを持つので、全画面で1つを使い回す。GameResources が持つ）
	//           所有権は受け取らない。nullptr 不可。この画面より長く生きていること
	explicit UiScreen( UiRenderer* renderer );

	UiCanvas& canvas() { return mCanvas; }

	void handleInput();	// 入力を読んで部品を操作する。Pad::update の後、1回の描画につき1回（シーンの beginFrame で）呼ぶ
	void draw();		// 部品を画面に描く。1回の描画につき1回。3D を描く画面では 3D の後に呼ぶ（上に重ねるため）

	// ── 並べる位置（画素）──
	// x は基準（UiAnchor）からのずれで返す。括弧の中の基準と組にして部品へ渡すこと（理由は UiAnchor.h）
	// ── RE:2 風の左寄せのメニュー（どれも UI_ANCHOR_LEFT）──
	static GameLib::Math::Vector2 sideTitlePosition();				// 題名の左上（UI_ALIGN_LEFT で使う）
	static UiRect sideRuleRect();									// 題名の下の赤い短い線の範囲
	static GameLib::Math::Vector2 sideTaglinePosition();			// 赤い線の下のひと言の、左端の縦の真ん中（UI_ALIGN_MIDDLE_LEFT で使う）
	static GameLib::Math::Vector2 sideLeadPosition();				// 題名の上の小さいひと言の、左端の縦の真ん中（UI_ALIGN_MIDDLE_LEFT で使う。脱出成功の「町を出た。」）
	// 縦に並べる count 行のうち、index 番目（0 が一番上）の範囲
	// 一番下の行の位置は行の数によらず同じで、行が多いほど上へ伸びる
	static UiRect sideMenuItemRect( int index, int count );
	static GameLib::Math::Vector2 sideHeadingPosition();			// 設定・操作の一覧の見出しの左上（UI_ALIGN_LEFT で使う）
	static UiRect sideHeadingRuleRect();							// 見出しの下の赤い短い線の範囲
	static UiRect sideSettingRowRect( int index );					// 設定のスライダーの index 行目（0 が一番上）の範囲
	static UiRect sideSettingLinkRect( int rowCount, int index );	// rowCount 行のスライダーの下に、少し空けて並べるページを開く行（操作の一覧・戻る）の index 番目の範囲
	static UiColumn controlsColumn( int index, int count );			// 操作の一覧のまとまりを置く列（count 列のうち、左から index 番目）。画面の左右の余白の間を等分する
	static float controlsGroupTop();								// 操作の一覧のまとまりの名前の上端（画面の上端から）
	static UiRect sideBackItemRect();								// 中身の下に 1 つだけ置く行（操作の一覧の「戻る」・脱出成功の「タイトルへ」）の範囲
	static UiRect sideTableRowRect();								// 結果の表の 1 行目の範囲。2 行目からは同じ高さで下へ続ける（UiStatTable）
	static GameLib::Math::Vector2 keyHintPosition();				// 画面の下の操作の案内の、左端の縦の真ん中（UiKeyHintRow）
	// ── 真ん中に寄せる画面（ゲームオーバー。どれも UI_ANCHOR_CENTER）──
	static GameLib::Math::Vector2 centerHeadingPosition();			// 見出しの真ん中（UI_ALIGN_CENTER で使う）
	static UiRect centerRuleRect();									// 見出しの下の細い線の範囲
	static GameLib::Math::Vector2 centerSummaryPosition();			// 線の下の 1 行の真ん中（UI_ALIGN_CENTER で使う）
	static UiRect centerChoiceRect( int index, int count );			// 横に並べる count 個の行のうち、index 番目（0 が一番左）の範囲。並び全体を真ん中に置く
	static GameLib::Math::Vector2 centerKeyHintPosition();			// 画面の下の操作の案内の真ん中（UiKeyHintRow を UI_ALIGN_CENTER で使う）
	// ── 本編の HUD ──
	static GameLib::Math::Vector2 screenCenter();					// 画面の中央（照準。UI_ANCHOR_CENTER）
	static GameLib::Math::Vector2 statusLinePosition( int index );	// 画面の左下に並べる状態表示の index 行目（0 が一番下）の左上（UI_ANCHOR_LEFT。左揃えで使う）

private:
	UiCanvas mCanvas;
	UiRenderer* mRenderer;					// 借りているだけ
	GameLib::Math::Vector2 mPreviousCursor;	// 前回のカーソルの位置（動いたかどうかを決めるため）
};

#endif
