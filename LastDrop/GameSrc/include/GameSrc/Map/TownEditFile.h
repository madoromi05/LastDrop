#ifndef INCLUDED_TOWN_EDIT_FILE_H
#define INCLUDED_TOWN_EDIT_FILE_H

#include "GameSrc/Map/TownEdits.h"
#include <string>
#include <vector>

// 町の手直し（TownEdits）を XML のファイルへ書く・ファイルから読む
//   <TownEdit version="1">
//     <Move kind="Building" fromX="-25.03" fromZ="-25.03" x="-23" y="0" z="-25.03" yaw="90"/>
//     <Delete kind="Crate" fromX="8.2" fromZ="-16.69"/>
//     <Add kind="Crate" x="8" y="0" z="15.5" yaw="180"/>
//   </TownEdit>
// 読むのは全部のビルド（.pak からも読める）。書くのは ZF_EDITOR だけ
// 値は自分で文字にする（PseudoXml の float の配列は 4 桁で丸めるので、元の値へ戻らない）
namespace TownEditFile{

extern const char* const kPath;	// ゲームが読む手直しのファイルの場所

// path から読んで out へ入れる（out は先に空にする）。1 行ごとに確かめ、読めない行（属性の欠け・数でない値・知らない kind）は
// 入れずに warnings へ理由を足す。版の違いも warnings へ足し、読めた行は入れる
// ファイルが無いときは false で out は空・warnings は触らない（手直し無し＝町はそのまま）
// 読めない・<TownEdit> が無い（壊れている）ときは false で out は空・warnings の 1 行
// warnings の文は英字で書く。F6 の Town タブにも出すが、F6 の字形には日本語が無いため
bool load( const char* path, TownEdits* out, std::vector< std::string >* warnings );

#ifdef ZF_EDITOR
// edits を path へ書く。一時ファイルに書いてから置き換える（途中で止まっても前のファイルが壊れない）
// 書けなかったら false と、その理由を error に入れる。一時ファイルは消す
bool save( const char* path, const TownEdits& edits, std::string* error );
#endif

} //namespace TownEditFile

#endif
