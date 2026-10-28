#ifndef INCLUDED_ZOMBIE_CRAWLER_H
#define INCLUDED_ZOMBIE_CRAWLER_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/Zombie/ZombieCrawl.h"
#include "GameSrc/Zombie/ZombieCrawlBody.h"

class ZombieCrawlLayout;
class ZombieDismemberment;
class SkinnedMeshInstance;
class CharacterMover;

// 脚を失って倒れ、腕で体を引きずるゾンビの「引きずる体」（1 体ぶん。SummaryPlan/個別仕様/ZombieSplit4_Plan.txt の 8 章）。
// 腕の骨の動き（ZombieCrawl。姿勢の書き換え）と、胸の置き場所（ZombieCrawlBody）を持ち、腕の往復の速さの段階・目の高さ・
// 腕が残っているかを知る（這う速さ・腕で引く長さ・胸の置き場所の修正の入口。値の理由は ZombieCrawl_Plan.txt）。
// いつ引きずり始めるか・どう追うか・攻撃を始めるか・誰に顔を向けるかは持ち主（Zombie）が決める。
// 腕で引いた長さを、壁・車に当たりながら動かすのも持ち主（ZombieLocomotion::walkStep）。
class ZombieCrawler{
public:
	// 腕の往復の速さの段階。
	enum class Effort{
		Still,    // 止める（つかみに行っている間・両腕を失った体）
		Struggle, // その場でゆっくりもがく（気付いていない・追いついて待っている）
		Crawl,    // 引きずって進む（追っている）
	};

	// layout: 曲げる骨の設計図。dismemberment: 腕が残っているかを見る（読むだけ）。どちらも所有権は受け取らない。nullptr 不可。
	// この体より長く生きていること（layout は ZombieModel が持つ。dismemberment は同じ Zombie のメンバで、この体より先に宣言する）。
	ZombieCrawler( const ZombieCrawlLayout* layout, const ZombieDismemberment* dismemberment );
	ZombieCrawler( const ZombieCrawler& ) = delete;
	ZombieCrawler& operator=( const ZombieCrawler& ) = delete;

	// 腕の骨の動き。持ち主が、メッシュの姿勢の書き換えの並び（BonePoseModifierChain）へ足すのに使う。
	ZombieCrawl* arms();
	bool isStarted() const; // 引きずり始めたか
	bool canPull() const;   // 引きずれる腕（前腕から先）が 1 本でも残っているか（両腕を失ったら進まず、つかまない）

	// 倒れ終わった姿勢（mesh の今の骨）から胸と進む向きを測り、腕を動かし始める。見た目は動かさない。
	// footing: 今の足元。angleY: 今の見た目の向き（度）。進む向きへ書き換える。nullptr 不可。
	// 骨がまだ引けない（update 前）なら false を返し、何も変えない。
	bool start( const SkinnedMeshInstance& mesh, const GameLib::Math::Vector3& footing, float* angleY );
	// 胸の位置（ワールド）。追う・回り込む・壁に当たるのはこの位置。start の前は意味が無い。
	const GameLib::Math::Vector3& center() const;
	// 自キャラの目が見えているかを調べる所（胸から少し上。伏せているので低い）。
	GameLib::Math::Vector3 eye() const;

	// 腕を 1 ティック動かす。moving なら、腕で引いて体が前へ進む長さ（m。残っている腕の分だけ）を返す。moving でなければ 0。
	// reaching: つかみに行っている・つかんでいる間（両腕とも前へ伸ばす）。
	float moveArms( Effort effort, bool reaching, bool moving );
	// 胸を step だけ動かした（壁・車に沿って滑った後の量）。
	void moveCenter( const GameLib::Math::Vector3& step );
	// 胸の位置と進む向き angleY から、見た目の向きを mesh へ、足元を footing へ書き、mesh を足元へ置く。mesh・footing: nullptr 不可。
	void place( float angleY, const CharacterMover& mover, SkinnedMeshInstance* mesh, GameLib::Math::Vector3* footing );
	// 顔を向ける先（骨格の根の空間）。hasTarget が false なら前を見る。毎ティック、置き場所を決めた後に渡す。
	void look( bool hasTarget, const GameLib::Math::Vector3& rootPoint );

private:
	bool hasArm( ZombieCrawlLayout::Side side ) const; // その腕（前腕から先）がまだあるか

	const ZombieCrawlLayout* mLayout;            // 借りているだけ
	const ZombieDismemberment* mDismemberment;   // 借りているだけ
	ZombieCrawl mArms;                           // 腕の骨の動き（姿勢の書き換え）
	ZombieCrawlBody mBody;                       // 胸の位置と、見た目の向きから進む向きまでのずれ
};

#endif
