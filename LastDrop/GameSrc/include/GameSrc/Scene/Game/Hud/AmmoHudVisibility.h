#ifndef INCLUDED_AMMO_HUD_VISIBILITY_H
#define INCLUDED_AMMO_HUD_VISIBILITY_H

// 右下の弾数（AmmoHud）を、いつ・どれだけの濃さで出すかを決める
// 決めるのは「出す条件が切れてから何ティック残すか」と「濃さ（0〜1）」だけ。何を出すか・どこに描くかは AmmoHud
// 出す条件（覗いている・リロード中）を決めるのは呼ぶ側（GameScene）なので、条件を変えてもこのクラスは変わらない
// 秒はティックに直して数える（ゲームの更新から呼ぶので、一時停止・カバンを開いている間は進まない）
class AmmoHudVisibility{
public:
	AmmoHudVisibility();

	// 1 ティック進める。wanted: このティックに出す条件があるか
	// 条件がある間は濃くなり、切れてから 2 秒（kLingerSeconds）は出したまま、その後に薄くなって消える
	void update( bool wanted );
	float opacity() const;	// 今の濃さ（0 = 出さない、1 = 一番濃い）
	bool isVisible() const;	// 濃さが 0 より大きいか（描くかどうか）

private:
	int mLingerTicks; // 条件が切れた後、あと何ティック出したままにするか
	float mOpacity;
};

#endif
