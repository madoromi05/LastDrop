#ifndef INCLUDED_ZOMBIE_H
#define INCLUDED_ZOMBIE_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshInstance.h"
#include "GameSrc/Collision/ICollisionBody.h"
#include "GameSrc/Hit/IHittable.h"
#include "GameSrc/Hit/IDamageable.h"
#include "GameSrc/World/IWorldMember.h"
#include "GameSrc/World/Component.h"
#include "GameSrc/Hit/HitboxSet.h"
#include "GameSrc/Zombie/ZombieHealth.h"
#include "GameSrc/Zombie/ZombieFlinch.h"
#include "GameSrc/Zombie/ZombieDismemberment.h"
#include "GameSrc/Zombie/ZombieSway.h"
#include "GameSrc/Render/SkinnedMesh/BonePoseModifierChain.h"
#include "GameSrc/Zombie/ZombieLocomotion.h"
#include "GameSrc/Zombie/ZombieAttack.h"
#include "GameSrc/Zombie/ZombieAttackReach.h"
#include "GameSrc/Zombie/ZombieRootMotionSpan.h"
#include "GameSrc/Zombie/ZombieAnimationSet.h"
#include "GameSrc/Zombie/ZombieStagger.h"
#include "GameSrc/Zombie/ZombieCrawler.h"
#include "GameSrc/Zombie/ZombieCorpse.h"
#include "GameSrc/Collision/CharacterMover.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Math/Random.h"
#include <vector>

class ZombieModel;
class CollisionWorld;
class HitWorld;
class NavMesh;
class ZombieCrowd;
struct ChaseTarget;

