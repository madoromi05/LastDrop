#ifndef INCLUDED_ZOMBIE_CRAWL_H
#define INCLUDED_ZOMBIE_CRAWL_H

#include "GameSrc/Render/SkinnedMesh/BonePoseModifier.h"
#include "GameSrc/Zombie/Crawl/ZombieCrawlLayout.h"
#include "GameLib/Math/Vector3.h"
#include <vector>

// 脚を失って倒れたゾンビ1体ぶんの「体を引きずる動き」。這うアニメが無いので、倒れ終わった姿勢（死亡アニメの最後）のまま、
// 骨を描く直前に曲げる（BonePoseModifier）。手足は地面から浮かせず、地面に付けたまま這う
//   胸：少し反らせる（前腕で上体を支える）
//   頭：相手（自キャラ）の方へ顔を向ける。相手が居なければ前を見る。首が回りすぎないよう角度に上限がある
//   腕：手を地面に置く所を決め、肩 → 肘 → 手首を 2 本の骨の IK で合わせる。前半で手を地面すれすれに前へ運び、
//       後半は手を地面に付けたまま、体が進む分だけ後ろへずらす（地面に手を突いたまま体を引き寄せて見える）。左右交互
//   脚：腕と反対の側の脚を、地面に沿って（真上の軸のまわりで）外へ開いて膝を曲げ、腕で引く間に伸ばす（地面を蹴る）
// 決めるのは「腕を今どこまで伸ばしているか」と「このティックに腕で引いた長さ」だけ。体を動かす・向きを変えるのは持ち主（Zombie）
// 骨格の根の空間の高さ 0 を地面とみなす（足元が地面の高さにあるため）
// どの骨で曲げるかは ZombieCrawlLayout（種類ごとに共有）を借りる。値の理由は SummaryPlan/個別仕様/ゾンビ/ZombieCrawl_Plan.txt の 3・4 章
class ZombieCrawl : public BonePoseModifier{
public:
	explicit ZombieCrawl( const ZombieCrawlLayout* layout );	// layout: 曲げる骨の設計図。所有権は受け取らない。nullptr 不可。この体より長く生きていること（ZombieModel が持つ）

	// 動き始める（倒れ終わったとき）。体は決まった時間をかけて、倒れた姿勢から引きずる形へ移る
	void start();
	bool isStarted() const;
	// 1ティック進める。始めていなければ何もしない。止める（死んだ）ときは呼ぶのをやめる（体はそのときの形のまま残る）
	// pace: 腕の往復の速さ（1 = 引きずって進む速さ。小さいほどゆっくりもがく。0 なら腕の往復を止める）
	// reaching: 腕を振っている間（両腕とも前へ伸ばして足首の高さで手を止め、脚は伸ばして止める）
	void update( float pace, bool reaching );
	void setLookTarget( bool hasTarget, const GameLib::Math::Vector3& rootPoint );	// 顔を向ける先（骨格の根の空間）。hasTarget が false なら前を見る。毎ティック、update と同じ頃に渡す（置き場所を決めた後）
	// 直前の update で、腕で引いて体が前へ進む長さ（m）。引いている腕の分だけ足す
	// hasLeftArm / hasRightArm: その腕がまだあるか（無い腕では引けない）
	float pullDistance( bool hasLeftArm, bool hasRightArm ) const;
	// 今の形へ、胸・頭・腕・脚を曲げる（BonePoseModifier）。のけぞり（ZombieFlinch）より先に掛ける（撃たれた所は、この形から押される）
	// 顔の向きを少しずつ相手へ回すので、1 ティックに 1 回だけ呼ばれること（SkinnedMeshInstance::update の中）
	virtual void apply( std::vector< GameLib::Math::Matrix34 >* pose ) override;

private:
	void liftChest( const GameLib::Math::Vector3& up, std::vector< GameLib::Math::Matrix34 >* pose ) const;	// 胸を反らせる。forward / up: 体の向きと上（骨格の根の空間。長さ1）
	// 顔を相手（居なければ前）へ向ける。向きは少しずつ回す（mLookDirection を進める）
	void turnHead( const GameLib::Math::Vector3& forward, const GameLib::Math::Vector3& up,
		std::vector< GameLib::Math::Matrix34 >* pose );
	// side の腕の手を、今の往復の位置と攻撃で腕を伸ばす量から決めた地面の点へ置く
	void placeHand( ZombieCrawlLayout::Side side, const GameLib::Math::Vector3& forward, const GameLib::Math::Vector3& up,
		std::vector< GameLib::Math::Matrix34 >* pose ) const;
	// side の脚を、反対の側の腕の往復に合わせて地面に沿って曲げる（腕を伸ばす間に開いて曲げ、引く間に伸ばす）
	void sweepLeg( ZombieCrawlLayout::Side side, const GameLib::Math::Vector3& forward, const GameLib::Math::Vector3& up,
		std::vector< GameLib::Math::Matrix34 >* pose ) const;
	float armPhase( ZombieCrawlLayout::Side side ) const; // side の腕の往復の位置（0 以上 1 未満）

	const ZombieCrawlLayout* mLayout; // 借りているだけ
	bool mStarted;
	float mWeight;  // 決めた形へ曲げる量（0 = 倒れた姿勢のまま、1 = 決めた形）。始めてから少しずつ 1 へ
	float mPhase;   // 左腕の往復の位置（0 以上 1 未満。右腕は半周ずらす）
	float mReach;   // 攻撃で腕を伸ばす量（0 = 往復の形、1 = 両腕とも前へ伸ばす）
	float mPulled[ ZombieCrawlLayout::SIDE_NUMBER ]; // 直前の update で、腕ごとに引いた長さ（m）
	// ── 顔の向き ──
	bool mHasLookTarget;
	GameLib::Math::Vector3 mLookTarget;    // 顔を向ける先（骨格の根の空間）
	bool mHasLookDirection;                // mLookDirection が入っているか（最初の apply で今の顔の向きを入れる）
	GameLib::Math::Vector3 mLookDirection; // 今向けている顔の向き（骨格の根の空間。長さ1）。向けたい向きへ少しずつ回す
};

#endif
