#ifndef INCLUDED_ZOMBIE_UNCERTAINTY_PARAMS_H
#define INCLUDED_ZOMBIE_UNCERTAINTY_PARAMS_H

namespace GameLib{
	namespace Math{
		class Random;
	}
}

// ゾンビの「倒れるまでが読めない」「狙いにくい」ための値（仕様は SummaryPlan/個別仕様/ZombieUncertainty_Plan.txt）。
// 起動中に F6 のエディタで動かせる。どれも画面で確かめる前の初期値。
// 湧く体ごとの値（丈夫さ・揺れの長さ）を、この幅から選ぶ関数も持つ（SummaryPlan/個別仕様/ZombieSplit4_Plan.txt の 4 章）。
struct ZombieUncertaintyParams{
	// ── 体力の個体差（3 章）──
	// 湧くときに、この間から一様に丈夫さを選び、全体と全部の部位の最大体力に掛ける。
	// 胴だけなら 5〜9 発、頭だけなら 3〜4 発（弾1発 20）。湧いた時に選ぶので、変えたら次に湧く体から効く。
	float toughnessMin = 0.8f;
	float toughnessMax = 1.4f;

	// ── フラつき歩き（6 章）──
	// 自キャラを追っている間、これより近いと揺れ始め（m）、fullDistance より近いと一番大きく揺れる（間はなめらかに増やす）。
	float swayStartDistance = 12.f;
	float swayFullDistance = 6.f;
	// 一番大きく揺れたときに胴を倒す角度（度）。背骨から頭まで約 0.6m なので、14°で頭が左右に約 15cm 動く。
	float swayMaxDegree = 14.f;
	// 左右に 1 往復する長さ（秒）。湧くときにこの間から体ごとに選ぶ（変えたら次に湧く体から効く）。
	float swayPeriodMin = 1.2f;
	float swayPeriodMax = 1.8f;
	// 揺れに合わせて歩く向きを左右へずらす最大の角度（度）。倒れている側へ流れる。
	float weaveDegree = 8.f;

	// 項目ごとに v( 名前, 値, 下限, 上限, 単位 ) を呼ぶ（WeaponPoseParams と同じ約束）。丈夫さの下限を 0 にしないのは体力が 0 で始まるため。
	// 往復の長さの下限を 0 にしないのは割り算に使うため。
	template< class V > void visit( V& v ) {
		v( "toughnessMin", toughnessMin, 0.1f, 3.f, "x" );
		v( "toughnessMax", toughnessMax, 0.1f, 3.f, "x" );
		v( "swayStartDistance", swayStartDistance, 0.f, 40.f, "m" );
		v( "swayFullDistance", swayFullDistance, 0.f, 40.f, "m" );
		v( "swayMaxDegree", swayMaxDegree, 0.f, 45.f, "deg" );
		v( "swayPeriodMin", swayPeriodMin, 0.1f, 5.f, "s" );
		v( "swayPeriodMax", swayPeriodMax, 0.1f, 5.f, "s" );
		v( "weaveDegree", weaveDegree, 0.f, 30.f, "deg" );
	}

	// 湧く体の丈夫さ・揺れの 1 往復の長さ（秒）を、それぞれの幅から一様に選ぶ。
	// F6 で幅を逆にしても（max <= min）min を使い、乱数を引かない（引く回数が変わると、うろつく先などがずれるため）。
	// random: 体ごとの乱数。nullptr 不可。
	float pickToughness( GameLib::Math::Random* random ) const;
	float pickSwayPeriod( GameLib::Math::Random* random ) const;
};

// 起動中ずっと 1 つだけの調整値（ZombieUncertaintyParams.cpp が持つ）。
ZombieUncertaintyParams& zombieUncertaintyParams();

#endif
