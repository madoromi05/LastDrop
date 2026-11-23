#ifndef INCLUDED_ZOMBIE_CROWD_PARAMS_H
#define INCLUDED_ZOMBIE_CROWD_PARAMS_H

// ゾンビどうしの押し合い・物への引っかかり・角の曲がり方を直す値
// 起動中に F6 のエディタで動かせる。どれも画面で確かめる前の初期値
struct ZombieCrowdParams{
	// ── 分離（近くのゾンビから離れる向きを、歩く向きに混ぜる）──
	float separationRadius = 1.5f;   // これより近いゾンビから離れる（m）。近いほど強く
	float separationStrength = 0.8f; // 行き先への向き（長さ 1）に対する強さ。1 より小さく、前へ進む向きを残す
	// 自キャラのそばで立ち止まっている間に、これより近い仲間からは横へ半歩よける（m）と、そのときの速さ（m/秒）
	float standAsideRadius = 0.7f;
	float standAsideSpeed = 0.3f;
	float separationHalfLife = 0.2f; // 歩く向きに混ぜる「離れる向き」が、今の値へ半分追い付くまでの時間（秒）。群れの中で向きが小刻みに揺れないように

	// ── なめらかに曲がる ──
	float lookaheadDistance = 1.5f; // 通り道に沿ってこれだけ先の点を向く（m）。角の手前から曲がり始める。0 なら角の点そのものを向く
	float turnHalfLife = 0.1f;      // 向きの差が半分になるまでの時間（秒）。差が小さいほどゆっくり回る。速さの上限は立つ体 1 秒 180°・這う体 1 秒 60°

	// ── 詰まったら横へ（追う・探す間）──
	float sidestepAfterSeconds = 1.5f; // 向かう点へこれだけ近づけなければ「詰まった」（秒）
	float sidestepDistance = 1.5f;     // 左右へよける距離（m）
	float sidestepAngle = 45.f;        // よける向き（度。向かう点の向きから左右へ）。斜め前へよけて歩みを止めない。通れなければ真横、それも無理なら下がる
	float sidestepSeconds = 1.f;       // よける点へ向かう長さの上限（秒）
	float sidestepReach = 0.5f;        // よける点にこれより近づいたら終わり（m）
	float backstepDistance = 1.f;      // 左右どちらも通れないときに下がる距離（m）

	// ── 体の大きさとナビメッシュ（次の出撃から効く。どちらも出撃のロードで作るため）──
	float footprintPadding = 0.15f; // 移動の長方形を体より一回り大きくする幅（m。四辺それぞれ）。体の見た目どうしがめり込む前に止めるため
	float navMeshClearance = 0.05f; // ナビメッシュの床の縁を、長方形が収まる円の外へさらに離す幅（m）。通り道の上で長方形の角が壁にこすらないように

	// ── 道探しの分散 ──
	// 1 ティックに道を探して（NavMesh::findPath）よい回数。超えた体は次のティックへ回る。四捨五入して使う
	float pathSearchesPerTick = 2.f;

	// 項目ごとに v( 名前, 値, 下限, 上限, 単位 ) を呼ぶ（WeaponPoseParams と同じ約束）。時間の下限を 0 にしないのは割り算に使うため
	template< class V > void visit( V& v ) {
		v( "separationRadius", separationRadius, 0.1f, 5.f, "m" );
		v( "separationStrength", separationStrength, 0.f, 3.f, "x" );
		v( "standAsideRadius", standAsideRadius, 0.f, 2.f, "m" );
		v( "standAsideSpeed", standAsideSpeed, 0.f, 2.f, "m/s" );
		v( "separationHalfLife", separationHalfLife, 0.01f, 2.f, "s" );
		v( "lookaheadDistance", lookaheadDistance, 0.f, 5.f, "m" );
		v( "turnHalfLife", turnHalfLife, 0.01f, 1.f, "s" );
		v( "sidestepAfterSeconds", sidestepAfterSeconds, 0.1f, 10.f, "s" );
		v( "sidestepDistance", sidestepDistance, 0.1f, 5.f, "m" );
		v( "sidestepAngle", sidestepAngle, 0.f, 90.f, "deg" );
		v( "sidestepSeconds", sidestepSeconds, 0.1f, 5.f, "s" );
		v( "sidestepReach", sidestepReach, 0.05f, 2.f, "m" );
		v( "backstepDistance", backstepDistance, 0.1f, 5.f, "m" );
		v( "footprintPadding", footprintPadding, 0.f, 0.5f, "m" );
		v( "navMeshClearance", navMeshClearance, 0.f, 0.5f, "m" );
		v( "pathSearchesPerTick", pathSearchesPerTick, 1.f, 10.f, "times" );
	}
};

ZombieCrowdParams& zombieCrowdParams();	// 起動中ずっと 1 つだけの調整値（ZombieCrowd.cpp が持つ）

#endif
