#include "GameSrc/Interaction/ItemCatalog.h"

#include "GameLib/GameLib.h"

namespace{
	// ItemId の並びと同じ順。
	const ItemSpec kItemSpecs[ ITEM_NUMBER ] = {
		// ITEM_FUEL: 大きいので 2 マス（カバンの 1 行）。重ねない。赤い携行缶の色。
		{ "Fuel", ITEM_KIND_FUEL, nullptr, "Resource/icon_fuel.tga", 2, 1, 0, 0.75f, 0.18f, 0.15f },
		// ITEM_AMMO_762: 1 マスに弾倉 1 本ぶん（30 発）。始めは持たない（始めの銃は拳銃。Pistol_Plan.txt Q1）。
		// 絵は弾の箱（Ammo&Prop の 7.62NATO Ammo.fbx）。落ちた形は弾薬を大きくして寝かせた物（tools/configs/ammo_drop.json）。
		{ "7.62 rounds", ITEM_KIND_AMMO, "Resource/ammo_drop.txt", "Resource/icon_ammo_762.tga", 1, 30, 0, 0.45f, 0.45f, 0.40f },
		// ITEM_PISTOL: 始めから持つ（出撃のたびにタダでもらえる銃）。1 マス。
		{ "Pistol", ITEM_KIND_WEAPON, nullptr, "Resource/icon_pistol.tga", 1, 1, 1, 0.30f, 0.30f, 0.32f },
		// ITEM_RIFLE: 町で拾う。1 マス（ユーザーの「銃も一枠」。2 マスにするなら cellSize を 2 にするだけ）。
		{ "Rifle", ITEM_KIND_WEAPON, nullptr, "Resource/icon_rifle.tga", 1, 1, 0, 0.40f, 0.30f, 0.20f },
		// ITEM_AMMO_9MM: 1 マスに弾倉 2 本ぶん（24 発）。始めは 1 マスぶん（弾倉の 12 発とは別）。
		// 落ちた形は 9mm の弾薬を大きくして寝かせた物（tools/configs/ammo9_drop.json）。
		{ "9mm rounds", ITEM_KIND_AMMO, "Resource/ammo9_drop.txt", "Resource/icon_ammo_9mm.tga", 1, 24, 24, 0.55f, 0.50f, 0.35f },
	};
}

const ItemSpec& itemSpec( ItemId id ) {
	ASSERT( id >= 0 && id < ITEM_NUMBER && "itemSpec : bad ItemId." );
	return kItemSpecs[ id ];
}
