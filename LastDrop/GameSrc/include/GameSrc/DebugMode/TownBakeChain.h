#ifndef INCLUDED_TOWN_BAKE_CHAIN_H
#define INCLUDED_TOWN_BAKE_CHAIN_H

#include <vector>

// F6 の Town タブの Bake・Bake all が、車の置き場所ごとに出撃を読み直して床を書き出す順番
// 床はその車を置いた当たり判定の形から作るので、書く車の置き場所ごとに、その車を置いた出撃を読み終えてから書く
// 書く車の置き場所の残りと、全部書いたら戻る置き場所を持ち、今読んである出撃から次にすることを決める
// 出撃を読み直す・書き出すのは呼ぶ側。書けなかった・読み直しても今の手直しと合わなかったら、残りを書かずに止まる
// Debug・Profile ビルドだけ（Release では中身をビルドしない）
class TownBakeChain{
public:
	enum class Step{
		Idle,		// 動いていない
		Bake,		// 今の出撃の車の置き場所を書き出す。書いたら finishBake を呼ぶ
		Reload,		// reloadSpot の車の置き場所で出撃を読み直す。読み終えたら、もう一度 next を呼ぶ
		Done,		// 全部書いて、始めの車の置き場所の出撃に居る。この呼び出しで止まる
		OutOfDate,	// 読み直した出撃も今の手直しと合わない（読み直しの間に手直しを変えた）。この呼び出しで止まる
	};

	TownBakeChain();

	// spots: 書く車の置き場所。startSpot: 押したときの出撃の車の置き場所（全部書いたらここへ戻る）。spots が空なら動かない
	// startSpot も書くなら最後に回す（最後に書いた出撃がそのまま始めの置き場所になり、戻る読み直しが要らない）
	void start( const std::vector< int >& spots, int startSpot );
	bool isRunning() const;
	// 次にすること。loadedSpot: 今の出撃（読み終えた物）の車の置き場所
	// loadedIsCurrent: 今の出撃の町が、今の手直しを当てた町と同じ床・表になるか（TownBakeStatus::bakesSame）
	Step next( int loadedSpot, bool loadedIsCurrent );
	int reloadSpot() const;				// 最後に next が Reload を返したときの、読み直す車の置き場所
	void finishBake( bool written );	// Bake の後に呼ぶ。書けたら残りから外す。書けなければ止まる
	void stop();						// 途中でやめる（読み直しが別の切り替えに替わった など）

private:
	Step requestReload( int spot );	// spot の車の置き場所で読み直すことにして Reload を返す

	std::vector< int > mSpots;	// まだ書いていない車の置き場所。前から順に書く
	int mStartSpot;
	int mReloadSpot;
	bool mRunning;
	bool mReloaded;				// start の後に読み直したか。読み直した出撃が今の手直しと合わなければ、もう読み直さずに止まる
};

#endif
