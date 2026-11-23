#ifndef INCLUDED_AIM_SWAY_PARAMS_H
#define INCLUDED_AIM_SWAY_PARAMS_H

// 照準のぶれ（AimSway）の調整値
// 角度は「覗いているとき」の大きさ（腰だめは hipScale 倍）。起動中に F6 のエディタで動かせる
// どれも見た目を確かめる前の初期値。小さめから始めて、画面で見ながら上げる
struct AimSwayParams{
	// ── a. 息（止まっていても出る、ゆっくり小さな 8 の字）──
	// 左右と上下を周期の違う正弦波で動かす。周期の比が 3:2 に近いので、8 の字に近い形を描く
	float breathYaw = 0.15f;        // 左右の振れ幅（度）
	float breathPitch = 0.10f;      // 上下の振れ幅（度）
	float breathYawPeriod = 4.0f;   // 左右の 1 往復（秒）
	float breathPitchPeriod = 2.7f; // 上下の 1 往復（秒）

	// ── b. 歩き（歩くと大きく、止まってもすぐには収まらない）──
	// 形は歩きのアニメの位相に合わせる（上下は 1 歩ごと、左右は 2 歩で 1 往復）
	float walkYaw = 0.6f;           // 左右の振れ幅（度）
	float walkPitch = 0.8f;         // 上下の振れ幅（度）
	float walkRiseSeconds = 0.3f;   // 歩き出してから強さが 1 になるまで（秒）
	float walkFallSeconds = 0.6f;   // 止まってから強さが 0 になるまで（秒）。タルコフも止まった後に揺れが残る
	// 位相はぶれの側で進め、アニメの位相へは少しずつ寄せる（アニメは止まる⇔歩くで頭から始まり、位相が跳ぶため）
	float walkCycleSeconds = 1.37f; // 歩きの 1 周（秒。rifle_walk と同じ）。速さの割合を掛けて位相を進める
	float walkSyncHalfLife = 0.3f;  // アニメの位相とのずれが半分になるまで（秒）

	// ── c. 振り向きの遅れ（慣性。マウスで回すと、銃が少し遅れて付いてくる）──
	float lagRate = 0.25f;          // 回した角度の何倍だけ、銃を逆へ残すか
	float lagLimit = 2.0f;          // 残すずれの上限（度。大きく振り向いても銃が画面の外へ行かないように）
	float lagHalfLife = 0.12f;      // ずれが半分に戻るまで（秒。はね返らない戻り方）

	// ── 全体に掛ける倍率 ──
	float hipScale = 2.0f;          // 腰だめ（覗き具合 0）のとき。覗き具合に合わせて 1 倍との間を取る
	float dangerScale = 1.8f;       // 体力が Danger のとき

	// ── 銃を回す中心 ──
	// 目のこれだけ後ろ（肩の付け根あたり。銃床を当てている所）を中心に、両腕と銃を回す（m）
	// 目を中心にするとサイトの手前が止まって先だけが動く。少し後ろを中心にすると、構えた銃が肩ごと揺れて見える
	float pivotBehindEye = 0.15f;

	// 項目ごとに v( 名前, 値, 下限, 上限, 単位 ) を呼ぶ（WeaponPoseParams と同じ約束）
	// 周期・時間の下限を 0 にしないのは、割り算に使うため
	template< class V > void visit( V& v ) {
		v( "breathYaw", breathYaw, 0.f, 3.f, "deg" );
		v( "breathPitch", breathPitch, 0.f, 3.f, "deg" );
		v( "breathYawPeriod", breathYawPeriod, 0.2f, 20.f, "s" );
		v( "breathPitchPeriod", breathPitchPeriod, 0.2f, 20.f, "s" );
		v( "walkYaw", walkYaw, 0.f, 5.f, "deg" );
		v( "walkPitch", walkPitch, 0.f, 5.f, "deg" );
		v( "walkRiseSeconds", walkRiseSeconds, 0.02f, 3.f, "s" );
		v( "walkFallSeconds", walkFallSeconds, 0.02f, 3.f, "s" );
		v( "walkCycleSeconds", walkCycleSeconds, 0.2f, 5.f, "s" );
		v( "walkSyncHalfLife", walkSyncHalfLife, 0.02f, 3.f, "s" );
		v( "lagRate", lagRate, 0.f, 1.f, "x" );
		v( "lagLimit", lagLimit, 0.f, 10.f, "deg" );
		v( "lagHalfLife", lagHalfLife, 0.01f, 1.f, "s" );
		v( "hipScale", hipScale, 0.f, 5.f, "x" );
		v( "dangerScale", dangerScale, 0.f, 5.f, "x" );
		v( "pivotBehindEye", pivotBehindEye, 0.f, 0.5f, "m" );
	}
};

AimSwayParams& aimSwayParams();	// 起動中ずっと 1 つだけの調整値（AimSway.cpp が持つ）

#endif
