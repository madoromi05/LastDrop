#ifndef INCLUDED_UI_CANVAS_H
#define INCLUDED_UI_CANVAS_H

#include "GameSrc/UI/IUiElement.h"
#include "GameSrc/UI/Widget/IUiSelectable.h"
#include "GameSrc/UI/Widget/IUiAdjustable.h"
#include <memory>
#include <type_traits>
#include <vector>

struct UiInput;
class UiRenderer;

// 1画面ぶんの部品（IUiElement）を持ち、まとめて操作・描画する
// 選んで決定できる部品（IUiSelectable）のうち、どれを選んでいるかはここだけが持つ:
//   ・マウス：カーソルが乗った部品を選ぶ。クリックでその部品を決定する
//   ・キー：上下で選ぶ部品を移し（端で反対側へ回る）、決定キーで決定する。値を動かせない部品では左右でも移す（横に並べた行）
// 値を動かせる部品（IUiAdjustable。スライダー）は、決定の代わりに値を変える:
//   ・キー：左右で 1 刻みずつ
//   ・マウス：クリックした所の値にし、押したまま動かすと付いてくる（離すまで、カーソルが行の外へ出ても）
// 部品を足した順が、上下キーで選ぶ順になる。最初は、最初に足した選べる部品を選んでおく（キーだけでもすぐ操作できるように）
class UiCanvas{
public:
	UiCanvas();
	UiCanvas( const UiCanvas& ) = delete;
	UiCanvas& operator=( const UiCanvas& ) = delete;

	// 部品を持つ。返すポインタはこの UiCanvas がある間使える（HUD の文字を後から書き換えるなど）。element: nullptr 不可
	template< class T >
	T* add( std::unique_ptr< T > element ) {
		static_assert( std::is_base_of< IUiElement, T >::value, "UiCanvas : T must implement IUiElement." );
		T* raw = element.get();
		if constexpr ( std::is_base_of< IUiSelectable, T >::value ) {
			IUiAdjustable* adjustable = nullptr;
			if constexpr ( std::is_base_of< IUiAdjustable, T >::value ) {
				adjustable = raw;
			}
			addSelectable( raw, adjustable );
		}
		mElements.push_back( std::move( element ) );
		return raw;
	}

	// 入力で選ぶ部品を変え、決定されたら部品の activate を呼ぶ。1回の描画につき1回（押した瞬間を1回だけ拾うため）
	// activate の中でシーンの切り替えを頼んでもよい（切り替えは SceneManager が後で行うので、この UiCanvas はまだ消えない）
	void handleInput( const UiInput& input );
	void draw( UiRenderer& renderer ) const;	// 全部品を renderer へ積む（足した順に描く。後に足した部品が上に重なる）

private:
	void addSelectable( IUiSelectable* selectable, IUiAdjustable* adjustable ); // adjustable: 値を動かせない部品なら nullptr
	void select( int index );        // index 番目の選べる部品を選ぶ（範囲外なら何も選ばない）
	int findSelectableAt( const GameLib::Math::Vector2& point ) const; // point にある選べる部品の番号。無ければ -1

	std::vector< std::unique_ptr< IUiElement > > mElements;
	std::vector< IUiSelectable* > mSelectables; // mElements の中の選べる部品（借りている）。上下キーで選ぶ順
	std::vector< IUiAdjustable* > mAdjustables; // mSelectables と同じ番号の部品の、値を動かす窓口（動かせない部品は nullptr）
	int mSelected;                              // 選んでいる部品の mSelectables の番号。何も選んでいなければ -1
	int mDragging;                              // マウスで値を動かしている部品の番号（左ボタンを離すまで）。していなければ -1
};

#endif
