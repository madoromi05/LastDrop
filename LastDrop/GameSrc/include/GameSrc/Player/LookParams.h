#ifndef INCLUDED_LOOK_PARAMS_H
#define INCLUDED_LOOK_PARAMS_H

// 視点の回し方とズーム（FirstPersonControl）の調整値。設定画面の感度（GameSettings）は、この上から掛ける倍率。
// 起動中に F6 のエディタで動かせる（SummaryPlan/個別仕様/DebugTools_Profiling_Plan.txt の 5-5）。
struct LookParams{
	// マウス1画素あたりの回転（度）。横に動かすと左右、縦に動かすと上下に回る。
	float lookDegreesPerPixel = 0.15f;
	// 縦の視野角（度）。覗いている間は狭めて少しズームする（アイアンサイトなので控えめ）。
	float hipFieldOfView = 60.f;
	float aimFieldOfView = 45.f;

	// 項目ごとに v( 名前, 値, 下限, 上限, 単位 ) を呼ぶ（WeaponPoseParams と同じ約束）。
	// 視野角の下限を 0 にしないのは、感度を視野角の比で割り振るため（FirstPersonControl::readInput）。
	template< class V > void visit( V& v ) {
		v( "lookDegreesPerPixel", lookDegreesPerPixel, 0.01f, 1.f, "deg/px" );
		v( "hipFieldOfView", hipFieldOfView, 20.f, 100.f, "deg" );
		v( "aimFieldOfView", aimFieldOfView, 10.f, 100.f, "deg" );
	}
};

// 起動中ずっと 1 つだけの調整値（FirstPersonControl.cpp が持つ）。
LookParams& lookParams();

#endif
