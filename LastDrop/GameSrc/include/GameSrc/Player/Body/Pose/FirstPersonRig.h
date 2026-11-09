#ifndef INCLUDED_FIRST_PERSON_RIG_H
#define INCLUDED_FIRST_PERSON_RIG_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"
#include <vector>

class SkinnedMeshData;

// 一人称の体の書き換えの段（Player/Body/Pose/ の BonePoseModifier たち）が、みんなで読む物
// （SummaryPlan/個別仕様/設計と分割/HighPrioritySplit_Plan.txt 5-1。2026-11-09 FirstPersonBody から分けた）。
// 持ち主は FirstPersonBody。段は借りて読むだけ（目の位置だけは EyePoint が書く）。
// 座標はすべて骨格の根の空間（前方 +Z・上 +Y・単位 m）。骨は Mixamo の名前で探し、見つからない骨は飛ばす（その部分が動かないだけで落ちない）。
struct FirstPersonRig{
	// 背骨1本ぶん。この関節を中心に、自分と先にある骨（上半身）をまとめて回す。
	struct SpineJoint{
		int bone;
		std::vector< int > followers; // 一緒に回す骨（自分を含む）
	};

	// ── 骨（作るときに決まる）──
	std::vector< SpineJoint > spine;   // 根に近い順
	int spineDivisions;                // 視線の上下を何本へ振り分けるか（背骨の名前の数。見つからない骨があっても変えない）
	std::vector< int > armBones;       // 両肩から先（腕・手・指）
	std::vector< int > rightHandBones; // 右手と右手の指（手首を曲げるときに一緒に回す）
	int headBone;                      // 目の位置を決める骨。-1 なら足元から一定の高さ
	int chestBone;                     // 走るときに銃を寄せる基準の骨・腕を差し替えるときの基準の骨。-1 ならどちらもしない

	// ── 毎ティック変わる物 ──
	float pitch; // 視点の上下（度。正で見下ろす。FirstPersonCamera::pitch と同じ約束）

	// ── 手に持つ銃（FirstPersonBody::setWeapon で決まる）──
	int handBone;                               // 銃を付けた骨。-1 なら銃なし（銃に合わせる段はどれも何もしない）
	GameLib::Math::Matrix34 grip;               // 銃のモデル座標 → 骨の空間（HeldItem に渡したのと同じ持ち方）
	GameLib::Math::Vector3 sightEye;            // 覗いたときに目を置く点（銃のモデル座標）
	int supportHandBone;                        // 銃を支える手。-1 なら両手持ちの補正をしない
	GameLib::Math::Vector3 supportHandOnWeapon; // 支える手の骨が来るべき点（銃のモデル座標）
	GameLib::Math::Vector3 weaponMin;           // 銃の頂点の範囲（銃のモデル座標）
	GameLib::Math::Vector3 weaponMax;

	// ── 段が書く物 ──
	GameLib::Math::Vector3 eyePosition; // 目の位置（EyePoint が背骨を曲げた後に決める。銃を目へ寄せる段・ぶれの段が読む）

	// data の骨の番号を引いて作る（覚えない）。data: nullptr 不可、isReady() 済みであること。
	explicit FirstPersonRig( const SkinnedMeshData* data );

	// 銃を持っているか（銃に合わせる段は、持っていなければ何もしない）。
	bool hasWeapon() const;
	// 今のポーズでの銃の置き場所（HeldItem が描くのと同じ「手の骨 * 持ち方」）。hasWeapon のときだけ呼ぶ。
	GameLib::Math::Matrix34 weaponPlacement( const std::vector< GameLib::Math::Matrix34 >& pose ) const;
	// 視線の向き（前方 +Z をピッチぶん下へ傾けた回転）。目・胸・銃の向きをそろえる基準。
	GameLib::Math::Matrix34 viewRotation() const;
};

#endif
