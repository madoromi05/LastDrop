#ifndef INCLUDED_SHADOW_PARAMS_H
#define INCLUDED_SHADOW_PARAMS_H

// 影の調整値（懐中電灯・月の影）。仕様は SummaryPlan/個別仕様/グラフィック/Shadow_Plan.txt の 7・9 章
// 起動中に F6 のエディタ（Tuning）で動かせる。決まった値は Copy as C++ でここの初期値へ戻す（コードの値が正）
// どれも見た目を確かめる前の初期値。にきび（面に自分の影が縞で落ちる）が消える所までだけ、ずらしを上げる
// （上げすぎると、物の根元で影が離れる・壁の裏に光が回る）
struct ShadowParams{
	float normalOffsetTexels = 1.5f;	// 影を読む位置を、面の法線の向きへずらす量（影のマップ 1 画素の大きさの何倍か）。にきびの主な対策（Normal Offset）
	float lightOffsetTexels = 1.f;		// 影を読む位置を、光の方へずらす量（同じく画素の何倍か）。比べる深度を手前にするのと同じ（深度のバイアス）
	float strength = 1.f;				// 影の濃さ。1 で影の中は真っ暗（その光が全く届かない）。下げると影の中にもその光が少し残る
	float flashlightNearClip = 0.1f;	// 懐中電灯の影のマップの、近い面（m。光から）。小さすぎると遠くの深度の細かさが落ちる

	// ── 月の影（CSM 3 枚） ── 各枚の受け持ちの奥の端（m。カメラの向きに沿った距離。小さい順）
	float moonCascadeEnd0 = 8.f;
	float moonCascadeEnd1 = 25.f;
	float moonCascadeEnd2 = 80.f;	// これより先は月の影を付けない（町は端から端まで約 115m）
	float moonCascadeBlend = 0.1f;	// 各枚の受け持ちの奥の何割を、次の枚と混ぜるか（境目で影の細かさが急に変わって見えないように）
	float moonCasterMargin = 40.f;	// 枚の範囲より月の側へ、影を落とす物を拾う長さ（m）。枚の外にある高い建物の影も落ちるように
	// ── 月の影の描き直しの間引き ── 手前の枚は毎フレーム描く
	// 2 枚目・3 枚目を何フレームに 1 回描き直すか（整数に丸めて使う。1 で毎フレーム）。2・4 なら同じフレームに重ならない
	float moonCascadeInterval1 = 2.f;
	float moonCascadeInterval2 = 4.f;
	// 前に描いた時からカメラの向きがこれ以上変わった（度）・目がこれ以上動いた（m）ら、間隔を待たずに描き直す
	// （急に振り向くと、前に描いた範囲の外が映って影が欠けるため）
	float moonRedrawAngle = 10.f;
	float moonRedrawDistance = 2.f;

	// 項目ごとに v( 名前, 値, 下限, 上限, 単位 ) を呼ぶ（AimSwayParams と同じ約束）
	template< class V > void visit( V& v ) {
		v( "normalOffsetTexels", normalOffsetTexels, 0.f, 8.f, "px" );
		v( "lightOffsetTexels", lightOffsetTexels, 0.f, 8.f, "px" );
		v( "strength", strength, 0.f, 1.f, "" );
		v( "flashlightNearClip", flashlightNearClip, 0.02f, 1.f, "m" ); // 0 は不可（透視の行列が作れない）
		// 端は小さい順にすること（GameScene が前の端より奥へ揃えるので、逆にしても止まりはしない）
		v( "moonCascadeEnd0", moonCascadeEnd0, 1.f, 40.f, "m" );
		v( "moonCascadeEnd1", moonCascadeEnd1, 2.f, 80.f, "m" );
		v( "moonCascadeEnd2", moonCascadeEnd2, 4.f, 200.f, "m" );
		v( "moonCascadeBlend", moonCascadeBlend, 0.f, 0.5f, "" );
		v( "moonCasterMargin", moonCasterMargin, 0.f, 100.f, "m" );
		v( "moonCascadeInterval1", moonCascadeInterval1, 1.f, 8.f, "frame" );
		v( "moonCascadeInterval2", moonCascadeInterval2, 1.f, 8.f, "frame" );
		v( "moonRedrawAngle", moonRedrawAngle, 0.f, 90.f, "deg" );
		v( "moonRedrawDistance", moonRedrawDistance, 0.f, 20.f, "m" );
	}
};

ShadowParams& shadowParams();	// 起動中ずっと 1 つだけの調整値（ShadowMap.cpp が持つ）
// 影を描くか（F6 → Render の Shadows。見比べと計測用）。false の間は影のパスごと描かず、ライトも影を読まない
// 起動中ずっと 1 つ（ShadowMap.cpp が持つ。初期値 true。Release では true のまま）
bool& shadowsEnabled();

#endif
