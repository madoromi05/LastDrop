#ifndef INCLUDED_FLIPBOOK_H
#define INCLUDED_FLIPBOOK_H

#include "GameSrc/Render/LitQuadBatch.h"

// 1 枚の絵をます目に分けて並べた「コマ」の場所を答える（フリップブック。仕様は SummaryPlan/個別仕様/射撃と手応え/EffectFlipbook_Plan.txt の 4 章）
// コマは左上から右へ、行ごとに下へ 0, 1, 2 … と並ぶ（tools/texgen/smoke_flipbook_gen.py・muzzle_flash_gen.py が書く並び）
// 絵そのものは持たない（持ち主が Texture と一緒に持ち、LitQuadBatch::add へコマの UV を渡す）
// コマとコマの間はなめらかにつながない（2 コマを重ねて描かない。重ねると板が 2 倍になり、半透明の順番も崩れる）
class Flipbook{
public:
	// columns / rows: ます目の数（1 以上）。frameNumber: 使うコマの数（1 以上、columns × rows 以下）
	Flipbook( int columns, int rows, int frameNumber );

	int frameNumber() const;
	// 進み具合 progress（0 で最初のコマ、1 で最後のコマ）のコマの番号。範囲の外は端のコマ
	int frameAt( float progress ) const;
	// frame 番目のコマの UV の範囲（0〜frameNumber-1。範囲の外は端のコマ）
	QuadUv frameUv( int frame ) const;

private:
	int mColumns;
	int mRows;
	int mFrameNumber;
};

#endif
