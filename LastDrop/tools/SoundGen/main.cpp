// 音の生成ツール。SoundCatalog の音（録音素材 recorded を除く）を合成して、ゲームが読む WAV を書き出す
//
// 使い方（ゲームのプロジェクトのフォルダを渡す。省略すると今のフォルダ）:
//   SoundGen.exe <LastDrop のフォルダ>
// 書き出し先は <フォルダ>/Resource/Sound/<名前>_<揺らぎ番号>.wav（名前は SoundCatalog.cpp）
//
// 種は「音の番号と揺らぎ番号」から決めるので、何度実行しても同じ音になる
// レシピ（*Synth.cpp）を変えたときだけ音が変わる
#include "Generators.h"
#include "WaveWriter.h"

#include <cstdio>
#include <filesystem>

namespace{
	// 音ごとに種の範囲を離す（揺らぎ番号を足しても隣の音の種と重ならないように）
	const unsigned kSeedStride = 1000;
	const unsigned kSeedBase = 1;	// 種 0 を避ける（xorshift は 0 から進まない）

	Synth::Buffer generate( SoundId id, Synth::Random* random ) {
		switch ( id ) {
		case SOUND_FOOTSTEP_CONCRETE:
		case SOUND_FOOTSTEP_METAL:
		case SOUND_FOOTSTEP_WOOD:
		case SOUND_FOOTSTEP_GRASS:
		case SOUND_FOOTSTEP_GRAVEL:
		case SOUND_FOOTSTEP_WATER:
		case SOUND_FOOTSTEP_GLASS:	return makeFootstep( id, random );
		case SOUND_CLOTH_RUSTLE:	return makeClothRustle( random );
		case SOUND_GEAR_RATTLE:		break; // 録音素材だけを使う（合成のレシピは無い。目録で recorded なので呼ばれない）
		case SOUND_WEAPON_DRAW:		return makeWeaponDraw( random );
		case SOUND_WEAPON_HOLSTER:	return makeWeaponHolster( random );
		case SOUND_AIM_IN:			return makeAimIn( random );
		case SOUND_AIM_OUT:			return makeAimOut( random );
		case SOUND_GUNSHOT:			break; // 録音素材だけを使う（SOUND_GEAR_RATTLE と同じ）
		case SOUND_DRY_FIRE:
		case SOUND_MAG_OUT:
		case SOUND_MAG_IN:
		case SOUND_BOLT_PULL:		break; // 録音素材だけを使う（SOUND_GEAR_RATTLE と同じ）
		case SOUND_SIREN:			return makeSiren( random );
		case SOUND_HIT_FLESH:		break; // 録音素材だけを使う（SOUND_GEAR_RATTLE と同じ）
		case SOUND_ZOMBIE_BODYFALL:	return makeZombieBodyFall( random );
		case SOUND_IMPACT_CONCRETE:
		case SOUND_IMPACT_DIRT:
		case SOUND_IMPACT_METAL:	return makeImpact( id, random );
		case SOUND_FLASHLIGHT_CLICK:	return makeFlashlightClick( random );
		case SOUND_PISTOL_SHOT:		break; // 録音素材だけを使う（SOUND_GEAR_RATTLE と同じ）
		case SOUND_LOOT_SEARCH_METAL:
		case SOUND_LOOT_SEARCH_WOOD:
		case SOUND_LOOT_SEARCH_CLOTH:	return makeLootSearch( id, random );
		case SOUND_CAR_ENGINE_START:	return makeEngineStart( random );
		case SOUND_CAR_ENGINE_IDLE:	return makeEngineIdle( random );
		case SOUND_REFUEL_POUR:		return makeRefuelPour( random );
		case SOUND_NUM:				break;
		}
		return Synth::Buffer(); // 目録に足したのにレシピが無い（main で知らせる）
	}
}

int main( int argc, char** argv ) {
	std::filesystem::path root = ( argc > 1 ) ? std::filesystem::path( argv[ 1 ] ) : std::filesystem::current_path();
	int failures = 0;
	for ( int i = 0; i < SOUND_NUM; ++i ) {
		SoundId id = static_cast< SoundId >( i );
		if ( soundEntry( id ).recorded ) {
			continue; // 録音素材は書き出さない（上書きしない）
		}
		for ( int variation = 0; variation < soundEntry( id ).variationCount; ++variation ) {
			Synth::Random random( kSeedBase + i * kSeedStride + variation );
			Synth::Buffer buffer = generate( id, &random );
			std::filesystem::path path = root / soundFilePath( id, variation );
			if ( buffer.empty() ) {
				std::printf( "no recipe: %s\n", path.string().c_str() );
				++failures;
				continue;
			}
			std::filesystem::create_directories( path.parent_path() );
			if ( !writeWave( path.string(), buffer ) ) {
				std::printf( "write failed: %s\n", path.string().c_str() );
				++failures;
				continue;
			}
			std::printf( "%s (%.2fs)\n", path.string().c_str(), static_cast< float >( buffer.size() ) / Synth::kSampleRate );
		}
	}
	return ( failures == 0 ) ? 0 : 1;
}
