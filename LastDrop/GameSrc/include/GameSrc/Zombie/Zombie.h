#ifndef INCLUDED_ZOMBIE_H
#define INCLUDED_ZOMBIE_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshInstance.h"
#include "GameSrc/Collision/ICollisionBody.h"
#include "GameSrc/Hit/IHittable.h"
#include "GameSrc/Hit/IDamageable.h"
#include "GameSrc/World/IWorldMember.h"
#include "GameSrc/Hit/HitboxSet.h"
#include "GameSrc/Zombie/ZombieHealth.h"
#include "GameSrc/Zombie/ZombieFlinch.h"
#include "GameSrc/Zombie/ZombieDismemberment.h"
#include "GameSrc/Zombie/ZombieCrawl.h"
#include "GameSrc/Render/SkinnedMesh/BonePoseModifierChain.h"
#include "GameSrc/Zombie/ZombieChase.h"
#include "GameSrc/Zombie/ZombiePath.h"
#include "GameSrc/Zombie/ZombieAttack.h"
#include "GameSrc/Collision/CharacterMover.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Scene/Animation.h"
#include <vector>

class ZombieModel;
class CollisionWorld;
class HitWorld;
class StreetGraph;
class ZombieCrowd;
struct ChaseTarget;

// ゾンビ1体。立っている間は自キャラに気付くと歩いて追ってくる（どこへ行くかは ZombieChase が決め、
// ここが向きを変えて歩き、壁・他のキャラに沿って滑る移動を CharacterMover に任せる。SummaryPlan/個別仕様/ZombieChase_Plan.txt）。
// 追っている・探している間、行き先へまっすぐ通れなければ、ZombiePath に聞いた通り道に沿って建物を回り込む。
// 見失った後に着けない所へ向かっていれば、探すのをやめさせる（SummaryPlan/個別仕様/ZombiePath_Plan.txt）。
// 気付いていない・追いついた・見失って探し終えたときは、立つアニメで立ち止まる。
// 形と、当たり判定の大きさ・部位の箱の設計図は ZombieModel を借りる（同じ種類の全員で共有。1体ごとには求めない）。
// 見た目は SkinnedMeshInstance に任せ、体を真上から囲む長方形（BodyFootprint）を差し出して、他のキャラの移動を止める（ICollisionBody）。
// 部位ごとの当たり判定（命中判定用）も持ち、弾がどの部位に当たったかを答えられる（IHittable）。
// 撃たれると体力（ZombieHealth）が減り、倒れる条件を満たすと死亡アニメで倒れ、しばらく死体として残ってから地面へ沈んで消える（IDamageable）。
// 生きている間に撃たれると、1 発ごとに当たった部位がのけぞり（ZombieFlinch）、ときどきよろける（よろけのアニメを1回再生して歩きへ戻る）。
// よろけは、よろけていない間に当たったときに出し、終わってから少しの間は出さない（頭に当たったときは待たない）。
// フルオートで当て続けても、よろけ続けて動けなくならないようにするため（SummaryPlan/個別仕様/ZombieStagger_Plan.txt）。
// 頭・腕・脚の体力が 0 になると、その部位を切る（ZombieDismemberment が見た目から消し、ここが部位の箱を外す）。
// 千切れた部位を飛ばすのは持ち主（severedByLastDamage と shownWorldPose を読んで作る）。
// 脚を失うと前へ倒れるアニメで倒れ、倒れたまま生きている（Downed。SummaryPlan/個別仕様/ZombieDismember_Plan.txt）。
// 倒れ終わったら、腕で体を引きずって追ってくる（腕の動きは ZombieCrawl。這うアニメが無いので、倒れた姿勢のまま腕の骨を曲げる。
// SummaryPlan/個別仕様/ZombieCrawl_Plan.txt）。追い方は立っている時と同じで、向きを変える中心・見る高さ・壁に当たる所は胸。
// 頭が相手の足元に届いたら、足をつかんで噛む（攻撃のアニメの代わりに時計で ZombieAttack を進める）。
// 追いついたら、つかんで噛む攻撃をする（段階は ZombieAttack が決め、ここが攻撃のアニメを流し、向きを変え、寄る。
// SummaryPlan/個別仕様/ZombieAttack_Plan.txt）。相手に何が起きるか（動けない・体力が減る）は知らない。
// 「つかんでいるか」「このティックに噛んだか」を答えるだけで、相手へ伝えるのは持ち主（GameWorld）。
// つかんでいる間に決まった量のダメージを受けると離して、よろける。攻撃の途中は、撃たれてもよろけない（のけぞりだけ）。
// 名簿から外す・消すのは持ち主（GameWorld）。ここは「もう移動を止めないか（blocksMovement）」「もう消えてよいか（isGone）」を答えるだけ。
class Zombie : public ICollisionBody, public IHittable, public IDamageable, public IWorldMember{
public:
	// 生きているか・倒れているか。並びは時間の順。
	enum class State{
		Alive,   // 立っている（撃たれると体力が減る）
		Downed,  // 脚を失って倒れ、倒れたまま生きている（撃たれると体力が減る。よろけない）
		Falling, // 死亡アニメの途中
		Corpse,  // 倒れ終わって、死体として残っている
		Sinking, // 地面へ沈んでいる（沈み終わったら消えてよい）
	};

