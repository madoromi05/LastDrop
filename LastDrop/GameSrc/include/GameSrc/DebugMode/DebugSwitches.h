#ifndef INCLUDED_DEBUG_SWITCHES_H
#define INCLUDED_DEBUG_SWITCHES_H

// F6 のエディタの「Debug」タブ（EditorWindow）が書き、本編の確認用の道具が読む、切り替えと頼み事（Debug・Profile ビルドだけ）
// 確認用の切り替えにはキーを割り当てず、F6 のボタンからここを通して動かす
// 書く側（エディタ）と読む側（本編のシーンの係）が互いを知らずに済むよう、間に置く受け渡しの入れ物
// ゾンビを止める切り替えは世界の更新が読むので、ここではなく zombiesFrozen（ZombieDirector.h）にある
struct DebugSwitches{
	// ── 表示の切り替え（起動中ずっと残る。Retry しても消えない）。読むのは GameDebugOverlay ──
	bool showCollision = false;  // 当たり判定と、画面中央へ撃った確認用の線分
	bool showPickups = false;    // 落ちている物の箱を、壁を透かして
	bool showZombieInfo = false; // ゾンビの体力・通り道・湧く様子・引っかかりの印・自キャラの体力
	bool showNavMesh = false;    // ゾンビが歩ける床（ナビメッシュ）の多角形の辺
	bool showTownInfo = false;   // 明かり（点光源の置き場所と届く範囲）・PVS・漁れる場所の中身

	// ── 1 回だけの頼み（押したフレームにエディタが立て、読んだ係が下ろす）──
	// 状態はそれぞれの持ち主が持つ（シーンを作り直すと元に戻る物なので、ここでは持たない）
	bool toggleFlight = false;     // 自キャラを飛ばす／やめる（GameEditorBridge が読む）
	bool toggleInvincible = false; // 自キャラの無敵を切り替える（GameEditorBridge が読む）
	bool spawnZombies = false;     // 自キャラの前にゾンビを出す（GameEditorBridge が読む）
};

DebugSwitches& debugSwitches();	// 起動中ずっと 1 つ

#endif
