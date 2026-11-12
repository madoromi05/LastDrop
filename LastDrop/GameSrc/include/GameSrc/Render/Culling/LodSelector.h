#ifndef INCLUDED_LOD_SELECTOR_H
#define INCLUDED_LOD_SELECTOR_H

// 置物 1 つの、今描く細かさの段（LOD）を覚えて選び直す係（SummaryPlan/個別仕様/最適化と計測/OptimizationC_Plan.txt 8 章・Q8）
// 画面の上の大きさ（画素）で選ぶ。境目に幅を持たせ、粗くするときと細かくするときで境目を変える（境目の上で行ったり来たりしないように）
// 段を選び直すのは目から見る絵だけ（DrawCulling が決める）。影のパスは最後に選んだ段（current）か、一番粗い段を使う
// 描くための状態だけを持ち、ゲームの進行には使わない
class LodSelector{
public:
	LodSelector();
	int select( float screenPixels );	// 画面の上の半径 screenPixels（画素）から段を選び直して返す（0 が細かい形）
	int current() const;				// 最後に選んだ段（まだ選んでいなければ 0）

private:
	int mLevel;
};

// LOD の境目の値（F6 の Lod で動かせる）
struct LodParams{
	float lod1Pixels = 200.f; // 画面の上の半径がこれより小さければ LOD1（画素）
	float lod2Pixels = 80.f;  // これより小さければ LOD2（画素）
	float hysteresis = 0.15f; // 境目の幅（境目の値に対する割合。上がる時と下がる時で ± 半分ずつずらす）

	// 項目ごとに v( 名前, 値, 下限, 上限, 単位 ) を呼ぶ（WeaponPoseParams と同じ約束）
	template< class V > void visit( V& v ) {
		v( "lod1Pixels", lod1Pixels, 1.f, 2000.f, "px" );
		v( "lod2Pixels", lod2Pixels, 1.f, 2000.f, "px" );
		v( "hysteresis", hysteresis, 0.f, 1.f, "x" );
	}
};

LodParams& lodParams();	// 起動中ずっと 1 つだけの調整値（LodSelector.cpp が持つ）
bool& lodEnabled();		// 粗い形で描くか（F6 → Render の LOD。見比べと計測用）。false の間はいつも元の形。起動中ずっと 1 つ（初期値 true）

#endif
