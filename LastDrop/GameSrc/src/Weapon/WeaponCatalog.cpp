#include "GameSrc/Weapon/WeaponCatalog.h"

#include "GameLib/GameLib.h"

using GameLib::Math::Vector3;

namespace{
	// ── 手の骨から見た持ち方（右手の握り）──
	// ライフル：握りの中心を置く、手の骨から見た位置（cm。骨格の単位）と向き。構え（rifle aiming idle の1フレーム目）で
	// 右手が AKM の握りを持ち、AKM が両手の間に来る値を Blender で求めた
	const Vector3 kRifleGripInHand( 0.354f, 0.38f, -9.102f );
	const float kRifleRotationY = -156.062f;
	const float kRifleRotationX = 2.115f;
	const float kRifleRotationZ = 89.224f;
	// 拳銃：構え（pistol_hold。tools/modelgen/pistol_reload_gen.py が Shooting.fbx の 1 フレーム目を骨ごとの向きで Ch15 に写した物）で、
	// 右手の中指〜小指の関節と手のひらの真ん中（握りこぶしの中心）へ拳銃の握りの中心を置き、銃口が真っすぐ前・水平、上が上になる値を
	// Blender で求めた。tools/configs/pistol_reload.json の grip と同じ値にしておく
	const Vector3 kPistolGripInHand( -0.706f, 1.342f, -10.76f );
	const float kPistolRotationY = -2.956f;
	const float kPistolRotationX = -157.73f;
	const float kPistolRotationZ = -84.344f;

	// ── リロード（アニメの時刻は秒）──
	// ライフル：Basic Shooter Pack の reloading（3.3 秒）の頭から 2.5 秒（左手がハンドガードへ戻った所）を 2 秒で流す（1.25 倍）
	// 合図は Blender で左手の動きを見て決めた（弾倉をつかんで下へ抜く・下から上がりきる・右手へ一番近づく＝右の槓桿）
	// 空でも残っていても同じ（いつも槓桿を引く）
	const WeaponReloadSpec kRifleReload = { "rifle_reload", 2.5f, 2.f, 0.45f, 1.6f, 2.1f };
	// 拳銃：tools/modelgen/pistol_reload_gen.py で作った（tools/configs/pistol_reload.json）。アニメの長さのまま流す
	// 残っているとき 1.2 秒（スライドは引かない）、空のとき 1.5 秒（左手でスライドを後ろへ引く）。どちらも覗いていない間の構え（pistol_ready）から始めて、そこで終わる
	const WeaponReloadSpec kPistolReload = { "pistol_reload", 1.2f, 1.2f, 0.35f, 1.02f, kNoBoltCue };
	const WeaponReloadSpec kPistolEmptyReload = { "pistol_reload_empty", 1.5f, 1.5f, 0.35f, 1.02f, 1.30f };

	// WeaponId の並びと同じ順
	const WeaponSpec kWeaponSpecs[ WEAPON_NUMBER ] = {
		// WEAPON_PISTOL：手に持つ形は G2-Combat.fbx を 1.8 倍（長さ 36cm。実物 16cm の 2.25 倍）。実物の大きさだと、覗いたときに構えた両手に
		// 隠れてサイトが見えず、1.5 倍でも手のかたまりの方が大きく見えたので大きくした。1.6 倍では覗いたとき拳銃と左手の親指の間に隙間が見えたので
		// 1.8 倍にした（一人称の手元の銃は大きめに見せる）
		// 地面に落ちた形（pistol_ground.txt）は 1.2 倍のまま（道に置くと 2 倍は大きすぎる）。弾倉 12 発・セミオート
		// 威力は 7.62 の 6 割・反動は小さく。腕は Shooting.fbx の構え
		// 銃声の素材は 9mm Glock の録音で、ライフルの素材より約 3 倍大きいので小さく鳴らす
		{
			"Pistol", ITEM_PISTOL, ITEM_AMMO_9MM,
			"Resource/Model/Weapon/pistol.txt", "Resource/Model/Weapon/pistol_mag.txt", "Resource/Model/Weapon/pistol_ground.txt",
			12, 400.f, false, 12.f, 0.5f,
			"pistol_hold", "pistol_ready", "rifle_idle", "pistol_hold", kPistolReload, kPistolEmptyReload,
			SOUND_PISTOL_SHOT, 0.3f,
			{
				// 握り：引き金の囲いより後ろ・下の部分の箱の真ん中から上へ（1.6 倍のときに 2.5cm 上げた点を 1.8 倍に伸ばした所）、左へ 0.8cm
				// 握りこぶしの中心に置く点を上げると、拳銃が手の中で低くなり、覗いたとき手が上がって見える
				// x を左（-）へずらすと、拳銃が手の中で左へ寄り、覗いたとき左手の親指が拳銃の左側に付く（
				// Blender でゲームと同じ覗いた絵を描いて量を決めた）
				Vector3( -0.008f, -0.0268f, -0.122f ), kPistolGripInHand, kPistolRotationY, kPistolRotationX, kPistolRotationZ,
				// 覗く目：スライドの上端（y = 0.1305）のすぐ上。z は 1.6 倍のときのまま（後ろのサイト z = -0.124 から 39cm 後ろ）
				// 目を離さないので、大きくした分だけ拳銃が大きく見える
				Vector3( 0.f, 0.1339f, -0.51f ),
				Vector3( 0.f, 0.1069f, 0.169f ),	// 銃口：銃身の一番前の頂点での銃身の中心（1.6 倍のときの値の 1.125 倍）
			},
		},
		// WEAPON_RIFLE：AKM。弾倉 30 発・600 発/分のフルオート（実銃の値）
		{
			"AK47", ITEM_RIFLE, ITEM_AMMO_762,
			"Resource/Model/Weapon/rifle.txt", "Resource/Model/Weapon/rifle_mag.txt", "Resource/Model/Weapon/rifle_ground.txt",
			30, 600.f, true, 20.f, 1.f,
			nullptr, nullptr, nullptr, "rifle_idle", kRifleReload, kRifleReload,
			SOUND_GUNSHOT, 1.f,
			{
				Vector3( 0.f, 1.438f, -0.169f ), kRifleGripInHand, kRifleRotationY, kRifleRotationX, kRifleRotationZ,
				// AKM のサイト線（リアサイト z≈0.05 → フロントサイト z≈0.43、どちらも上端 y≈1.597）を、ほお付けの位置まで後ろへ延ばした所
				// z は、カメラの近い方の面（目の 5cm 先）が機関部（上端 y≈1.59）に掛からない所にする（掛かると銃の中が見える）
				Vector3( 0.f, 1.600f, -0.28f ),
				Vector3( 0.f, 1.541f, 0.471f ),	// rifle.txt の一番前の頂点（z = 0.471）での銃身の中心
			},
		},
	};
}

const WeaponSpec& weaponSpec( WeaponId id ) {
	ASSERT( id >= 0 && id < WEAPON_NUMBER && "weaponSpec : bad WeaponId." );
	return kWeaponSpecs[ id ];
}

WeaponId weaponForItem( ItemId item ) {
	for ( int i = 0; i < WEAPON_NUMBER; ++i ) {
		if ( kWeaponSpecs[ i ].item == item ) {
			return static_cast< WeaponId >( i );
		}
	}
	return WEAPON_NONE;
}
