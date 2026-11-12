#ifndef INCLUDED_BONE_NAME_PATTERN_H
#define INCLUDED_BONE_NAME_PATTERN_H

#include <vector>

class SkinnedMeshData;

// 骨を名前の表で引く、ただ1か所。状態は持たない
// 表の名前は「骨の名前の頭の共通部分（"Zombie_" など）を除いたもの」で書き、末尾が '*' なら、その前までで始まる骨を全部含める
// （指の骨 16 本を1つずつ書かないため。"LeftHand*" で手首と指をまとめて引く）
// のけぞり（ZombieFlinchLayout）と部位欠損（ZombieSeverLayout）が同じ書き方の表を使う
class BoneNamePattern{
public:
	// prefix + name に当てはまる骨の番号を out の後ろへ足す。当てはまる骨が無ければ何も足さない
	// data / prefix / name / out: nullptr 不可
	static void append( const SkinnedMeshData* data, const char* prefix, const char* name, std::vector< int >* out );
	static int find( const SkinnedMeshData* data, const char* prefix, const char* name );	// prefix + name の骨の番号（'*' は使えない）。無ければ -1
};

#endif