// ゾンビ1体。立っている間は自キャラに気付くと歩いて追ってくる。どこへ行き・どちらを向いて・どう進むかは ZombieLocomotion
// （中で ZombieChase が行き先を決め、ZombiePath が建物を回り込む通り道を出す。SummaryPlan/個別仕様/ZombieChase_Plan.txt・ZombiePath_Plan.txt）。
// ここは「考えた後、歩く前」に攻撃を始めるかを見て、歩く・立つのアニメを切り替える（SummaryPlan/個別仕様/ZombieSplit2_Plan.txt）。
// 気付いていない・追いついた・見失って探し終えたときは、立つアニメで立ち止まる。
// 形と、当たり判定の大きさ・部位の箱の設計図は ZombieModel を借りる（同じ種類の全員で共有。1体ごとには求めない）。
// 見た目は SkinnedMeshInstance に任せ、体を真上から囲む長方形（BodyFootprint）を差し出して、他のキャラの移動を止める（ICollisionBody）。
// 部位ごとの当たり判定（命中判定用）も持ち、弾がどの部位に当たったかを答えられる（IHittable）。
// 撃たれると体力（ZombieHealth）が減り、倒れる条件を満たすと死亡アニメで倒れ、しばらく死体として残ってから地面へ沈んで消える（IDamageable）。
// 死体として残る・沈む時間は ZombieCorpse が数える。
// 体力の最大は体ごとに違う（作るときに丈夫さを選ぶ。撃ってみないと何発で倒れるか分からない。SummaryPlan/個別仕様/ZombieUncertainty_Plan.txt）。
// 頭に当たると、確率で頭が弾けて倒れる（クリティカル。確率は ZombieHealth の表、乱数を引くのはここ）。
// 自キャラを追って近づくと、胴を左右に揺らしながら（ZombieSway）、揺れた側へ少し流れて歩く。揺れには部位の箱も付いてくる（頭を狙いにくい）。
// 生きている間に撃たれると、1 発ごとに当たった部位がのけぞり（ZombieFlinch）、ときどきよろける（よろけのアニメを1回再生して歩きへ戻る）。
// よろけの最中か・次のよろけまで間をあけるか（頭に当たったときは待たない）は ZombieStagger が数える
// （SummaryPlan/個別仕様/ZombieStagger_Plan.txt）。ここはよろけを始め・終える時を決めるだけ（アニメは ZombieAnimationSet）。
// 頭・腕・脚の体力が 0 になると、その部位を切る（ZombieDismemberment が見た目から消し、ここが部位の箱を外す）。
// 千切れた部位を飛ばすのは持ち主（severedByLastDamage と shownWorldPose を読んで作る）。
// 脚を失うと前へ倒れるアニメで倒れ、倒れたまま生きている（Downed。SummaryPlan/個別仕様/ZombieDismember_Plan.txt）。
// 倒れ終わったら、腕で体を引きずって追ってくる（腕の動き・胸の置き場所は ZombieCrawler。這うアニメが無いので、倒れた姿勢のまま腕の骨を曲げる。
// SummaryPlan/個別仕様/ZombieCrawl_Plan.txt）。追い方は立っている時と同じで、向きを変える中心・見る高さ・壁に当たる所は胸。
// 頭が相手の足元に届いたら、腕を伸ばして足を引っかく（攻撃のアニメの代わりに時計で ZombieAttack を進める）。
// 追いついたら、腕で引っかく攻撃をする（段階は ZombieAttack が決め、ここが攻撃を始め（アニメは ZombieAnimationSet）、向きを変え、寄る（間合いは ZombieAttackReach）。
// SummaryPlan/個別仕様/ZombieAttack_Plan.txt）。相手に何が起きるか（動けない・体力が減る）は知らない。
// 「このティックに腕が当たったか」を答えるだけで、相手へ伝えるのは持ち主（GameWorld）。
// 攻撃の途中は、撃たれてもよろけない（のけぞりだけ）。
// 攻撃・よろけ・倒れるアニメは腰が前後へ動く（攻撃で踏み込む・倒れて体がずれる）ので、その動きを置き場所へ移す（ルートモーション。
// 表は ZombieRootMotion、区間を進めるのは ZombieRootMotionSpan。SummaryPlan/個別仕様/ZombieRootMotion_Plan.txt）。
// いつ区間を始め・閉じるかはここが、どのアニメにどの表と通り抜けの決まりを組むかは ZombieAnimationSet が決める（SummaryPlan/個別仕様/ZombieSplit3_Plan.txt）。
// 名簿から外す・消すのは持ち主（GameWorld）。ここは「もう移動を止めないか（blocksMovement）」「もう消えてよいか（isGone）」を答えるだけ。
// 物（GameObject。タグ Zombie）の部品として GameObjectWorld が持つ（SummaryPlan/個別仕様/ComponentSystem_Plan.txt。中身は分けずに 1 つの部品）。
class Zombie : public Component, public ICollisionBody, public IHittable, public IDamageable, public IWorldMember{
public:
	// 生きているか・倒れているか。並びは時間の順。
	enum class State{
		Alive,   // 立っている（撃たれると体力が減る）
		Downed,  // 脚を失って倒れ、倒れたまま生きている（撃たれると体力が減る。よろけない）
		Falling, // 死亡アニメの途中
		Corpse,  // 倒れ終わって、死体として残っている
		Sinking, // 地面へ沈んでいる（沈み終わったら消えてよい）
	};

	// 使うアニメの名前（文字列は持ち主の定数。コンストラクタの中で引くだけで、覚えない）。中身の説明は ZombieAnimationNames。
	typedef ZombieAnimationNames Animations;

