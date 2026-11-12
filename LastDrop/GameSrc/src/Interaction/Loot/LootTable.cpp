#include "GameSrc/Interaction/Loot/LootTable.h"
#include "GameSrc/Interaction/Loot/LootParams.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	// 出る物 1 行（重みは同じ表の中での比べ合い。足して 100 にそろえてあるのは読みやすさのためで、そろえなくても動く）
	struct LootEntry{
		ItemId item;
		int weight;
	};

	// 場所 1 種類の表。entries は kMaxEntries までで、count 行を使う
	const int kMaxEntries = 6;
	struct LootSpec{
		const char* name;      // 中を見る画面の見出し
		int minItems;          // 空でなかったときの物の数（この幅から選ぶ）
		int maxItems;
		SoundId sound;         // 開けている間の音
		int count;
		LootEntry entries[ kMaxEntries ];
	};

	const int kAmmoMinDivisor = 2;	// 弾は、1 個でその弾の 1 マスの半分〜全部（7.62mm 15〜30 発・9mm 12〜24 発）
	const int kSingleItem = 1;		// 弾以外は 1 個ずつ（包帯も 1 つ。重なる数は拾ってから）

	// LootTableId の順
	const LootSpec kSpecs[ static_cast< int >( LootTableId::Count ) ] = {
		// ガソリンは表から出さない（町のガソリンはちょうど RaidParams::fuelCanCount 個。WorldBuilder が選んだ箱へ必ず入れる）
		// 車のトランクの表は改訂 2 で消した（車には物を入れない）
		{ "ゴミ箱", 1, 3, SOUND_LOOT_SEARCH_METAL, 3, {
			{ ITEM_AMMO_9MM, 43 }, { ITEM_BANDAGE, 36 }, { ITEM_AMMO_762, 21 } } },
		// ライフルは表から出さない（町にちょうど RaidParams::rifleCount 丁。WorldBuilder が選んだ木箱へ、弾倉いっぱいで必ず入れる）
		{ "木箱", 1, 3, SOUND_LOOT_SEARCH_WOOD, 3, {
			{ ITEM_AMMO_762, 44 }, { ITEM_AMMO_9MM, 31 }, { ITEM_BANDAGE, 25 } } },
	};

	// 空になる確率（LootParams。開けてみないと分からない）
	float emptyChanceOf( LootTableId id ) {
		const LootParams& params = lootParams();
		return ( id == LootTableId::Crate ) ? params.crateEmptyChance : params.dumpsterEmptyChance;
	}

	const LootSpec& specOf( LootTableId id ) {
		int index = static_cast< int >( id );
		ASSERT( index >= 0 && index < static_cast< int >( LootTableId::Count ) && "LootTable : bad LootTableId." );
		return kSpecs[ index ];
	}

	// 重みで 1 行を選ぶ
	ItemId pickItem( const LootSpec& spec, Random* random ) {
		int total = 0;
		for ( int i = 0; i < spec.count; ++i ) {
			total += spec.entries[ i ].weight;
		}
		int roll = random->getInt( total );
		for ( int i = 0; i < spec.count; ++i ) {
			roll -= spec.entries[ i ].weight;
			if ( roll < 0 ) {
				return spec.entries[ i ].item;
			}
		}
		return spec.entries[ spec.count - 1 ].item; // 届かない（重みの合計の内で必ず選ばれる）
	}

	// 1 個ぶんの数（弾は 1 マスの半分〜全部、ほかは 1）
	int amountOf( ItemId item, Random* random ) {
		const ItemSpec& spec = itemSpec( item );
		if ( spec.kind != ITEM_KIND_AMMO ) {
			return kSingleItem;
		}
		return random->getInt( spec.stackSize / kAmmoMinDivisor, spec.stackSize + 1 );
	}
}

namespace LootTable{

const char* displayName( LootTableId id ) {
	return specOf( id ).name;
}

float searchSeconds( LootTableId id ) {
	const LootParams& params = lootParams();
	switch ( id ) {
	case LootTableId::Dumpster: return params.dumpsterSeconds;
	case LootTableId::Crate:    return params.crateSeconds;
	default:                    break;
	}
	ASSERT( false && "LootTable::searchSeconds : bad LootTableId." );
	return 0.f;
}

SoundId searchSound( LootTableId id ) {
	return specOf( id ).sound;
}

void roll( LootTableId id, Random* random, const std::vector< ItemStack >& guaranteed, ItemGrid* out ) {
	ASSERT( random && out && "LootTable::roll : an argument is null." );
	out->clearAll();
	const LootSpec& spec = specOf( id );
	// 必ず入れる物は先に入れる（後のふつうの中身でマスが埋まって入らない、が起きないように）
	// ガソリン（2 マス）とライフル（1 マス）が同じ箱に当たっても、2 × 3 に入る
	bool allPlaced = true;
	for ( const ItemStack& stack : guaranteed ) {
		allPlaced = out->placeAnywhere( stack ) && allPlaced;
	}
	ASSERT( allPlaced && "LootTable::roll : the guaranteed items do not fit in the box." );
	if ( guaranteed.empty() && random->getFloat() < emptyChanceOf( id ) ) {
		return;
	}
	int itemCount = random->getInt( spec.minItems, spec.maxItems + 1 );
	for ( int i = 0; i < itemCount; ++i ) {
		ItemStack stack = { pickItem( spec, random ), 0 };
		stack.amount = amountOf( stack.item, random );
		out->placeAnywhere( stack ); // 入りきらなければ出さない（2 × 3 に収まらない分）
	}
}

} //namespace LootTable
