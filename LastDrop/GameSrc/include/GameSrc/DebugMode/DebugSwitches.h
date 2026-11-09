#ifndef INCLUDED_DEBUG_SWITCHES_H
#define INCLUDED_DEBUG_SWITCHES_H

// F6 のエディタの「Debug」タブ（EditorWindow）が書き、本編の確認用の道具が読む、切り替えと頼み事（Debug・Profile ビルドだけ）。
// 2026-11-09 に確認用の F キー（F1〜F5・F7・F9）をやめ、F6 のボタンからここを通して動かす形にした（ユーザー指定）。
// 書く側（エディタ）と読む側（本編のシーンの係）が互いを知らずに済むよう、間に置く受け渡しの入れ物。
// ゾンビを止める切り替えは世界の更新が読むので、ここではなく zombiesFrozen（ZombieDirector.h）にある。
struct DebugSwitches{
	// ── 表示の切り替え（起動中ずっと残る。Retry しても消えない）。読むのは GameDebugOverlay ──
	bool showCollision = false;  // 当たり判定と、画面中央へ撃った確認用の線分（旧 F1）
	bool showPickups = false;    // 落ちている物の箱を、壁を透かして（旧 F2）
	bool showZombieInfo = false; // ゾンビの体力・通り道・湧く様子・PVS・ライト・自キャラの体力・漁れる場所の中身（旧 F3）

	// ── 1 回だけの頼み（押したフレームにエディタが立て、読んだ係が下ろす）──
	// 状態はそれぞれの持ち主が持つ（シーンを作り直すと元に戻る物なので、ここでは持たない）。
	bool toggleFreeCamera = false; // 観察用カメラを使う／やめる（旧 F4。FreeCamera が読む）
	bool toggleInvincible = false; // 自キャラの無敵を切り替える（旧 F5。GameEditorBridge が読む）
	bool spawnZombies = false;     // 自キャラの前にゾンビを出す（旧 F9。GameEditorBridge が読む）
};

// 起動中ずっと 1 つ。
DebugSwitches& debugSwitches();

#endif
