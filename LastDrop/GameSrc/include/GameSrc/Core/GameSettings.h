#ifndef INCLUDED_GAME_SETTINGS_H
#define INCLUDED_GAME_SETTINGS_H

class AudioEngine;

// 遊ぶ人が設定画面（Options）で変える値（音量・視点の感度）。起動から終了まで 1 つだけ（GameResources が持つ）
// ・値は画面に見せる数のまま整数で持つ（音量 0〜100、感度は百分率で 100 が元の速さ）。刻み（5 ずつ）を足し引きしても
//   小数のずれがたまらない。ゲームが使う倍率への直し方は、ここの関数だけに置く
// ・保存はキーコンフィグ（InputConfig）と同じく、PseudoXml で文字列にして fstream で書く
//   （FileIO はファイルが無いと止まることがあるため）
// SummaryPlan/個別仕様/UI/OptionsMenu_Plan.txt
struct GameSettings{
	static const int kVolumeMin = 0;
	static const int kVolumeMax = 100;
	static const int kVolumeStep = 5;
	static const int kSensitivityMin = 20;  // 0.20 倍
	static const int kSensitivityMax = 300; // 3.00 倍
	static const int kSensitivityStep = 5;  // 0.05 倍ずつ

	int masterVolume;     // 全体の音量（0〜100）
	int effectVolume;     // 効果音の音量（0〜100。AudioEngine::BUS_EFFECT）
	int mouseSensitivity; // マウスで回す視点の感度（百分率。100 が FirstPersonControl の元の速さ）
	int padSensitivity;   // パッドの右スティックで回す視点の感度（百分率。100 が LookParams のスティックの速さ）

	GameSettings();	// 既定値（音量はどちらも 100、感度もどちらも 100）

	bool load();									// 保存したファイルから読む。ファイルが無い・壊れているときは false を返し、値は既定値のまま。読めた値は範囲に収める
	bool save() const;								// ファイルへ書く。書けなければ false（遊ぶのは続けられるので止めない）
	void applyAudio( AudioEngine& audio ) const;	// 音量を audio へ流す（起動時と、設定画面で変えたとき）
	float mouseSensitivityScale() const;			// マウスの感度の倍率（1 が元の速さ）
	float padSensitivityScale() const;				// パッドの感度の倍率（1 が元の速さ）
};

#endif