	// 使うアニメの名前（文字列は持ち主の定数。コンストラクタの中で引くだけで、覚えない）。
	struct Animations{
		// 歩くアニメと開始時刻（フレーム。体ごとにずらして動きを揃えない）。
		// nullptr ならアニメを付けず、レスト姿勢（T ポーズ）のまま立たせ、追ってもこない（当たり判定の確認用）。
		const char* move;
		float moveStartTime;
		const char* idle;    // 立ち止まっているときのアニメ。nullptr なら立ち止まっている間も歩くアニメのまま
		// 倒れるときのアニメ（脚を失って倒れるときにも使う）。nullptr なら体力が 0 になっても・脚を失っても倒れない
		// （確認用。体力は減って 0 で止まる）
		const char* death;
		// 脚を失って倒れるときのアニメ（前へ倒れてうつ伏せになる物。倒れ終わったら体を引きずる）。nullptr なら death で倒し、
		// 倒れたまま動かない（仰向けに倒れるアニメでは引きずれないため）。
		const char* downed;
		const char* stagger; // よろけるときのアニメ。nullptr ならよろけない（のけぞりだけ）
		// つかんで噛む攻撃のアニメ（atack3）。nullptr なら攻撃しない（追いついても立って待つだけ）
		const char* attack;
	};

	// model: 形・見た目・アニメと、当たり判定の設計図。所有権は受け取らない。nullptr 不可。
	//        このゾンビより長く生きていること（持ち主の GameWorld が、ゾンビより後に破棄する）。
	// collisionWorld: 歩くときに壁・他のキャラを聞く名簿。所有権は受け取らない。nullptr 不可。このゾンビより長く生きていること。
	// streets: 建物を回り込む通り道を聞く道の図。所有権は受け取らない。nullptr 不可。寿命の決まりは collisionWorld と同じ。
	// position / angleY: 足元の位置と向き（度。0 は +Z）。
	// sightPhase: 見通し・まっすぐ通れるかを調べる時刻をずらす番号（何体目か。ZombieChase・ZombiePath）。
	// randomSeed: うろつく先・立つ長さを選ぶ乱数の種（体ごとに違う値。ZombieChase）。
	Zombie( const ZombieModel* model, const CollisionWorld* collisionWorld, const StreetGraph* streets,
		const GameLib::Math::Vector3& position, float angleY, const Animations& animations, int sightPhase, int randomSeed );
	Zombie( const Zombie& ) = delete;
	Zombie& operator=( const Zombie& ) = delete;

	// 1フレーム（1ティック）進める。立っていてよろけていなければ、追う先を考えて向きを変え・歩き、
	// アニメを進め、倒れた後の時間を進め、部位の当たり判定をそのポーズへ置き直す。頂点の変形は draw で行う。
	// target: 追う先。nullptr なら追わない（自キャラが居ない）。hitWorld: 見通しを聞く名簿。
	// crowd: 立って歩けるゾンビの位置の並び（近くの仲間を避けるのに使う。SummaryPlan/個別仕様/ZombieCrowd_Plan.txt の 3 章）。
	//        nullptr なら避けない。借りるだけ。
	void update( const ChaseTarget* target, const HitWorld& hitWorld, const ZombieCrowd* crowd );
	void draw() const;

