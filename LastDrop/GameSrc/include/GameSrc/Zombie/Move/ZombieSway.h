#ifndef INCLUDED_ZOMBIE_SWAY_H
#define INCLUDED_ZOMBIE_SWAY_H

#include "GameSrc/Render/SkinnedMesh/BonePoseModifier.h"
#include <vector>

class ZombieFlinchLayout;

// ゾンビ1体ぶんの「フラつき」。胴を背骨で左右に倒し、頭を左右へ揺らす（足は地面に付けたまま）
// 頭を狙いにくくするのが目的なので、見た目だけでなく部位の当たり判定も一緒に動かす
// （持ち主が SkinnedMeshInstance::setBodyPoseModifier に渡す。SummaryPlan/個別仕様/ゾンビ/ZombieUncertainty_Plan.txt の 6 章）
// 揺れは 2 つの波を足す。ふだんの左右の揺れに、ゆっくりの波が重なったときだけ大きく傾く（よろめき）
// いつ・どれだけ揺らすか（強さ）は知らない（Zombie が自キャラとの距離から決める）。急に揺れ出さないよう、強さは少しずつ寄せる
// 胴を曲げる関節は、のけぞりと同じ設計図（ZombieFlinchLayout の胴のまとまり）を借りる
class ZombieSway : public BonePoseModifier{
public:
	// layout: 胴を曲げる設計図。所有権は受け取らない。nullptr 不可。このフラつきより長く生きていること（ZombieModel が持つ）
	// periodSeconds: 左右に 1 往復する長さ（秒。体ごとに違う値にして、群れが揃って揺れないようにする）。0 より大きいこと
	// startPhase: 揺れを始める位相（0 以上 1 未満。体ごとに違う値）
	ZombieSway( const ZombieFlinchLayout* layout, float periodSeconds, float startPhase );

	// 揺らす強さを targetStrength（0〜1）へ少しずつ寄せ、揺れを 1 ティック進める
	// maxDegree: 強さ 1 で波が一番大きいときに、胴を倒す角度（度）
	void update( float targetStrength, float maxDegree );
	// 今の傾き（-1〜1。強さ込み）。正は骨格の根の空間の +X（体の向きを正に回す側）へ倒れている
	// 歩く向きを揺れに合わせてずらすのに使う
	float lean() const;
	float weaveDegree( float maxDegree ) const;										// 今の傾きの分だけ、歩く向きをずらす角度（度。maxDegree × lean。倒れている側へ流れる）
	// distance（m）に居る相手を追うときに揺らす強さ（0〜1）。startDistance より遠いと 0、fullDistance より近いと 1、
	// 間はなめらかに増やす（遠くから頭を狙う楽しさは残す）。幅が 0 以下（F6 で逆にした）なら、fullDistance を境に 0 か 1
	// どの状態で揺らすか（追っている・よろけていない等）は呼ぶ側が決める
	static float strengthAt( float distance, float startDistance, float fullDistance );
	virtual void apply( std::vector< GameLib::Math::Matrix34 >* pose ) override;	// 今の傾きの分だけ、胴を左右に倒す（BonePoseModifier）

private:
	const ZombieFlinchLayout* mLayout; // 借りているだけ
	int mGroup;                        // 曲げるまとまり（胴）の番号
	float mPhaseStep;                  // ふだんの揺れの位相を 1 ティックで進める量
	float mPhase;                      // ふだんの揺れの位相（0 以上 1 未満）
	float mLurchPhase;                 // ゆっくりの波（よろめき）の位相（0 以上 1 未満）
	float mStrength;                   // 今の強さ（0〜1）
	float mLean;                       // 今の傾き（-1〜1。強さ込み）
	float mDegree;                     // 今、胴を倒す角度（度。符号付き）
};

#endif
