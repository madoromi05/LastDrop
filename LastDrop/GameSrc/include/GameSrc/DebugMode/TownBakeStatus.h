#ifndef INCLUDED_TOWN_BAKE_STATUS_H
#define INCLUDED_TOWN_BAKE_STATUS_H

#include <vector>

class TownMap;

// 事前に作った床と表のファイルが、今の手直しを当てた町で使えるか（F6 の Town タブの状態の行）
// 車の置き場所ごとの床と、隠れの表について、ファイルが無い・あるが指紋が合わない・使える を見分ける
// 指紋は Release が床を作る長さで求める（次の起動の Release がそのファイルを読むかと同じ答えにするため）
// ファイルを全部読むので、毎フレームは求め直さない（呼ぶ側が、手直しが変わった・Bake した・読み直した ときだけ update する）
// Debug・Profile ビルドだけ（Release では中身をビルドしない）
class TownBakeStatus{
public:
	enum class State{
		Missing,	// ファイルが無い（まだ書き出していない）
		Stale,		// あるが、今の町と指紋が合わない（手直しの後に書き出していない）か、壊れている
		File,		// 次の出撃でそのまま読める
	};

	static State classify( bool exists, bool matches );	// ファイルがあるか・今の指紋で読めるか から状態を決める
	static const char* stateName( State state );		// F6 に出す名前（file・stale・missing）
	// loaded の町（今の出撃）に carSpot の車を置いて書き出した床と表が、edited の町（今の手直しを当てた町）のファイルとして読めるか
	// 指紋を比べるだけ。置き場所は cm に丸めて比べ、指紋に入らない物（ゾンビの湧く候補など）の違いは見ない
	static bool bakesSame( const TownMap& loaded, const TownMap& edited, int carSpot, float agentRadius );

	TownBakeStatus();
	// 今の手直しを当てた町 map の、車の置き場所ごとの床と表のファイルを読んで、状態を求め直す
	// agentRadius: 床の縁を壁から離す長さ。Release の既定値（ZombieDirector::defaultNavMeshRadius）を渡す
	void update( const TownMap& map, float agentRadius );
	const std::vector< State >& navMeshes() const;	// 車の置き場所ごと（TownMap::escapeCarSpots の番号の順）
	State visibility() const;
	bool isUpToDate() const;						// 全部 File か（update の前は false）
	std::vector< int > spotsToBake( int currentSpot ) const;	// 床が File でない車の置き場所（番号の順）。床が全部 File で表だけ違うなら、表を書くために currentSpot だけ

private:
	std::vector< State > mNavMeshes;
	State mVisibility;
};

#endif
