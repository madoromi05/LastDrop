#ifndef INCLUDED_TOWN_BAKE_PANEL_H
#define INCLUDED_TOWN_BAKE_PANEL_H

#include "GameSrc/DebugMode/TownEditPanel.h"
#include "GameSrc/DebugMode/TownEditReload.h"

// F6 の Town タブの Bake の段。事前に作る床と表のファイルの状態を出し、今の手直しを当てた町で書き出す
//   状態の行：車の置き場所ごとの床と、隠れの表（file・stale・missing。TownBakeStatus）。ファイルを全部読むので、手直しが変わったときと書いた後だけ求め直す
//   Bake    ：今の出撃の車の置き場所の床と、表を書く
//   Bake all：床が古い車の置き場所を順に、その車で出撃を読み直してから書き、最後に押したときの車の置き場所へ戻る（TownBakeChain）
//   どちらも、手直しをファイルから変えていれば先に TownEdit.xml へ保存し、床は Release の既定の長さで作る。今の出撃が手直しより前の町なら、読み直してから書く
//   ファイルに読めなかった行があれば、保存するとその行が消えるので止まる（TownEditFileState）
//   書けなかったら止まり、理由を黄色で出す。終わったら、保存したか・書いたファイルを全部出す（TownBakeReport）
// 出撃の読み直しを待つ係は Apply と同じ 1 つ（TownEditPanel が持つ）を借りる。進めるのは F6 が開いている間だけ（閉じている間は止まって待つ）
// Debug・Profile ビルドだけ（Release では中身をビルドしない）
namespace TownBakePanel{

bool isRunning();	// Bake・Bake all の途中か（読み直しを待っている間も）。途中は Apply と Bake を押せなくする
// F6 が開いている間、Town タブを出していなくても 1 回の描画につき 1 回呼ぶ。Bake・Bake all を 1 段ずつ進める
// reload: Apply と共有の読み直しを待つ係（nullptr 不可）。reloaded: この描画で reload の poll が返した結果
void update( const TownEditPanel::Context& context, TownEditReload* reload, TownEditReload::Result reloaded );
void draw( const TownEditPanel::Context& context, const TownEditReload& reload );	// Town タブの中に Bake の段を出す。出撃中（context.town がある）だけ呼ぶ。reload は押せるかを見るだけ

} //namespace TownBakePanel

#endif
