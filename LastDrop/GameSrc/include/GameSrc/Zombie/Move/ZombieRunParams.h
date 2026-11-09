#ifndef INCLUDED_ZOMBIE_RUN_PARAMS_H
#define INCLUDED_ZOMBIE_RUN_PARAMS_H

namespace GameLib{
	namespace Math{
		class Random;
	}
}

// 走るゾンビの値（仕様は SummaryPlan/個別仕様/出撃と帰還/R4_Plan.txt）。起動中に F6 のエディタで動かせる。
// 走るゾンビが「速すぎて怖い／遅すぎて怖くない」は遊んで決めるので、速さ・割合はここに集める。
struct ZombieRunParams{
	// 湧いた体が「走る体」になる確率（0〜1）。湧いたときに 1 回だけ引く（変えたら次に湧く体から効く）。
	// 残り時間で割合は上げない。
	float runnerRatio = 0.2f;
	// 走りのアニメ（zombie@run）を流す倍率。等倍で約 2.0m/秒なので、1.45 倍で約 3.0m/秒（自キャラの走り 3.1〜3.2m/秒より少しだけ遅い）。
	// 進む長さも足の動きの表 × この倍率なので、足は地面を滑らない。すぐ効く。
	float runPlaybackRate = 1.45f;
	// 脚の部位（左右の太もも・すね）のどれかの体力の残りがこの割合以下になったら、その体はもう走らない（歩きに落ちる）。
	// 0.5 で、ライフルなら太もも・すねのどちらも 2 発（0 になって切れるのは 4 発）。
	float legSlowRatio = 0.5f;

	// 項目ごとに v( 名前, 値, 下限, 上限, 単位 ) を呼ぶ（WeaponPoseParams と同じ約束）。倍率の下限を 0 にしないのはアニメの速さに使うため。
	template< class V > void visit( V& v ) {
		v( "runnerRatio", runnerRatio, 0.f, 1.f, "" );
		v( "runPlaybackRate", runPlaybackRate, 0.5f, 3.f, "x" );
		v( "legSlowRatio", legSlowRatio, 0.f, 1.f, "" );
	}

	// 湧く体が走る体かを決める。random: 体ごとの乱数。nullptr 不可。割合が 0 や 1 でも乱数は 1 回引く
	// （F6 で割合を動かしても、後に引く値の並びが変わらないように）。
	bool pickRunner( GameLib::Math::Random* random ) const;
};

// 起動中ずっと 1 つだけの調整値（ZombieRunParams.cpp が持つ）。
ZombieRunParams& zombieRunParams();

#endif
