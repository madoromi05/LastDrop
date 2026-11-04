#include "GameSrc/Raid/RunRules.h"
#include "GameSrc/Raid/RaidParams.h"
#include "GameSrc/Weapon/WeaponCatalog.h"

#include "GameLib/GameLib.h"
#include <cmath>

namespace{
	const int kFirstDay = 1;

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
		run->save();
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
}

void loadOrStart( RunState* run ) {
	ASSERT( run && "RunRules::loadOrStart : run is null." );
	if ( !run->load() ) {
		startNewRun( run );
	}
}

void deploy( RunState* run ) {
	ASSERT( run && "RunRules::deploy : run is null." );
	run->save();
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
	run->save();
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
