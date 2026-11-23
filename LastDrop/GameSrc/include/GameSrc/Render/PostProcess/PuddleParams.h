#ifndef INCLUDED_PUDDLE_PARAMS_H
#define INCLUDED_PUDDLE_PARAMS_H

// 雨上がりの水たまり（道路の材質だけ。puddle="1"）の調整値
// 模様は Resource/puddle_mask（tools/texgen/puddle_gen.py）。模様の値は「それより暗い画素の割合」に並べ直してあるので、
// coverage がそのまま水たまりの面積の割合になる
// 起動中に F6 のエディタ（Tuning）で動かせる。決まった値は Copy as C++ でここの初期値へ戻す（コードの値が正）
// どれも見た目を確かめる前の初期値。F6 → Render の G-Buffer を Roughness にすると、水たまりが黒い形で見える
struct PuddleParams{
	float coverage = 0.2f;			// 水たまりの面積の割合（0 で無し）。0.35 だと水たまりどうしがつながって池のようになる
	float wetBand = 0.1f;			// 水たまりの周りの濡れた所の幅（模様の値で。大きいほど濡れた縁が広い）
	float puddleRoughness = 0.05f;	// 水たまりの粗さ（水面はほぼ鏡）
	float wetRoughness = 0.4f;		// 濡れた路面の粗さ（元の粗さより小さいときだけ下げる）
	float wetDarkening = 0.6f;		// 濡れた所・水たまりの色に掛ける数（濡れると暗く見える。Lagarde「Physically based wet surfaces」）
	float tileSize = 16.f;			// 模様 1 枚を貼る大きさ（m。ワールドの x・z で繰り返す）

	// 項目ごとに v( 名前, 値, 下限, 上限, 単位 ) を呼ぶ（AimSwayParams と同じ約束）
	template< class V > void visit( V& v ) {
		v( "coverage", coverage, 0.f, 1.f, "" );
		v( "wetBand", wetBand, 0.f, 0.5f, "" );
		v( "puddleRoughness", puddleRoughness, 0.f, 1.f, "" );
		v( "wetRoughness", wetRoughness, 0.f, 1.f, "" );
		v( "wetDarkening", wetDarkening, 0.f, 1.f, "x" );
		v( "tileSize", tileSize, 2.f, 64.f, "m" );
	}
};

PuddleParams& puddleParams();	// 起動中ずっと 1 つだけの調整値（GBuffer.cpp が持つ）
// 水たまりを書くか（F6 → Render の Puddles。見比べと計測用）。false の間は面積 0 として書く（道路は ORM の粗さのまま）
// 起動中ずっと 1 つ（GBuffer.cpp が持つ。初期値 true。Release では true のまま）
bool& puddlesEnabled();

#endif
