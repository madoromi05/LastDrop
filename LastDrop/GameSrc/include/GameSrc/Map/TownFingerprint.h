#ifndef INCLUDED_TOWN_FINGERPRINT_H
#define INCLUDED_TOWN_FINGERPRINT_H

#include <cstdint>

class TownMap;

// 事前に作った床と表のファイルが、今の町から作る物と同じかを見分ける 64 ビットの値
// 作る材料の置き場所を cm に丸めた整数にして、FNV-1a で混ぜる。形式の番号も混ぜる
// 見るのは置き場所だけ。建物などのモデルの形を差し替えても値は変わらない（差し替えたら Bake し直す）
// 混ぜ方か、床と表の作り方（NavMeshBuilder・TownVisibility の決まり）を変えたら、.cpp の形式の番号を上げる
// （古いファイルの値が合わなくなり、読まずに作り直す）
namespace TownFingerprint{

// ゾンビの歩ける床。手直しを当てた後の、移動の当たり判定に入る物全部（建物・町を囲む建物・ゴミ箱・木箱・消火栓・街灯）と、
// escapeCarSpot 番の脱出の車の置き場所と、床の縁を壁から離す長さ agentRadius（m）から求める
std::uint64_t navMesh( const TownMap& map, int escapeCarSpot, float agentRadius );
std::uint64_t visibility( const TownMap& map );	// 建物の隠れの表。手直しを当てた後の建物と町を囲む建物の置き場所から求める

} //namespace TownFingerprint

#endif