	// model: 形・見た目・アニメと、当たり判定の設計図。所有権は受け取らない。nullptr 不可。
	//        このゾンビより長く生きていること（持ち主の GameWorld が、ゾンビより後に破棄する）。
	// collisionWorld: 歩くときに壁・他のキャラを聞く名簿。所有権は受け取らない。nullptr 不可。このゾンビより長く生きていること。
	// navMesh: 建物を回り込む通り道を聞く町の床の形。所有権は受け取らない。nullptr 不可。寿命の決まりは collisionWorld と同じ。
	// position / angleY: 足元の位置と向き（度。0 は +Z）。
	// sightPhase: 見通し・まっすぐ通れるかを調べる時刻をずらす番号（何体目か。ZombieChase・ZombiePath）。
	// randomSeed: 体ごとの乱数の種（体ごとに違う値）。丈夫さを選び、うろつく先・立つ長さを選ぶ乱数（ZombieChase）の種もここから引く。
	Zombie( const ZombieModel* model, const CollisionWorld* collisionWorld, const NavMesh* navMesh,
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
	// 足元の位置（ワールド）。攻撃・よろけ・倒れる間は腰に付いて動く（倒れた後は死体の腰の下）。
	const GameLib::Math::Vector3& position() const;
	// 生きているか（Alive か Downed）。
	bool isAlive() const;
	// 脚を失って倒れ終わり、腕で体を引きずっているか（Downed の間だけ true になりうる）。攻撃の届く距離を頭から測るのに使う。
	bool isCrawling() const;
	// 体の中心（ワールド）。立っている間は足元、体を引きずっている間は胸。追う・回り込む・壁に当たるのはこの位置。
	// 攻撃・よろけ・倒れるアニメの間は、始めた所（根の動きのアンカー。腕が届く距離もここから測る）。
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
	// 腕を振っている間か。持ち主は、この間ほかのゾンビに攻撃を始めさせない（ChaseTarget::attackTaken）。
	bool isAttacking() const;
	bool hitTargetThisTick() const; // 直前の update で相手に腕が当たったか（ダメージの量は ZombieAttack::hitDamage）
	// 頭の位置（ワールド。頭の部位の箱の中心）。攻撃の音を鳴らす所。頭を失っていれば目の高さ。
	GameLib::Math::Vector3 headPosition() const;
	const ZombieHealth& health() const;
	// 他のキャラの移動を止めるか（立っている間だけ。Downed も止めない＝立ち姿の長方形が倒れた体と合わないため）。
	// false になったら、持ち主が移動の名簿から外す（死体・倒れたゾンビの上を歩ける）。
	bool blocksMovement() const;
	// 沈み終わって、消してよいか（持ち主が付いている物に GameObjectWorld::destroyLater する）。
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
	// 倒れ終わった姿勢の腰 → 首を進む向きに、胸を体の中心にして腕を動かし始め（ZombieCrawler。見た目は動かさない）、通り道を捨てる。
	void startCrawling();
	// 追う先を考えて向きを変え（ZombieLocomotion）、追いついたら攻撃を始める（攻撃していないとき）。
	// 腕の往復の速さの段階を返し、前へ進むなら *moving を true にする。
	ZombieCrawler::Effort crawlChase( const ChaseTarget* target, const HitWorld& hitWorld, bool* moving );
	// part が 0 になっていて、まだあれば切る（見た目から消して切れた部位を覚えるのは ZombieDismemberment、部位の箱を外すのはここ）。
	void severIfBroken( BodyPart part );
	void updateAfterDeath();  // Falling → Corpse → Sinking と進め、沈む分だけ見た目を下げる
	// 今当たった part でよろけを始めてよいか（よろけていない・間あけが済んだ、または頭）。
	bool canStagger( BodyPart part ) const;
	void startStagger();      // よろけのアニメへ繋ぐ
	void updateStagger();     // よろけが終わったら歩く・立つのアニメへ戻し、間あけを数える
	// 追う先を考えて向きを変え（ZombieLocomotion）、追いついたら攻撃を始め、そうでなければ歩く・横へよける
	// （立っていて、よろけていなくて、攻撃していないときだけ）。
	void chase( const ChaseTarget* target, const HitWorld& hitWorld, const ZombieCrowd* crowd );
	// ── フラつき（SummaryPlan/個別仕様/ZombieUncertainty_Plan.txt の 6 章）──
	// 今揺らしたい強さ（0〜1）。立って自キャラを追っていて（よろけ・攻撃の最中でない）、近いほど強い。それ以外は 0。
	float swayStrength( const ChaseTarget* target ) const;
	// 追いついた相手へ攻撃を始めてよいか（追っている・近い・前に居る・攻撃してよい・ほかのゾンビが攻撃に行っていない）。
	bool canStartAttack( const ChaseTarget* target ) const;
	void startAttack();       // 攻撃のアニメへ繋ぐ
	// 攻撃の間、相手の方を向き、腕を振っている間は手が届く所まで寄る。
	void approachDuringAttack( const ChaseTarget* target );
	// 攻撃の段階を1ティック進め、終わったら立つ・歩くのアニメへ戻す（立っている間は毎ティック）。
	void updateAttack( const ChaseTarget* target );
	// 今腕が届く所に相手が居るか（攻撃してよい・近い・前に居る）。体を引きずっている間は、近いかを頭から測る。
	bool isTargetInReach( const ChaseTarget* target ) const;
	// 攻撃の間合いを測る、今の体の形（ZombieAttackReach に渡す）。
	ZombieAttackReach::Origin attackOrigin() const;
	// 体の中心（bodyCenter）から target までの水平の距離（m）。
	float flatDistanceTo( const GameLib::Math::Vector3& target ) const;
	// 向きを変える速さの種類（体を引きずっている間は Crawling。ZombieLocomotion::turnToward に渡す）。
	ZombieLocomotion::Posture posture() const;
	bool canChase() const;    // 追ってくる体か（歩くアニメがある＝T ポーズの確認用ではない）

	const ZombieModel* mModel; // 借りているだけ
	// 体ごとの乱数（丈夫さ・揺れの速さを選ぶ・頭のクリティカルを引く）。mSway・mHealth・mLocomotion の初期化に使うので、それらより先に宣言する。
	GameLib::Math::Random mRandom;
	// 描く直前の姿勢の書き換え。引きずる腕（mCrawler の中）→ のけぞり → 失った部位を潰す、の順に掛ける
	// （撃たれた腕は引きずる形から押され、のけぞった肩の位置で切り口を作るため）。掛ける順は mPoseModifiers へ足す順で決まる。
	// mMesh が借りるので、mMesh より先に宣言する。mCrawler は mDismemberment を借りる（腕が残っているか）ので、それより後に宣言する。
	ZombieDismemberment mDismemberment;
	// 脚を失って倒れた後に、腕で体を引きずる体（腕の骨・胸の置き場所・腕の速さの段階）。
	ZombieCrawler mCrawler;
	ZombieFlinch mFlinch;
	BonePoseModifierChain mPoseModifiers;
	// フラつき。見た目だけの書き換え（上の 3 つ）より先に、部位の箱も動く「体の書き換え」として掛ける（setBodyPoseModifier）。
	ZombieSway mSway;
	SkinnedMeshInstance mMesh;
	HitboxSet mHitboxes;
	ZombieHealth mHealth;
	ZombieStagger mStagger;           // よろけの最中か・次のよろけまでの間あけ
	GameLib::Math::Vector3 mPosition; // 足元の位置（沈んでも変えない。沈む分は見た目の位置だけ下げる）
	float mAngleY;                    // 向き（度）
	ZombieAttack mAttack;             // 腕で引っかく攻撃の段階
	CharacterMover mMover;            // 壁・他のキャラに沿って滑る移動（自分を当たり判定として読む）
	// 攻撃・よろけ・倒れる間の根の動きの区間。mMesh・mMover を借りるので、それらより後に宣言する。
	ZombieRootMotionSpan mRootSpan;
	// 使うアニメと、どのアニメをどう繋ぎ・どの根の動きで流すか。mMesh・mRootSpan を借りるので、それらより後に宣言する。
	ZombieAnimationSet mAnimations;
	// どこへ行き・どちらを向いて・どう進むか（行き先の AI と通り道も持つ）。mMover を借りるので、それより後に宣言する。
	ZombieLocomotion mLocomotion;
	State mState;
	ZombieCorpse mCorpse;             // 倒れ終わってから、死体として残り沈んで消えるまでの時間
};

#endif
