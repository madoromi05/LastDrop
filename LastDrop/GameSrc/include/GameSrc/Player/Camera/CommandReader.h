#ifndef INCLUDED_COMMAND_READER_H
#define INCLUDED_COMMAND_READER_H

#include "GameSrc/Player/PlayerCommand.h"

// キー・パッドの入力を、自キャラへの操作（PlayerCommand）にまとめる係（SummaryPlan/個別仕様/設計と分割/HighPrioritySplit_Plan.txt 6 章。
// 2026-11-09 FirstPersonControl から分けた）。視点は回さない（それは FirstPersonControl）。
// 押した瞬間の入力（holsterPressed など）は、consumePressed で消すまで貯める。
// 1回の描画の間に update が0回でも入力が消えず、2回あっても2回反応しないようにするため。
// 撃つ入力は「一度離すまで止める」印を持つ（メニューのボタンを押したままシーンに入った・解除中のクリックでロックし直した・
// 一時停止した瞬間に、そのクリックで撃ち出さないように）。
class CommandReader{
public:
	CommandReader();

	// 今の入力を読む（1回の描画につき1回、Pad::update の後）。cursorLocked: カーソルをロックしているか（解除中は撃たない）。
	// 押されている間の入力は今の状態で上書きし、押した瞬間の入力は使われるまで残す。
	void read( bool cursorLocked );
	// 操作を「何もしない」にし（その場で立ち止まる・撃たない・覗かない）、撃つ入力を一度離すまで止める。
	// 押した瞬間の入力は read と同じく consumePressed に任せる（ここで消すと、切り替えた描画の入力が落ちる）。
	void clear();
	// 撃つ入力を一度離すまで止める（解除中のクリックでロックし直したとき）。
	void suppressFire();
	// 押した瞬間の入力を消す。command をキャラへ渡した後（update のたび）に呼ぶ。
	void consumePressed();

	const PlayerCommand& command() const;
	// 撃つ入力を止めている最中か（離すまで撃たない。1 発目が出ない件の記録 FireProbe が読むだけ）。
	bool isFireSuppressed() const;

private:
	PlayerCommand mCommand;
	bool mFireSuppressed; // 撃つ入力を一度離すまで、撃つ操作を出さないか（始めは true）
};

#endif
