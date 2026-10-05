#ifndef INCLUDED_ISCENE_H
#define INCLUDED_ISCENE_H

// シーン共通インターフェース。
// ゲームの更新（update）は 1/60 秒（1ティック。GameTime.h）ずつ、描画（draw）はモニターの速さで呼ばれるので、
// 1回の描画の間に update が 0 回のことも、複数回のこともある。SceneManager が次の順に呼ぶ。
//   beginFrame() → update() を 0 回以上 → draw()
// 注意: GameLib::Scene 名前空間と名前衝突しないよう、あえて IScene とする。
class IScene{
public:
	virtual ~IScene(){}
	// 1回の描画につき1回、update より前に呼ばれる。入力の読み取り・マウスで視点を回すなど、
	// update の回数に左右されてはいけないもの（2回進めたら2倍回る、0回なら入力が消える等）をここで行う。
	virtual void beginFrame() = 0;
	// ゲームを1ティック進める。移動・アニメ・当たり判定など、時間で進むものはすべてここで行う。
	virtual void update() = 0;
	// 1回の描画につき1回、update の後に呼ばれる。視点を置いて描く。
	// 視点・音の聞き手を置き直すなど描く前の準備で状態を書き換えるので const にしない
	// （何回描いても結果が変わらない「描くだけ」の部分は、各シーンの中で const 関数に分ける）。
	virtual void draw() = 0;
};

#endif
