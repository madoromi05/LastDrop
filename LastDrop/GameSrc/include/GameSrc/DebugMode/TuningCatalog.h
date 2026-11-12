#ifndef INCLUDED_TUNING_CATALOG_H
#define INCLUDED_TUNING_CATALOG_H

// 調整値の構造体の一覧と、まとめて行う操作（全部描く・書く・読む・既定値へ・C++ 化）。Debug・Profile ビルドだけ
// 構造体を足すときは、TuningCatalog.cpp の一覧に 1 行足すだけ（構造体の側は visit と、取り出す関数を持つ）
//
// プリセットのファイル（Tuning/<名前>.xml。起動した場所から見た所。git には入れない）:
//   <Tuning version="1">
//     <WeaponPose carryMuzzleDown="30" carryWristFromChest="-0.12, -0.12, 0.28" .../>
//     <Look lookDegreesPerPixel="0.15" .../>
//   </Tuning>
// 書くのは全部の項目（後でコードの既定値が変わっても、試した値のまま戻せるように）
// 起動したときには読まない（コードの初期値が正。読むのはエディタで Load を押したときだけ）
#ifdef ZF_EDITOR

#include <string>
#include <vector>

namespace TuningCatalog{

void drawAll();		// 全部の構造体を、見出しを付けて描く（F6 のエディタの欄の中で呼ぶ）
void resetAll();	// 全部の項目を既定値へ戻す
// 全部の項目を path へ書く。一時ファイルに書いてから置き換える（途中で止まっても前のファイルが壊れない）
// 書けなかったら false と、その理由を error に入れる
bool save( const std::string& path, std::string* error );
// まず全部を既定値へ戻してから、path の値を入れる（ファイルに無い項目は既定値）
// 知らない要素・属性、読めなかった値、版の違いは warnings に足す（止めずに読めた所まで入れる）。開けなかったら false
bool load( const std::string& path, std::vector< std::string >* warnings );
// 既定値と違う項目だけを、構造体ごとに初期値の形の文字列にする（違う項目が無ければ空）
// changedNumber: 違う項目の数を入れる（nullptr 可）
std::string toCpp( int* changedNumber );

} //namespace TuningCatalog

#endif // ZF_EDITOR

#endif