	// 生きている間だけ体力を減らし、のけぞらせ、当たった部位が 0 になったら切り、条件を満たせばよろけさせる。
	// 脚を失ったら倒れて Downed に、倒れる条件を満たしたら死亡アニメへ繋いで倒し始める（IDamageable）。
	virtual void applyDamage( const DamageInfo& damage );
	// position で銃声がした（届くかは持ち主が決める）。立っている（か、体を引きずっている）、よろけ・攻撃の最中でなければ、
	// 少し遅れてからそこへ探しに来る（ZombieChase::hearNoise。SummaryPlan/個別仕様/ZombieHearing_Plan.txt）。
	void hearNoise( const GameLib::Math::Vector3& position );
	// 最後の applyDamage で切れた部位（無ければ空。次の applyDamage で入れ替わる）。持ち主が千切れた部位を飛ばすのに読む。
	const std::vector< BodyPart >& severedByLastDamage() const;
	// 今見えている骨の姿勢（ワールド。のけぞり・部位欠損の書き換え後）を、骨の番号の順に out へ書く（前の中身は捨てる）。
	// 切れた部位を飛ばすときに、切れた瞬間の姿勢として写す。applyDamage の直後なら、切れた部位はまだ見えている（消えるのは次の update）。
	// out: nullptr 不可。
	void shownWorldPose( std::vector< GameLib::Math::Matrix34 >* out ) const;

	State state() const;
	const GameLib::Math::Vector3& position() const; // 足元の位置（ワールド）
	// 生きているか（Alive か Downed）。
	bool isAlive() const;
	// 脚を失って倒れ終わり、腕で体を引きずっているか（Downed の間だけ true になりうる）。噛む所を脚にするのに持ち主が読む。
	bool isCrawling() const;
	// 体の中心（ワールド）。立っている間は足元、体を引きずっている間は胸。追う・回り込む・壁に当たるのはこの位置。
	GameLib::Math::Vector3 bodyCenter() const;
	// 追ってくる AI の今の考え（Wander / Chase / Search）。確認用の表示に使う。
	ZombieChase::Mode chaseMode() const;
	// 建物を回り込む通り道（確認用の表示に使う）。
	const ZombiePath& path() const;
	// 最後の update で歩く向きへ混ぜた、仲間から離れる向き（水平。長さは強さを掛けた後）。確認用の表示に使う。
	const GameLib::Math::Vector3& separation() const;
	// よろけのアニメの最中か（生きている間だけ true になりうる）。AI を作るとき、この間は動かない。
	bool isStaggering() const;
	// ── 攻撃（ZombieAttack）──
	ZombieAttack::Phase attackPhase() const; // 今の攻撃の段階。確認用の表示に使う
	// 相手を取りに行っている・つかんでいる間か。持ち主は、この間ほかのゾンビに攻撃を始めさせない（ChaseTarget::attackTaken）。
	bool isAttacking() const;
	bool isHoldingTarget() const; // 相手をつかんでいるか
	bool grabbedThisTick() const; // 直前の update で相手をつかんだか
	bool bitThisTick() const;     // 直前の update で相手を噛んだか（ダメージの量は ZombieAttack::biteDamage）
	// 頭の位置（ワールド。頭の部位の箱の中心）。つかまれた相手が顔を向ける先・噛む音を鳴らす所。頭を失っていれば目の高さ。
	GameLib::Math::Vector3 headPosition() const;
	const ZombieHealth& health() const;
	// 他のキャラの移動を止めるか（立っている間だけ。Downed も止めない＝立ち姿の長方形が倒れた体と合わないため）。
	// false になったら、持ち主が移動の名簿から外す（死体・倒れたゾンビの上を歩ける）。
	bool blocksMovement() const;
	// 沈み終わって、消してよいか（持ち主が ActorList::destroyLater する）。
	bool isGone() const;
	// 消えるまでの残り秒（死体・沈んでいる間だけ。それ以外は 0）。確認用の表示に使う。
	float secondsUntilGone() const;

