#ifndef INCLUDED_ZOMBIE_STAGGER_PARAMS_H
#define INCLUDED_ZOMBIE_STAGGER_PARAMS_H

// 撃たれたときの反応の大きさ（SummaryPlan/個別仕様/ゾンビ/ZombieStagger_Plan.txt の 9 章。2026-11-09 ユーザー指定「よろめきをもう少し大きく」）。
// 起動中に F6 のエディタで動かせる。
//   押し下げ：よろけ（gethit）を始めたとき、弾の向きへ体を当たり判定つきで押し下げる（ZombieKnockback）。
//             gethit の根の動きは 40 フレームで 0.17m 下がって元へ戻るだけなので、それとは別に、戻らない分を足す。
//   のけぞり：1 発ごとに関節を曲げる角度（ZombieFlinchLayout の部位ごとの角度）に掛ける倍率（ZombieFlinch::push）。
struct ZombieStaggerParams{
	// 押し下げる距離（m）。gethit で下がる 0.17m と合わせて、一番下がった所で約 0.47m（今までの約 2.5 倍）。
	float knockbackDistance = 0.3f;
	// 押し下げ終えるまでの長さ（秒）。初めが速く、だんだん遅くなる（弾を受けた瞬間に一番押される）。
	float knockbackSeconds = 0.3f;
	// のけぞりの角度の倍率（胴 12°・頭 20°・腕 25°・脚 15° に掛ける）。1.5 で胴 18°・頭 30°・腕 37.5°・脚 22.5°。
	float flinchScale = 1.5f;

	// 項目ごとに v( 名前, 値, 下限, 上限, 単位 ) を呼ぶ（WeaponPoseParams と同じ約束）。時間の下限を 0 にしないのは割り算に使うため。
	template< class V > void visit( V& v ) {
		v( "knockbackDistance", knockbackDistance, 0.f, 1.5f, "m" );
		v( "knockbackSeconds", knockbackSeconds, 0.05f, 1.f, "s" );
		v( "flinchScale", flinchScale, 0.f, 3.f, "x" );
	}
};

// 起動中ずっと 1 つだけの調整値（ZombieKnockback.cpp が持つ）。
ZombieStaggerParams& zombieStaggerParams();

#endif
