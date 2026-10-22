#ifndef INCLUDED_MAGAZINE_H
#define INCLUDED_MAGAZINE_H

// 銃に入っている弾倉の弾の数。決めるのは「何発入っているか」と「あと何発入るか」だけ。
// 予備の弾（持ち物の ITEM_AMMO）・込めるのにかかる時間（WeaponReload）・撃つ間隔（FireControl）は持たない。
// 弾倉に何発入るかを変えるときは Magazine.cpp の定数だけを直す。
class Magazine{
public:
	Magazine(); // 満タンで始まる

	int rounds() const;			// 入っている弾の数
	bool isEmpty() const;
	int missingRounds() const;	// あと何発入るか（満タンなら 0）
	// 1発撃って減らす。空のときは呼ばないこと（ASSERT で止める）。
	void takeRound();
	// count 発込める。0 以上 missingRounds 以下であること（ASSERT で止める）。
	void load( int count );

private:
	int mRounds;
};

#endif
