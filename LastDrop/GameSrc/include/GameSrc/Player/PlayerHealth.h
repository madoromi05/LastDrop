#ifndef INCLUDED_PLAYER_HEALTH_H
#define INCLUDED_PLAYER_HEALTH_H

// 体力の状態（バイオハザードの Fine / Caution / Danger）。残りの割合で決まる。並びは元気な順
enum class PlayerCondition{
	Fine,    // 元気
	Caution, // 傷ついている（少し遅くなる）
	Danger,  // 危ない（もっと遅くなり、照準のぶれが大きくなり、画面の周りが赤くなる）
};

const char* playerConditionName( PlayerCondition condition );	// 状態の名前（確認用の表示）

// 自キャラの体力。バイオハザードと同じく、体全体で 1 つだけ持つ
// ・0 になったら死ぬ
// ・残りの割合で状態（PlayerCondition）が変わり、状態で歩く・走る速さが落ちる（読むのは PlayerCharacter）
// ・包帯で戻る（heal）
// 最大の体力・状態の境目・速さの倍率は、この .cpp だけが知っている
class PlayerHealth{
public:
	PlayerHealth(); // 満タンで始める

	void takeDamage( float amount );	// amount のダメージを受ける。死んだ後は何もしない
	void heal( float amount );			// amount だけ戻す（最大で止める）。死んだ後は何もしない（死んでから包帯で起き上がらないように）
	bool isInjured() const;				// 最大より減っているか（減っていなければ、カバンの画面に「使う」を出さない）

	bool isDead() const;               // 体力が 0 になったか
	float current() const;             // 残り（0 以上）
	float maxHealth() const;           // 最大
	PlayerCondition condition() const; // 今の状態
	float walkSpeedScale() const;      // 歩く・走る速さに掛ける倍率（Fine で 1。傷つくほど小さい）
private:
	float mCurrent;
};

#endif
