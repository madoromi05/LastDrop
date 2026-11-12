#ifndef INCLUDED_PATH_BUDGET_H
#define INCLUDED_PATH_BUDGET_H

#include <vector>

// 1 ティックに道を探して（NavMesh::findPath）よい回数を数え、超えた頼みを順番待ちにする係（SummaryPlan/個別仕様/最適化と計測/OptimizationC_Plan.txt 6 章）
// 銃声で何十体もが一度に向かい直しても、探す処理が 1 ティックに集まらないようにする（Detour の dtPathQueue と同じ考え）
// 待つ順は先に頼んだ順（ずっと待たされる体が出ないように）。待つ間どう歩くかは頼んだ側（ZombiePath）が決める
// 待つ体は毎ティック頼み直す決まり。前のティックに頼み直さなかった頼みは捨てる（待つ間に倒れた体などが列の前を塞ぎ続けないように）
// 持ち主は GameWorld。共有の棚（shared）に置き、ゾンビの ZombiePath が借りる
class PathBudget{
public:
	PathBudget();
	PathBudget( const PathBudget& ) = delete;
	PathBudget& operator=( const PathBudget& ) = delete;

	// ティックの始めに呼ぶ。このティックに探してよい回数を searchesPerTick に戻し（ZombieCrowdParams::pathSearchesPerTick）、
	// 前のティックに頼み直さなかった頼みを捨てる。待っている数を回数の印 Path.waiting に足す
	void beginTick( int searchesPerTick );
	// requester（頼む物を見分ける印。中身は読まない）が今探してよいか。よければ列から外して true
	// だめなら列の後ろに並べて（並んでいればそのまま）false。false なら次のティックにもう一度頼むこと
	bool tryAcquire( const void* requester );
	// 頼みを取り下げる（探す必要が無くなった・頼んだ物が消える）。並んでいなければ何もしない
	void cancel( const void* requester );
	// requester が順番を待っているか（確認用の表示）
	bool isWaiting( const void* requester ) const;

private:
	// 待っている頼み 1 つ
	struct Request{
		const void* requester;
		int lastTick; // 最後に頼んだティック（mTick の値）
	};
	// requester の頼み（並んでいなければ end）
	std::vector< Request >::iterator find( const void* requester );
	std::vector< Request >::const_iterator find( const void* requester ) const;

	std::vector< Request > mQueue; // 待っている頼み（先頭ほど先に頼んだ）。数十体なので並びで足りる
	int mRemaining;                // このティックに、あと何回探してよいか
	int mTick;                     // beginTick を呼んだ回数（頼みが古いかを見る）
};

#endif
