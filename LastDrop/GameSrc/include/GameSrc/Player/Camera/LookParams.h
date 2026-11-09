#ifndef INCLUDED_LOOK_PARAMS_H
#define INCLUDED_LOOK_PARAMS_H

// 視点の回し方とズーム（FirstPersonControl）の調整値。設定画面の感度（GameSettings）は、この上から掛ける倍率。
// 起動中に F6 のエディタで動かせる（SummaryPlan/個別仕様/最適化と計測/DebugTools_Profiling_Plan.txt の 5-5）。
struct LookParams{
	// マウス1画素あたりの回転（度）。横に動かすと左右、縦に動かすと上下に回る。
	float lookDegreesPerPixel = 0.15f;
	// 縦の視野角（度）。覗いている間は狭めて少しズームする（アイアンサイトなので控えめ）。
	float hipFieldOfView = 60.f;
	float aimFieldOfView = 45.f;
	// パッドの右スティックを倒しきったときの回る速さ（度/秒）。縦は横より遅くする（上下は狙いを細かく合わせるため）。
	float stickYawDegreesPerSecond = 180.f;
	float stickPitchDegreesPerSecond = 120.f;
	// スティックの傾きを速さにするときの累乗（1 でそのまま。大きいほど小さく倒したときにゆっくり回り、細かく狙える）。
	float stickLookExponent = 2.f;

	// 項目ごとに v( 名前, 値, 下限, 上限, 単位 ) を呼ぶ（WeaponPoseParams と同じ約束）。
	// 視野角の下限を 0 にしないのは、感度を視野角の比で割り振るため（FirstPersonControl::readInput）。
	template< class V > void visit( V& v ) {
		v( "lookDegreesPerPixel", lookDegreesPerPixel, 0.01f, 1.f, "deg/px" );
		v( "hipFieldOfView", hipFieldOfView, 20.f, 100.f, "deg" );
		v( "aimFieldOfView", aimFieldOfView, 10.f, 100.f, "deg" );
		v( "stickYawDegreesPerSecond", stickYawDegreesPerSecond, 30.f, 600.f, "deg/s" );
		v( "stickPitchDegreesPerSecond", stickPitchDegreesPerSecond, 30.f, 600.f, "deg/s" );
		v( "stickLookExponent", stickLookExponent, 1.f, 4.f, "" );
	}
};

// 起動中ずっと 1 つだけの調整値（FirstPersonControl.cpp が持つ）。
LookParams& lookParams();

#endif
