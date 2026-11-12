#ifndef INCLUDED_BODY_PART_H
#define INCLUDED_BODY_PART_H

// 体の部位。部位ごとの当たり判定（HitboxSet）が、どこに当たったかをこの番号で返す
// 将来の部位ごとの体力・部位破壊も、この番号で引く想定（並びを変えると、それらの表も合わせて直すことになる）
// 主人公もゾンビも同じ 10 部位。足（足首より先）には当たり判定が無い
// 部位を足すときは Count の直前に足す（途中に挿すと、番号で引いている表がずれる）
enum class BodyPart{
	Head,          // 頭（首は含まない）
	Torso,         // 胴（腰〜胸）
	LeftUpperArm,  // 左の上腕
	RightUpperArm, // 右の上腕
	LeftForeArm,   // 左の前腕
	RightForeArm,  // 右の前腕
	LeftThigh,     // 左の太もも
	RightThigh,    // 右の太もも
	LeftLowerLeg,  // 左のすね（膝〜足首。足は含まない）
	RightLowerLeg, // 右のすね（膝〜足首。足は含まない）
	Count,         // 部位の数（部位ではない）
};

// 部位の名前（デバッグ表示用。英語の短い名前）。Count など部位でないものは "-"
inline const char* bodyPartName( BodyPart part ){
	switch ( part ){
		case BodyPart::Head:          return "Head";
		case BodyPart::Torso:         return "Torso";
		case BodyPart::LeftUpperArm:  return "LeftUpperArm";
		case BodyPart::RightUpperArm: return "RightUpperArm";
		case BodyPart::LeftForeArm:   return "LeftForeArm";
		case BodyPart::RightForeArm:  return "RightForeArm";
		case BodyPart::LeftThigh:     return "LeftThigh";
		case BodyPart::RightThigh:    return "RightThigh";
		case BodyPart::LeftLowerLeg:  return "LeftLowerLeg";
		case BodyPart::RightLowerLeg: return "RightLowerLeg";
		case BodyPart::Count:         break;
	}
	return "-";
}

#endif