	// 移動の当たり判定（体を真上から囲む長方形。体の向きに合わせて回る）。
	virtual BoxCollider collider() const;
	// 部位ごとの当たり判定。倒れた後も死体の姿勢に付いてくる（死体も弾を止める）。
	virtual const HitboxSet& hitboxes() const;
	// 移動の名簿（体の長方形）と命中の名簿（部位の箱）の両方へ、自分を載せる／外す（IWorldMember）。
	// 外すのは、移動の名簿から先に外れていても安全（名簿が何もしない）。
	virtual void addTo( CollisionWorld* collisionWorld, HitWorld* hitWorld );
	virtual void removeFrom( CollisionWorld* collisionWorld, HitWorld* hitWorld );

private:
	void startFalling();      // 死亡アニメへ繋ぎ（Downed なら倒れる途中のまま続け）、Falling にする
	void startDowned();       // 前へ倒れるアニメ（無ければ死亡アニメ）へ繋ぎ、Downed にする（倒れたまま生きている）
	// ── 体を引きずる（Downed の間。SummaryPlan/個別仕様/ZombieCrawl_Plan.txt）──
	// 倒れ終わるのを待って引きずり始め、追う先を考えて胸を中心に向きを変え、腕で引いた分だけ進む。
	void updateDowned( const ChaseTarget* target, const HitWorld& hitWorld );
	bool canStartCrawling() const; // 前へ倒れるアニメで倒れ終わったか（引きずれる骨がそろっているか）
	// 倒れ終わった姿勢の腰 → 首を進む向きに、胸を体の中心にする（見た目は動かさず、向きと中心の数字だけ合わせる）。
	void startCrawling();
	// 追う先を考え、向きを変える（攻撃していないとき）。追いついたら攻撃を始める。
	// 腕の往復の速さ（ZombieCrawl::update の pace）を返し、前へ進むなら *moving を true にする。
	float crawlChase( const ChaseTarget* target, const HitWorld& hitWorld, bool* moving );
	// 胸の位置と進む向きから、見た目の向き・足元の位置を決め直す（胸の位置は地面の高さへ合わせる）。
	void placeCrawlingBody();
	bool hasArm( ZombieCrawlLayout::Side side ) const; // その腕（前腕から先）がまだあるか
	bool canCrawl() const;    // 引きずれる腕が1本でも残っているか（両腕を失ったら進まず、つかまない）
	// part が 0 になっていて、まだあれば切る（見た目から消し、部位の箱を外し、severedByLastDamage へ足す）。
	void severIfBroken( BodyPart part );
	void updateAfterDeath();  // Falling → Corpse → Sinking と進め、沈む分だけ見た目を下げる
	// 今当たった part でよろけを始めてよいか（よろけていない・間あけが済んだ、または頭）。
	bool canStagger( BodyPart part ) const;
	void startStagger();      // よろけのアニメへ繋ぐ
	void updateStagger();     // よろけが終わったら歩く・立つのアニメへ戻し、間あけを数える
	// 追う先を考え、向きを変えて歩く（立っていて、よろけていなくて、攻撃していないときだけ）。追いついたら攻撃を始める。
	// 歩く向きには、近くの仲間から離れる向きを混ぜる。自キャラのそばで立ち止まっている間は、重なりそうな仲間から半歩よける。
	void chase( const ChaseTarget* target, const HitWorld& hitWorld, const ZombieCrowd* crowd );
	// point（向かいたい点）へ、近くの仲間から離れる向きを混ぜた「向く点」を返し、混ぜた向きを mSeparation へ覚える。
	// crowd が nullptr なら point のまま。
	GameLib::Math::Vector3 avoidCrowd( const GameLib::Math::Vector3& point, const ZombieCrowd* crowd );
	// 自キャラのそばで立ち止まっている間に、重なりそうな仲間から横へ半歩よける（向きは変えない）。
	void standAside( const ZombieCrowd* crowd );
	// 追いついた相手へ攻撃を始めてよいか（追っている・近い・前に居る・つかんでよい・ほかのゾンビが攻撃に行っていない）。
	bool canStartAttack( const ChaseTarget* target ) const;
	void startAttack();       // 攻撃のアニメへ繋ぐ
	void shakeOff();          // 振りほどかれた。よろけて手を放す
	// 攻撃の間、相手の方を向き、手を伸ばしている間は手が届く所まで寄る。
	void approachDuringAttack( const ChaseTarget* target );
	// 攻撃の段階を1ティック進め、噛んだら首を突き出し、終わったら立つ・歩くのアニメへ戻す（立っている間は毎ティック）。
	void updateAttack( const ChaseTarget* target );
	// 今つかめる所に相手が居るか（つかんでよい・近い・前に居る）。体を引きずっている間は、近いかを頭から測る。
	bool isTargetInReach( const ChaseTarget* target ) const;
	// 相手へ手が届く距離（m。水平だけ）。立っている間は足元から、体を引きずっている間は頭から相手の足元まで。
	float reachDistanceTo( const GameLib::Math::Vector3& target ) const;
	// 体の中心（bodyCenter）から target までの水平の距離（m）と、向きの差（度。-180〜180）。
	float flatDistanceTo( const GameLib::Math::Vector3& target ) const;
	float headingDifferenceTo( const GameLib::Math::Vector3& target ) const;
	// このティックに歩いて進む長さ（m）。歩きアニメの足の動きの表（ZombieModel::stride）から引く。
	float walkDistanceThisTick() const;
	// 体の中心から行き先の方へ1ティックぶん向きを変える（体を引きずっている間はゆっくり）。向きの差が大きいうちは false（まだ歩き出さない）。
	bool turnToward( const GameLib::Math::Vector3& destination );
	// 歩く・立つのアニメを walking に合わせる（変わったときだけ繋ぐ）。
	void setWalking( bool walking );
	// 今の歩く・立つに合ったアニメ（立つアニメが無ければ歩くアニメ）。
	GameLib::Scene::Animation locomotionAnimation() const;
	bool canChase() const;    // 追ってくる体か（歩くアニメがある＝T ポーズの確認用ではない）

