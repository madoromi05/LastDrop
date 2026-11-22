#ifndef INCLUDED_GAMELIB_FILEIO_READWHOLE_H
#define INCLUDED_GAMELIB_FILEIO_READWHOLE_H

#include <vector>

namespace GameLib{
namespace FileIO{

// filename を FileIO（今の読み方でアーカイブかディスク）から丸ごと読み、読み終わるまで待つ
// InFile は読み込みのスレッドで読むので、すぐに中身が要る所（シェーダ・音）のためにまとめて待つ
// 無い・読めないときは false（InFile と違い、無くても止まらない）
bool readWhole( const char* filename, std::vector< char >* out );

} //namespace FileIO
} //namespace GameLib

#endif
