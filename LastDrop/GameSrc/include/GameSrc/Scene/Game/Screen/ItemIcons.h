#ifndef INCLUDED_ITEM_ICONS_H
#define INCLUDED_ITEM_ICONS_H

#include "GameLib/Graphics/Texture.h"
#include "GameSrc/Interaction/Item/ItemCatalog.h"

// カバンの画面に出す物の絵（ItemSpec::icon のファイル）を、物の種類ごとに 1 枚ずつ持つ。
// 作った時に全種類の読み込みを始める（本編を始めるときに作れば、カバンを開く前に読み終わる）。
// どの物の絵を使うかの表は ItemCatalog にあり、ここは読んで持つだけ。描くのは InventoryScreen。
// 画面（バックバッファ）は sRGB ではなく、UI は ToneMap の後にそのまま描くので、絵は色の変換をせずに読む（TEXTURE_COLOR_SPACE_LINEAR）。
// SummaryPlan/個別仕様/ItemIcon_Plan.txt。
class ItemIcons{
public:
	ItemIcons();

	// 読み終えた id の絵。読み込み中・読めなかった（ファイルが無い）ときは nullptr（呼ぶ側は絵の代わりに物の色でマスを塗る）。
	// 読み込みは、これを呼んだときに進む（GameLib の Texture::isReady）。
	const GameLib::Graphics::Texture* icon( ItemId id );

private:
	GameLib::Graphics::Texture mTextures[ ITEM_NUMBER ];
};

#endif
