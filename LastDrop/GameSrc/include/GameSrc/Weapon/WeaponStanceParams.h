#ifndef INCLUDED_WEAPON_STANCE_PARAMS_H
#define INCLUDED_WEAPON_STANCE_PARAMS_H

// 覗く操作（WeaponStance）の調整値。単位は秒で持ち、ティックへの換算は WeaponStance が使うときに行う
// 起動中に F6 のエディタで動かせる
struct WeaponStanceParams{
	float tapSeconds = 0.2f;	// これより短く押して離したら「覗いたまま」にする（秒）。Tarkov の短押し判定に合わせた体感値
	float aimSeconds = 0.25f;	// 覗き始めてからサイトが目の前に来るまで（秒）。短すぎると銃が瞬間移動して見え、長いと操作が重い

	// 項目ごとに v( 名前, 値, 下限, 上限, 単位 ) を呼ぶ（WeaponPoseParams と同じ約束）
	// aimSeconds の下限を 0 にしないのは、1 ティックに進む量（1 ÷ ティック数）で割り算をするため
	template< class V > void visit( V& v ) {
		v( "tapSeconds", tapSeconds, 0.f, 1.f, "s" );
		v( "aimSeconds", aimSeconds, 0.02f, 2.f, "s" );
	}
};

WeaponStanceParams& weaponStanceParams();	// 起動中ずっと 1 つだけの調整値（WeaponStance.cpp が持つ）

#endif
