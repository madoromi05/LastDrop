#ifndef INCLUDED_UI_BACKDROP_H
#define INCLUDED_UI_BACKDROP_H

#include "GameSrc/UI/IUiElement.h"
#include "GameLib/Graphics/Texture.h"

// 画面いっぱいの背景の絵（タイトル画面の夜の町）。絵の縦横の比を保ったまま画面を覆うように貼り、はみ出た所は画面の外に出す
// （ウィンドウの縦横の比が変わっても絵をつぶさない）。暗くするのは UiShade で、ここは貼るだけ
// 絵は作るときに読み始め、読み終わるまでは何も描かない（読み込みは非同期）
// 絵は幅と高さが 2 の累乗の TGA にすること（GameLib は TGA を 2 の累乗へ切り上げて余白を作り、addImage は絵の全体を貼るので、
// 余白があると画面に出る）。画面写真からは tools/convert/screenshot_to_ui_tga.py、作る絵は tools/texgen で作る
class UiBackdrop : public IUiElement{
public:
	// filename: 絵のファイル。nullptr 不可。color: 絵に掛ける色（VertexColor::WHITE でそのまま。暗くするなら灰色）
	// alignX: 横がはみ出たとき、絵のどこを画面に合わせるか（0 で絵の左端を画面の左端に、0.5 で真ん中同士、1 で右端同士）
	//         絵の主役（街灯など）を、左に置く文字から離すのに使う
	UiBackdrop( const char* filename, unsigned color, float alignX );

	void draw( UiRenderer& renderer ) const override;

private:
	// 読み込みは isReady を呼んだときに進む（GameLib）。draw は const なので mutable にする
	// 読み込みを進めるだけで、描く内容（どの絵をどこに貼るか）は変えない
	mutable GameLib::Graphics::Texture mTexture;
	unsigned mColor;
	float mAlignX;
};

#endif
