#ifndef INCLUDED_GAMELIB_FILEIO_ARCHIVESCRAMBLE_H
#define INCLUDED_GAMELIB_FILEIO_ARCHIVESCRAMBLE_H

namespace GameLib{
namespace FileIO{

// アーカイブに入れたファイルの中身のかき混ぜ（ResourcePack_Plan.txt）
// 書籍の Archiver の形式は公開されていて、そのままだと誰でも取り出せる。配る素材を「普通には取り出せない形」にするため、決まった鍵で XOR する
// 同じ鍵を .pak を作る tools/release/make_pak.ps1 も持つ（ゲームの外のスクリプトなので 2 か所）。片方を変えたら両方を変える
namespace ArchiveScramble{
	const unsigned char kKey[] = {
		0x5A, 0xC3, 0x1E, 0x97, 0x6B, 0xF0, 0x24, 0x8D,
		0xB1, 0x3F, 0xE6, 0x72, 0x09, 0xD4, 0x58, 0xAE,
	};
	const int kKeySize = sizeof( kKey );

	// XOR なので、かき混ぜるのも戻すのも同じ。data の先頭は、そのファイルがアーカイブに入っている先頭のバイト
	inline void apply( char* data, int size ){
		for ( int i = 0; i < size; ++i ){
			data[ i ] = static_cast< char >( data[ i ] ^ kKey[ i % kKeySize ] );
		}
	}
}

} //namespace FileIO
} //namespace GameLib

#endif
