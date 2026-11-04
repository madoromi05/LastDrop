#include "GameSrc/Interaction/ItemCatalog.h"

#include "GameLib/GameLib.h"
#include <cstring>

namespace{
	const int kNoLimit = 0;  // カバンに何個まで、を決めない（空きのマス目が上限）
	const int kOneGun = 1;   // 銃はカバンに 1 種類 1 丁（R2_Plan.txt Q14）

	// ItemId の並びと同じ順。点（scoreValue）は R2_Plan.txt Q4 の初期値（遊んで決める）。
	const ItemSpec kItemSpecs[ ITEM_NUMBER ] = {
		// ITEM_FUEL: 大きいので 2 マス（カバンの 1 行）。重ねない。赤い携行缶の色。持ち帰ると点が高い（2026-11-05 から車へは入れない）。
		{ "Fuel", "fuel", ITEM_KIND_FUEL, nullptr, "Resource/icon_fuel.tga", 2, 1, kNoLimit, 300, 0.75f, 0.18f, 0.15f },
		// ITEM_AMMO_762: 1 マスに弾倉 1 本ぶん（30 発）。
		// 絵は弾の箱（Ammo&Prop の 7.62NATO Ammo.fbx）。落ちた形は弾薬を大きくして寝かせた物（tools/configs/ammo_drop.json）。
		{ "7.62 rounds", "ammo_762", ITEM_KIND_AMMO, "Resource/ammo_drop.txt", "Resource/icon_ammo_762.tga", 1, 30, kNoLimit, 3, 0.45f, 0.45f, 0.40f },
		// ITEM_PISTOL: 出撃のたびにタダでもらえる銃（倉庫にもカバンにも無いときだけ。R2_Plan.txt Q14）。1 マス。
		{ "Pistol", "pistol", ITEM_KIND_WEAPON, nullptr, "Resource/icon_pistol.tga", 1, 1, kOneGun, 100, 0.30f, 0.30f, 0.32f },
		// ITEM_RIFLE: 町で拾う。1 マス（ユーザーの「銃も一枠」。2 マスにするなら cellSize を 2 にするだけ）。
		{ "Rifle", "rifle", ITEM_KIND_WEAPON, nullptr, "Resource/icon_rifle.tga", 1, 1, kOneGun, 500, 0.40f, 0.30f, 0.20f },
		// ITEM_AMMO_9MM: 1 マスに弾倉 2 本ぶん（24 発）。
		// 落ちた形は 9mm の弾薬を大きくして寝かせた物（tools/configs/ammo9_drop.json）。
		{ "9mm rounds", "ammo_9mm", ITEM_KIND_AMMO, "Resource/ammo9_drop.txt", "Resource/icon_ammo_9mm.tga", 1, 24, kNoLimit, 2, 0.55f, 0.50f, 0.35f },
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
