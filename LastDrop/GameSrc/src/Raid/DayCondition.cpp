#include "GameSrc/Raid/DayCondition.h"

#include "GameLib/GameLib.h"
#include <cstring>

namespace{
	const float kKeep = 1.f;           // 変えない倍率
	const float kLightsOut = 0.f;      // 停電：街灯を消す
	const float kThickFog = 0.3f;      // 霧：霧が半分になる距離 40m → 12m
	const float kMuffledSteps = 0.5f;  // 雨：足音の届く距離 歩き 5 → 2.5m・走り 18 → 9m
	const int kNormalZombies = 30;     // ふつうの日
	const int kHordeZombies = 45;      // 群れの日（ふつうの 1.5 倍）

	// DayCondition の順に並べる（static_assert で数を確かめる）。
	const DayConditionEffect kEffects[] = {
		// 名前        見出し         一言                       街灯        霧          足音           ゾンビ
		{ "normal",   "夜", "いつもの町",               kKeep,      kKeep,      kKeep,         kNormalZombies },
		{ "blackout", "停電の日",   "街灯が全部消えている",     kLightsOut, kKeep,      kKeep,         kNormalZombies },
		{ "fog",      "霧の日",     "遠くが見えない",           kKeep,      kThickFog,  kKeep,         kNormalZombies },
		{ "rain",     "雨の日",     "足音が聞こえにくい",       kKeep,      kKeep,      kMuffledSteps, kNormalZombies },
		{ "horde",    "群れの日",   "ゾンビがいつもより多い",   kKeep,      kKeep,      kKeep,         kHordeZombies },
	};
	static_assert( sizeof( kEffects ) / sizeof( kEffects[ 0 ] ) == static_cast< size_t >( DayCondition::Count ),
		"kEffects must list every DayCondition" );
}

const DayConditionEffect& dayConditionEffect( DayCondition condition ) {
	int index = static_cast< int >( condition );
	ASSERT( index >= 0 && index < static_cast< int >( DayCondition::Count ) && "dayConditionEffect : bad condition." );
	return kEffects[ index ];
}

DayCondition dayConditionFromSaveName( const char* name ) {
	if ( !name ) {
		return DayCondition::Normal;
	}
	for ( int i = 0; i < static_cast< int >( DayCondition::Count ); ++i ) {
		if ( std::strcmp( kEffects[ i ].saveName, name ) == 0 ) {
			return static_cast< DayCondition >( i );
		}
	}
	return DayCondition::Normal;
}
