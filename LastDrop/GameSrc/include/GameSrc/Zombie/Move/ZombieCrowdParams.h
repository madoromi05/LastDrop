#ifndef INCLUDED_ZOMBIE_CROWD_PARAMS_H
#define INCLUDED_ZOMBIE_CROWD_PARAMS_H

// ゾンビどうしの押し合い・物への引っかかりを直す値（仕様は SummaryPlan/個別仕様/ゾンビ/ZombieCrowd_Plan.txt の 3〜5 章）。
// 起動中に F6 のエディタで動かせる。どれも画面で確かめる前の初期値。
struct ZombieCrowdParams{
	// ── 分離（近くのゾンビから離れる向きを、歩く向きに混ぜる）──
	float separationRadius = 1.5f;   // これより近いゾンビから離れる（m）。近いほど強く
	float separationStrength = 0.8f; // 行き先への向き（長さ 1）に対する強さ。1 より小さく、前へ進む向きを残す
	// 自キャラのそばで立ち止まっている間に、これより近い仲間からは横へ半歩よける（m）と、そのときの速さ（m/秒）。
	float standAsideRadius = 0.7f;
	float standAsideSpeed = 0.3f;

	// ── 詰まったら横へ（追う・探す間）──
	float sidestepAfterSeconds = 1.5f; // 向かう点へこれだけ近づけなければ「詰まった」（秒）
	float sidestepDistance = 1.5f;     // 左右へよける距離（m）
	float sidestepSeconds = 1.f;       // よける点へ向かう長さの上限（秒）
	float sidestepReach = 0.5f;        // よける点にこれより近づいたら終わり（m）
	float backstepDistance = 1.f;      // 左右どちらも通れないときに下がる距離（m）

	// ── 道探しの分散（SummaryPlan/個別仕様/最適化と計測/OptimizationC_Plan.txt 6 章）──
	// 1 ティックに道を探して（NavMesh::findPath）よい回数。超えた体は次のティックへ回る。四捨五入して使う。
	float pathSearchesPerTick = 2.f;

	// 項目ごとに v( 名前, 値, 下限, 上限, 単位 ) を呼ぶ（WeaponPoseParams と同じ約束）。時間の下限を 0 にしないのは割り算に使うため。
	template< class V > void visit( V& v ) {
		v( "separationRadius", separationRadius, 0.1f, 5.f, "m" );
		v( "separationStrength", separationStrength, 0.f, 3.f, "x" );
		v( "standAsideRadius", standAsideRadius, 0.f, 2.f, "m" );
		v( "standAsideSpeed", standAsideSpeed, 0.f, 2.f, "m/s" );
		v( "sidestepAfterSeconds", sidestepAfterSeconds, 0.1f, 10.f, "s" );
		v( "sidestepDistance", sidestepDistance, 0.1f, 5.f, "m" );
		v( "sidestepSeconds", sidestepSeconds, 0.1f, 5.f, "s" );
		v( "sidestepReach", sidestepReach, 0.05f, 2.f, "m" );
		v( "backstepDistance", backstepDistance, 0.1f, 5.f, "m" );
		v( "pathSearchesPerTick", pathSearchesPerTick, 1.f, 10.f, "times" );
	}
};

// 起動中ずっと 1 つだけの調整値（ZombieCrowd.cpp が持つ）。
ZombieCrowdParams& zombieCrowdParams();

#endif
