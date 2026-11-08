#include "GameSrc/Raid/RunRules.h"
#include "GameSrc/Raid/RaidParams.h"
#include "GameSrc/Raid/RunStateFile.h"
#include "GameSrc/Weapon/WeaponCatalog.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Random.h"
#include "Framework.h"
#include <algorithm>
#include <cmath>
#include <iterator>
#include <utility>

namespace{
	const int kFirstDay = 1;

	// 5 日の条件の並べ方（DayCondition_Plan.txt Q2）：1 日目はふつう（町を覚える日）・5 日目は群れ（山場を最終日にまとめる）・
	// その間は停電・霧・雨を 1 回ずつ混ぜる（同じ条件が続かない・群れは 1 回だけ）。並べ方を変えるときはここだけを直す。
	const DayCondition kFirstDayCondition = DayCondition::Normal;
	const DayCondition kLastDayCondition = DayCondition::Horde;
	const DayCondition kMiddleConditions[] = { DayCondition::Blackout, DayCondition::Fog, DayCondition::Rain };
	const int kMiddleConditionNumber = sizeof( kMiddleConditions ) / sizeof( kMiddleConditions[ 0 ] );
	const int kFirstAndLastDays = 2;
	static_assert( kMiddleConditionNumber + kFirstAndLastDays == RunState::kDayCount, "the conditions must fill every day" );

	// 5 日分の条件を決める（5 日間の始めに 1 回。遊ぶたびに変わるよう、起動からの時間を種にする）。
	void rollConditions( RunState* run ) {
		DayCondition middle[ kMiddleConditionNumber ];
		std::copy( std::begin( kMiddleConditions ), std::end( kMiddleConditions ), middle );
		GameLib::Math::Random random = GameLib::Math::Random::create( static_cast< int >( GameLib::Framework::instance().time() ) );
		for ( int i = kMiddleConditionNumber - 1; i > 0; --i ) { // Fisher–Yates
			std::swap( middle[ i ], middle[ random.getInt( i + 1 ) ] );
		}
		run->setCondition( kFirstDay, kFirstDayCondition );
		for ( int i = 0; i < kMiddleConditionNumber; ++i ) {
			run->setCondition( kFirstDay + 1 + i, middle[ i ] );
		}
		run->setCondition( RunState::kDayCount, kLastDayCondition );
	}

	// 始めの倉庫（1 日目・ゲームオーバーの後。R2_Plan.txt Q14）。9mm の弾 48 発＝1 マス 24 発 × 2 マス。
	struct StartItem{
		ItemId item;
		int amount;
	};
	const StartItem kStartStash[] = {
		{ ITEM_AMMO_9MM, 24 },
		{ ITEM_AMMO_9MM, 24 },
	};

	// 始めのカバンに入れる銃（弾倉いっぱい。InventoryPickup_Plan.txt Q1）。
	// 2026-11-05 に「出撃のたびに、倉庫にもカバンにも無ければタダで渡す」をやめた（死ぬと 5 日間が終わって始めの形に戻るので、
	// 回の途中で拳銃を失うことが無くなった）。
	const WeaponId kStartWeapon = WEAPON_PISTOL;

	int roundToInt( float value ) {
		return static_cast< int >( std::lround( value ) );
	}

	// 5 日間を終える（クリア・ゲームオーバー共通）。スコアを数え、ハイスコアを直し、1 日目の形に戻して保存する。
	// survivedDays: 脱出できた日の数。itemPoints: スコアに入れる物の点（クリアなら倉庫とカバン、ゲームオーバーなら倉庫だけ）。
	void endRun( RunState* run, RaidEnding ending, int day, int survivedDays, int itemPoints ) {
		const RaidParams& params = raidParams();
		RunEnd end;
		end.ending = ending;
		end.day = day;
		end.score.daysSurvived = survivedDays;
		end.score.kills = run->kills();
		end.score.dayPoints = roundToInt( survivedDays * params.scorePerDay );
		end.score.killPoints = roundToInt( run->kills() * params.scorePerKill );
		end.score.itemPoints = itemPoints;
		end.score.clearBonus = ( ending == RaidEnding::Survived ) ? roundToInt( params.clearBonus ) : 0;
		end.newBest = end.score.total() > run->bestScore();
		if ( end.newBest ) {
			run->setBestScore( end.score.total() );
		}
		run->setLastRunEnd( end );
		run->setRunEndPending( true );
		RunRules::startNewRun( run );
		RunStateFile::save( *run );
	}
}

namespace RunRules{

void startNewRun( RunState* run ) {
	ASSERT( run && "RunRules::startNewRun : run is null." );
	run->stash().clearAll();
	run->bag().clearAll();
	for ( const StartItem& start : kStartStash ) {
		ItemStack stack = { start.item, start.amount };
		run->stash().placeAnywhere( stack );
	}
	const WeaponSpec& weapon = weaponSpec( kStartWeapon );
	ItemStack gun = { weapon.item, 1 };
	gun.loadedRounds = weapon.magazineCapacity;
	run->bag().placeAnywhere( gun );
	run->setDay( kFirstDay );
	run->setKills( 0 );
	rollConditions( run ); // 5 日分を始めに決める（その日をやり直しても引き直せない。DayCondition_Plan.txt Q3）
}

void loadOrStart( RunState* run ) {
	ASSERT( run && "RunRules::loadOrStart : run is null." );
	if ( !RunStateFile::load( run ) ) {
		startNewRun( run );
	}
}

void deploy( RunState* run ) {
	ASSERT( run && "RunRules::deploy : run is null." );
	RunStateFile::save( *run );
}

void finishDay( RunState* run, const DayResult& day, const ItemGrid& bagAfterRaid ) {
	ASSERT( run && day.ending == RaidEnding::Survived && "RunRules::finishDay : not survived." );
	run->bag() = bagAfterRaid;
	run->setKills( run->kills() + day.kills );
	DayResult result = day;
	result.day = run->day();
	result.broughtBack = bagAfterRaid.stacks();
	run->setLastDay( result );
	if ( run->day() >= RunState::kDayCount ) {
		// 5 日目に脱出した＝クリア。持ち帰った物は倉庫とカバンの全部。
		endRun( run, RaidEnding::Survived, run->day(), RunState::kDayCount, itemPoints( run->stash() ) + itemPoints( run->bag() ) );
		return;
	}
	run->setDay( run->day() + 1 );
	RunStateFile::save( *run );
}

void failDay( RunState* run, const DayResult& day ) {
	ASSERT( run && ( day.ending == RaidEnding::Killed || day.ending == RaidEnding::Missing ) && "RunRules::failDay : not failed." );
	run->setKills( run->kills() + day.kills );
	DayResult result = day;
	result.day = run->day();
	run->setLastDay( result );
	// 死んだ日のカバンは持ち帰っていないので入れない。脱出できたのは前の日まで。
	endRun( run, day.ending, run->day(), run->day() - kFirstDay, itemPoints( run->stash() ) );
}

int itemPoints( const ItemGrid& grid ) {
	int points = 0;
	for ( const ItemStack& stack : grid.stacks() ) {
		points += stack.amount * itemSpec( stack.item ).scoreValue;
		WeaponId weapon = weaponForItem( stack.item );
		if ( weapon != WEAPON_NONE ) {
			points += stack.loadedRounds * itemSpec( weaponSpec( weapon ).ammo ).scoreValue;
		}
	}
	return points;
}

} //namespace RunRules
