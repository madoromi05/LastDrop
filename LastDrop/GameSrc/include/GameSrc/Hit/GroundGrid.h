#ifndef INCLUDED_GROUND_GRID_H
#define INCLUDED_GROUND_GRID_H

#include "GameLib/Math/Vector3.h"
#include <vector>

// 上から見た一様なます（一様グリッド）。「どのますに、何番の物がかかっているか」だけを持つ。
// 物そのもの（形・当たり判定）は持たない。番号の意味は使う側（TownGround）が決める。
// 地面の部品は数百枚あり、線分を聞くたびに全部を調べると重いので、線分が通るますの物だけに絞る
// （計画書 OptimizationS246_Plan.txt 4 章）。置いた後に物は動かない前提（作るのは 1 回）。
// 高さ（y）は見ない。地面は平らで、ますの中の物は高さに関わらず候補にする。
class GroundGrid{
public:
	GroundGrid();
	// ますを作り直す（前の中身は捨てる）。boundsMin〜boundsMax（ワールド、y は見ない）を cellSize m 四方のますで覆う。
	// 範囲は、後で add する物の箱を全部囲む大きさにすること（はみ出した所は端のますに入る）。cellSize は 0 より大きいこと。
	void reset( const GameLib::Math::Vector3& boundsMin, const GameLib::Math::Vector3& boundsMax, float cellSize );
	// item 番の物を、箱 boundsMin〜boundsMax がかかる全部のますに登録する。item は 0 以上。
	void add( int item, const GameLib::Math::Vector3& boundsMin, const GameLib::Math::Vector3& boundsMax );
	// origin から delta だけ進む線分が上から見て通るますにかかる物の番号を、items へ入れる（前の中身は捨てる）。
	// 同じ物は 1 度だけ入れる。並びは線分の進む順のますごと（近い方のますの物が先）。
	// 線分がますの範囲の外を通る所は調べない。items: 出力先。nullptr 不可。
	void collect( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta, std::vector< int >* items ) const;

private:
	// 1 つのますの物を、まだ入れていなければ items へ足す。
	void collectCell( int cellX, int cellZ, std::vector< int >* items ) const;
	int cellIndexX( float x ) const; // 端のますに切り詰める
	int cellIndexZ( float z ) const;

	float mMinX;
	float mMinZ;
	float mCellSize;
	int mCountX;
	int mCountZ;
	std::vector< std::vector< int > > mCells; // 添え字は cellZ * mCountX + cellX
	// collect の中で、同じ物を 2 度入れないための印（物ごとに、最後に入れた問い合わせの番号）。
	// 聞く側から見れば collect は中身を変えないので mutable にする。
	mutable std::vector< unsigned int > mVisitedQuery;
	mutable unsigned int mQuery;
};

#endif
