#ifndef INCLUDED_MAGAZINE_H
#define INCLUDED_MAGAZINE_H

// 銃に入っている弾倉の弾の数。決めるのは「何発入っているか」と「あと何発入るか」だけ
// 予備の弾（持ち物の、銃に合った弾）・込めるのにかかる時間（WeaponReload）・撃つ間隔（FireControl）は持たない
// 弾倉に何発入るかは銃ごとの表（WeaponCatalog の magazineCapacity）で決まり、作る時に受け取る
class Magazine{
public:
	explicit Magazine( int capacity ); // 満タンで始まる。capacity: 弾倉に入る弾の数（1 以上）

	int rounds() const;				// 入っている弾の数
	bool isEmpty() const;
	int missingRounds() const;		// あと何発入るか（満タンなら 0）
	void takeRound();				// 1発撃って減らす。空のときは呼ばないこと（ASSERT で止める）
	void setRounds( int rounds );	// 入っている弾の数を rounds にする（0〜弾倉の大きさに収める）。拠点で覚えた数を出撃の始めに戻すとき
	void load( int count );			// count 発込める。0 以上 missingRounds 以下であること（ASSERT で止める）

private:
	int mCapacity;
	int mRounds;
};

#endif
