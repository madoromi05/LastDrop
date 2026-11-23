#ifndef INCLUDED_TOWN_BAKE_FILE_H
#define INCLUDED_TOWN_BAKE_FILE_H

#include <cstdint>
#include <string>
#include <vector>

// 事前に作った床と表（中身はバイト列）のファイルを書く・読む
// ファイルの頭に 4 文字の印「TWNB」・形式の番号・指紋（TownFingerprint）・中身の長さを置き、その後に中身を続ける
// 読むのは全部のビルド（.pak からも読める）。書くのは ZF_EDITOR だけ
// 中身が何か（床か表か）は知らない。中身を作る・使うのは NavMesh と TownVisibility
namespace TownBakeFile{

extern const char* const kVisibilityPath;			// 建物の隠れの表のファイル
std::string navMeshPath( int escapeCarSpot );		// escapeCarSpot 番の脱出の車を置いた町の、ゾンビの歩ける床のファイル

// path を読み、頭が正しく指紋が fingerprint と同じなら、中身を payload に入れて true
// 無い・印か形式の番号が違う・指紋が違う・長さが頭と合わない（途中で切れている）ときは false で payload は空。どれも止まらない
bool read( const char* path, std::uint64_t fingerprint, std::vector< unsigned char >* payload );

#ifdef ZF_EDITOR
// 頭と payload を path へ書く。無いフォルダは作る。一時ファイルに書いてから置き換える（途中で止まっても前のファイルが壊れない）
// 書けなかったら false と、その理由を error に入れる。一時ファイルは消す
bool write( const char* path, std::uint64_t fingerprint, const std::vector< unsigned char >& payload, std::string* error );
#endif

} //namespace TownBakeFile

#endif
