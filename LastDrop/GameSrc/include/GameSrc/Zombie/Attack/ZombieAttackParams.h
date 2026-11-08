#ifndef INCLUDED_ZOMBIE_ATTACK_PARAMS_H
#define INCLUDED_ZOMBIE_ATTACK_PARAMS_H

// ゾンビがつかんでいる間（Hold）の、自キャラとの距離・向きを合わせる値（SummaryPlan/個別仕様/ZombieAttack_Plan.txt の 10 章）。
// つかむ位置がその時の間合い（0.9〜1.6m・左右 45°）のままだと、手が離れて見えたり、踏み込みで自キャラへめり込んだりするので、
// つかんだら決まった距離・正面へ寄せる。起動中に F6 のエディタで動かせる。
struct ZombieAttackParams{
	// つかんでいる間の、自キャラの足元からゾンビの腰（足元）までの水平の距離（m）。
	// atack3 のつかんだ姿勢（Blender の 56〜92 フレーム）は手が腰の 0.35m 前・頭が 0.31m 前に出るので、
	float grabDistance = 0.80f;
	// つかんでから、その距離・正面へ寄せ終えるまでの長さ（秒）。その場で飛ぶと目立つので、手を伸ばし切る動きに紛らせる。
	float grabAlignSeconds = 0.1f;

	// 項目ごとに v( 名前, 値, 下限, 上限, 単位 ) を呼ぶ（WeaponPoseParams と同じ約束）。時間の下限を 0 にしないのは割り算に使うため。
	template< class V > void visit( V& v ) {
		v( "grabDistance", grabDistance, 0.2f, 1.5f, "m" );
		v( "grabAlignSeconds", grabAlignSeconds, 0.02f, 1.f, "s" );
	}
};

// 起動中ずっと 1 つだけの調整値（ZombieAttackComponent.cpp が持つ）。
ZombieAttackParams& zombieAttackParams();

#endif
