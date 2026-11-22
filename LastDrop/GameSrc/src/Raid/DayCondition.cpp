#include "GameSrc/Raid/DayCondition.h"

#include "GameLib/GameLib.h"

namespace{
	const float kKeep = 1.f;           // 変えない倍率
	const float kLightsOut = 0.f;      // 停電：街灯を消す
	const float kThickFog = 0.3f;      // 霧：霧が半分になる距離 40m → 12m
	const int kNormalZombies = 40;     // ふつうの日
	const int kHordeZombies = 55;      // 群れの日

	// DayCondition の順に並べる（static_assert で数を確かめる）
	const DayConditionEffect kEffects[] = {
		// 名前        見出し             街灯        霧          ゾンビ
		{ "normal",   "いつもの夜だ",   kKeep,      kKeep,      kNormalZombies },
		{ "blackout", "停電か",         kLightsOut, kKeep,      kNormalZombies },
		{ "fog",      "霧が濃い",       kKeep,      kThickFog,  kNormalZombies },
		{ "horde",    "ゾンビが多いな", kKeep,      kKeep,      kHordeZombies },
	};
	static_assert( sizeof( kEffects ) / sizeof( kEffects[ 0 ] ) == static_cast< size_t >( DayCondition::Count ),
		"kEffects must list every DayCondition" );
}

const DayConditionEffect& dayConditionEffect( DayCondition condition ) {
	int index = static_cast< int >( condition );
	ASSERT( index >= 0 && index < static_cast< int >( DayCondition::Count ) && "dayConditionEffect : bad condition." );
	return kEffects[ index ];
}
