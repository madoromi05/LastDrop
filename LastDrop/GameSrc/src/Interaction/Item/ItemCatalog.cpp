#include "GameSrc/Interaction/Item/ItemCatalog.h"

#include "GameLib/GameLib.h"
#include <cstring>

namespace{
	const int kNoLimit = 0;  // カバンに何個まで、を決めない（空きのマス目が上限）
	const int kOneGun = 1;   // 銃はカバンに 1 種類 1 丁（R2_Plan.txt Q14）

	// ItemId の並びと同じ順。物の点（scoreValue）は 2026-11-09 にスコアと一緒に消した（SimpleRun_Plan.txt C8）。
	const ItemSpec kItemSpecs[ ITEM_NUMBER ] = {
		// ITEM_FUEL: 大きいので 2 マス（カバンの 1 行）。重ねない。赤い携行缶の色。脱出の車の給油口に入れる（FuelPort_Plan.txt）。
		{ "ガソリン", "fuel", ITEM_KIND_FUEL, nullptr, "Resource/UI/icon_fuel.tga", 2, 1, kNoLimit, 0.75f, 0.18f, 0.15f },
		// ITEM_AMMO_762: 1 マスに弾倉 1 本ぶん（30 発）。
		// 絵は弾の箱（Ammo&Prop の 7.62NATO Ammo.fbx）。落ちた形は弾薬を大きくして寝かせた物（tools/configs/ammo_drop.json）。
		{ "7.62mm 弾", "ammo_762", ITEM_KIND_AMMO, "Resource/Model/Weapon/ammo_drop.txt", "Resource/UI/icon_ammo_762.tga", 1, 30, kNoLimit, 0.45f, 0.45f, 0.40f },
		// ITEM_PISTOL: 始めのカバンに入っている銃（RaidStart）。1 マス。
		{ "拳銃", "pistol", ITEM_KIND_WEAPON, nullptr, "Resource/UI/icon_pistol.tga", 1, 1, kOneGun, 0.30f, 0.30f, 0.32f },
		// ITEM_RIFLE: 町で拾う。1 マス（ユーザーの「銃も一枠」。2 マスにするなら cellSize を 2 にするだけ）。
		{ "ライフル", "rifle", ITEM_KIND_WEAPON, nullptr, "Resource/UI/icon_rifle.tga", 1, 1, kOneGun, 0.40f, 0.30f, 0.20f },
		// ITEM_AMMO_9MM: 1 マスに弾倉 2 本ぶん（24 発）。
		// 落ちた形は 9mm の弾薬を大きくして寝かせた物（tools/configs/ammo9_drop.json）。
		{ "9mm 弾", "ammo_9mm", ITEM_KIND_AMMO, "Resource/Model/Weapon/ammo9_drop.txt", "Resource/UI/icon_ammo_9mm.tga", 1, 24, kNoLimit, 0.55f, 0.50f, 0.35f },
		// ── 段階R3 の物（R3_Plan.txt 7-1）。形と絵は tools/modelgen/loot_prop_gen.py・item_icon_gen.py ──
		// ITEM_BANDAGE: 1 マスに 3 つ。カバンの画面で使えば体力が戻る（R3_Plan.txt Q11）。
		{ "包帯", "bandage", ITEM_KIND_MEDICAL, "Resource/Model/Prop/bandage.txt", "Resource/UI/icon_bandage.tga", 1, 3, kNoLimit, 0.90f, 0.88f, 0.80f },
		// ITEM_WATCH: 1 マス。点が無くなったので、今は拾っても使い道が無い（SimpleRun_Plan.txt C8）。
		{ "腕時計", "watch", ITEM_KIND_VALUABLE, "Resource/Model/Prop/watch.txt", "Resource/UI/icon_watch.tga", 1, 1, kNoLimit, 0.80f, 0.70f, 0.35f },
		// ITEM_LAPTOP: 2 マス。腕時計と同じく、今は使い道が無い。
		{ "ノートパソコン", "laptop", ITEM_KIND_VALUABLE, "Resource/Model/Prop/laptop.txt", "Resource/UI/icon_laptop.tga", 2, 1, kNoLimit, 0.25f, 0.26f, 0.30f },
	};
}

const ItemSpec& itemSpec( ItemId id ) {
	ASSERT( id >= 0 && id < ITEM_NUMBER && "itemSpec : bad ItemId." );
	return kItemSpecs[ id ];
}

ItemId itemFromSaveName( const char* saveName ) {
	if ( !saveName ) {
		return ITEM_NONE;
	}
	for ( int i = 0; i < ITEM_NUMBER; ++i ) {
		if ( std::strcmp( kItemSpecs[ i ].saveName, saveName ) == 0 ) {
			return static_cast< ItemId >( i );
		}
	}
	return ITEM_NONE;
}
