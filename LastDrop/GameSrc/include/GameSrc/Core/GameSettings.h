#ifndef INCLUDED_GAME_SETTINGS_H
#define INCLUDED_GAME_SETTINGS_H

class AudioEngine;

// 遊ぶ人が設定画面（Options）で変える値（音量・視点の感度・画面の明るさ）。起動から終了まで 1 つだけ（GameResources が持つ）
// ・値は画面に見せる数のまま整数で持つ（音量 0〜100、感度は百分率で 100 が元の速さ）。刻み（5 ずつ）を足し引きしても
//   小数のずれがたまらない。ゲームが使う倍率への直し方は、ここの関数だけに置く
// ・保存はキーコンフィグ（InputConfig）と同じく、PseudoXml で文字列にして fstream で書く
//   （FileIO はファイルが無いと止まることがあるため）
struct GameSettings{
	static const int kVolumeMin = 0;
	static const int kVolumeMax = 100;
	static const int kVolumeStep = 5;
	static const int kDefaultMusicVolume = 70;	// 曲は UI の音や操作の音より下がっている方が聞きやすいので、始めは少し下げておく
	static const int kSensitivityMin = 20;  // 0.20 倍
	static const int kSensitivityMax = 300; // 3.00 倍
	static const int kSensitivityStep = 5;  // 0.05 倍ずつ
	static const int kBrightnessMin = -50;	// 暗い所がもっと暗い（指数 1.41）
	static const int kBrightnessMax = 50;	// 暗い所が明るい（指数 0.71）
	static const int kBrightnessStep = 5;

	int masterVolume;     // 全体の音量（0〜100）
	int effectVolume;     // 効果音の音量（0〜100。AudioEngine::BUS_EFFECT）
	int musicVolume;      // 音楽の音量（0〜100。AudioEngine::BUS_MUSIC）
	int mouseSensitivity; // マウスで回す視点の感度（百分率。100 が FirstPersonControl の元の速さ）
	int padSensitivity;   // パッドの右スティックで回す視点の感度（百分率。100 が LookParams のスティックの速さ）
	int brightness;       // 画面の明るさ（-50〜+50。0 で元のまま）

	GameSettings();	// 既定値（全体と効果音の音量は 100、音楽は kDefaultMusicVolume、感度はどちらも 100、明るさは 0）

	bool load();									// 保存したファイルから読む。ファイルが無い・壊れているときは false を返し、値は既定値のまま。読めた値は範囲に収める
	bool save() const;								// ファイルへ書く。書けなければ false（遊ぶのは続けられるので止めない）
	void applyAudio( AudioEngine& audio ) const;	// 音量を audio へ流す（起動時と、設定画面で変えたとき）
	float mouseSensitivityScale() const;			// マウスの感度の倍率（1 が元の速さ）
	float padSensitivityScale() const;				// パッドの感度の倍率（1 が元の速さ）
	float brightnessExponent() const;				// 画面の色に掛けるガンマの指数（1 で元のまま。小さいほど暗い所が明るい）
};

#endif