	const ZombieModel* mModel; // 借りているだけ
	const StreetGraph* mStreets; // 借りているだけ
	// 使うアニメ（コンストラクタで名前から引いておく）。空なら、そのアニメは使わない（Animations の nullptr と同じ意味）。
	GameLib::Scene::Animation mMoveAnimation;
	GameLib::Scene::Animation mIdleAnimation;
	GameLib::Scene::Animation mDeathAnimation;
	GameLib::Scene::Animation mDownedAnimation;
	GameLib::Scene::Animation mStaggerAnimation;
	GameLib::Scene::Animation mAttackAnimation;
	// 描く直前の姿勢の書き換え。引きずる腕 → のけぞり → 失った部位を潰す、の順に掛ける
	// （撃たれた腕は引きずる形から押され、のけぞった肩の位置で切り口を作るため）。
	// mMesh が借りるので、mMesh より先に宣言する。
	ZombieCrawl mCrawl;
	ZombieFlinch mFlinch;
	ZombieDismemberment mDismemberment;
	BonePoseModifierChain mPoseModifiers;
	SkinnedMeshInstance mMesh;
	HitboxSet mHitboxes;
	ZombieHealth mHealth;
	bool mStaggering;             // よろけのアニメの最中か
	int mStaggerCooldownTicks;    // 次のよろけを出せるまでの残り（ティック。0 なら出せる）
	GameLib::Math::Vector3 mPosition; // 足元の位置（沈んでも変えない。沈む分は見た目の位置だけ下げる）
	float mAngleY;                    // 向き（度）
	ZombieChase mChase;               // どこへ行きたいか
	ZombiePath mPath;                 // 行き先へまっすぐ通れないときの通り道
	ZombieAttack mAttack;             // つかんで噛む攻撃の段階
	CharacterMover mMover;            // 壁・他のキャラに沿って滑る移動（自分を当たり判定として読む）
	GameLib::Math::Vector3 mSeparation; // 最後に歩く向きへ混ぜた、仲間から離れる向き（確認用の表示）
	bool mWalking;                    // 歩くアニメを再生しているか（false なら立つアニメ）
	float mMoveStartTime;             // 歩き・立つのアニメを最初に始める時刻（体ごとにずらした値）
	State mState;
	int mStateTicks;                  // 今の状態に入ってからのティック数（Corpse・Sinking で使う）
	// ── 体を引きずっている間だけ使う ──
	GameLib::Math::Vector3 mCrawlCenter;      // 胸の位置（ワールド。動かすのはこちらで、足元 mPosition はここから決める）
	GameLib::Math::Vector3 mCrawlChestOffset; // 足元から胸まで（骨格の根の空間。水平だけ。倒れ終わった姿勢で決める）
	float mCrawlYawOffset;                    // 見た目の向きから進む向き（腰 → 首）までの角度（度）
	float mAttackClock;                       // 攻撃のアニメの代わりに進める時刻（フレーム）
	std::vector< BodyPart > mSeveredByLastDamage; // 最後の applyDamage で切れた部位
};

#endif
