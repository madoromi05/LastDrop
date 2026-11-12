#ifndef INCLUDED_MODAL_CONTROLLER_H
#define INCLUDED_MODAL_CONTROLLER_H

class AudioEngine;
class FirstPersonControl;

// 全画面のモーダル（一時停止・カバン・漁り/中を見る画面）の、どれが前に出ているかの状態と、
// 開く/閉じるときの共通の切り替えを 1 か所に持つ。3 つとも「開く＝世界の音を止めカーソルを外す／閉じる＝音を戻しカーソルをロック」で同じ骨格
// 画面ごとの固有の処理（PauseMenu を開く・弾倉の数の退避・拾った物・給油の判定など）は GameScene に残る
// ここは状態と共通遷移だけを受け持つ（入力の振り分けと描画の選択は、GameScene が mode() を見て行う）
class ModalController{
public:
	enum class Mode{
		None,  // どのモーダルも出ていない（世界が動いている）
		Pause, // 一時停止（PauseMenu）
		Bag,   // カバン（InventoryScreen）
		Loot,  // 漁れる場所・給油口の中を見る画面（LootScreen）
	};
	ModalController( AudioEngine* audio, FirstPersonControl* control );	// audio・control: 借りるだけ（所有しない）。この Controller より長く生きていること。nullptr 不可
	void enter( Mode mode );											// モーダルを開く。世界の音を止め、押していた移動・撃つを放し、カーソルを外す（ボタン・マスをマウスで操作するため）
	// モーダルを閉じる。止めた音を戻し、カーソルをロックし直す（視点を回せるように）
	void exit();
	Mode mode() const { return mMode; }
	bool isOpen() const { return mMode != Mode::None; }					// どれかが開いている間は世界を進めない
	bool isPause() const { return mMode == Mode::Pause; }
	bool isBag() const { return mMode == Mode::Bag; }
	bool isLoot() const { return mMode == Mode::Loot; }

private:
	AudioEngine* mAudio;          // 借りているだけ
	FirstPersonControl* mControl; // 借りているだけ
	Mode mMode;
};

#endif
