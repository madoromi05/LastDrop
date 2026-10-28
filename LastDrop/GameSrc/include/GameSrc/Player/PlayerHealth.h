#ifndef INCLUDED_PLAYER_HEALTH_H
#define INCLUDED_PLAYER_HEALTH_H

// 体力の状態（バイオハザードの Fine / Caution / Danger）。残りの割合で決まる。並びは元気な順。
enum class PlayerCondition{
	Fine,    // 元気
	Caution, // 傷ついている（少し遅くなる）
	Danger,  // 危ない（もっと遅くなり、照準のぶれが大きくなり、画面の周りが赤くなる）
};

// 状態の名前（確認用の表示）。
const char* playerConditionName( PlayerCondition condition );

// 自キャラの体力。バイオハザードと同じく、体全体で 1 つだけ持つ（2026-10-26 ユーザー決定。それまではタルコフの 7 部位）。
// ・0 になったら死ぬ
// ・残りの割合で状態（PlayerCondition）が変わり、状態で歩く・走る速さが落ちる（読むのは PlayerCharacter）
// 回復はしない（回復アイテムを作るときに入口を足す）。最大の体力・状態の境目・速さの倍率は、この .cpp だけが知っている。
class PlayerHealth{
public:
	PlayerHealth(); // 満タンで始める

	// amount のダメージを受ける。死んだ後は何もしない。
	void takeDamage( float amount );

	bool isDead() const;               // 体力が 0 になったか
	float current() const;             // 残り（0 以上）
	float maxHealth() const;           // 最大
	PlayerCondition condition() const; // 今の状態
	float walkSpeedScale() const;      // 歩く・走る速さに掛ける倍率（Fine で 1。傷つくほど小さい）
private:
	float mCurrent;
};

#endif
