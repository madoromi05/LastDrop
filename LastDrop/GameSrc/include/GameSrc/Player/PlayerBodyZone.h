#ifndef INCLUDED_PLAYER_BODY_ZONE_H
#define INCLUDED_PLAYER_BODY_ZONE_H

// 自キャラの体力を分ける部位（タルコフと同じ 7 部位。SummaryPlan/個別仕様/ZombieAttack_Plan.txt の 2 章）。
// 当たり判定の部位（BodyPart。主人公もゾンビも 10 部位）とは別の分け方。上腕と前腕は「腕」、太ももとすねは「脚」にまとめ、
// 胴は胸と腹に分ける。体力の表（PlayerHealth）と体の図（PlayerBodyFigure）は、この番号で引く。
// 部位を足すときは Count の直前に足す（途中に挿すと、番号で引いている表がずれる）。
enum class PlayerBodyZone{
	Head,     // 頭
	Thorax,   // 胸
	Stomach,  // 腹
	LeftArm,  // 左腕
	RightArm, // 右腕
	LeftLeg,  // 左脚
	RightLeg, // 右脚
	Count,    // 部位の数（部位ではない）
};

// 部位の名前（確認用の表示。英語の短い名前）。Count など部位でないものは "-"。
inline const char* playerBodyZoneName( PlayerBodyZone zone ){
	switch ( zone ){
		case PlayerBodyZone::Head:     return "Head";
		case PlayerBodyZone::Thorax:   return "Thorax";
		case PlayerBodyZone::Stomach:  return "Stomach";
		case PlayerBodyZone::LeftArm:  return "LeftArm";
		case PlayerBodyZone::RightArm: return "RightArm";
		case PlayerBodyZone::LeftLeg:  return "LeftLeg";
		case PlayerBodyZone::RightLeg: return "RightLeg";
		case PlayerBodyZone::Count:    break;
	}
	return "-";
}

#endif
