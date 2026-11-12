#ifndef INCLUDED_GAMELIB_PROFILING_OUTPUT_FILES_H
#define INCLUDED_GAMELIB_PROFILING_OUTPUT_FILES_H

#ifdef ZF_EDITOR

#include <string>

namespace GameLib{
namespace Profiling{

// 計測のファイルの書き出し先（Profiling/。.gitignore 済み）と、書き方の決まり
// 1 回の起動で出たファイルは、起動した時刻のフォルダ（Profiling/20261028_153000/）にまとめる
namespace OutputFiles{

// 今の時刻を、ファイル名に使える形にする（例：20261024_153000）
std::string timestamp();
// 今の日時を、人と Excel が読める形にする（例：2026/10/24 15:30:00）
std::string dateTime();
// この起動のフォルダの中のパスにする（フォルダが無ければ作る）。起動した時刻は最初に呼ばれたときに決める
std::string pathOf( const std::string& fileName );
// Profiling/ 直下のパスにする（フォルダが無ければ作る）。起動をまたいで使うファイル（起動の回数）だけが使う
std::string rootPathOf( const std::string& fileName );
// 一時ファイルに書いてから置き換える（書いている途中で落ちても、壊れたファイルを残さない）
bool write( const std::string& path, const std::string& text );
// テキストファイルを丸ごと読む。無ければ false
bool read( const std::string& path, std::string* text );

} //namespace OutputFiles

} //namespace Profiling
} //namespace GameLib

#endif // ZF_EDITOR

#endif
