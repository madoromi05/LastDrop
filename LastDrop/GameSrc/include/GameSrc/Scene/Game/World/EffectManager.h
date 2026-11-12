#ifndef INCLUDED_EFFECT_MANAGER_H
#define INCLUDED_EFFECT_MANAGER_H

#include "GameSrc/Weapon/Bullet/BulletHoleList.h"
#include "GameSrc/Effect/ImpactDustList.h"
#include "GameSrc/Effect/BloodSprayList.h"
#include "GameSrc/Effect/BloodStainList.h"
#include "GameSrc/Zombie/Damage/SeveredLimbList.h"
#include "GameSrc/Hit/BodyPart.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Math/Matrix44.h"
#include <vector>

class IRayBlocker;
class SkinnedMeshInstance;
class ZombieModel;
class DrawCulling;

// 弾が当たった後に出る、見せるだけの物（弾痕・砂ぼこり・血しぶき・血の跡・千切れた部位）の持ち主
// （SummaryPlan/個別仕様/設計と分割/GameWorldSplit_Plan.txt 2-3）
// 頼む側（CombatResolver）は「ここに出して」と言うだけで、いつ消えるか・どれだけ出すか（頭なら多く）はここが決める
// どれも GameObject にしない（数が多く、まとめて描く・詰めて持つ方が速いため）
// 当たり判定の名簿には載らない（千切れた部位が地面へ落ちるときに、地面を聞くだけ）
class EffectManager{
public:
	// 血の跡の大きさ。Large は頭に当たったとき・部位が千切れたとき（一番多く血が出る所）
	enum class BloodStainSize{
		Normal,
		Large,
	};
	// 弾が当たったときの血しぶきの大きさ（HitFeel_Plan.txt。どれにするかは CombatResolver が命中の重さで決める）
	// 弾が当たった音を 1 つにしたので、頭の手応えは血の量で出す
	enum class BloodHitSize{
		Body,  // 胴・腕・脚（ふつう）
		Head,  // 頭に当たったが倒れていない（1 回に出せる上限まで多く）
		Heavy, // 部位が飛んだ・倒した
		Burst, // 頭が弾けた（上限の血しぶきを 3 方向へ）
	};

	EffectManager();
	EffectManager( const EffectManager& ) = delete;
	EffectManager& operator=( const EffectManager& ) = delete;

	// 弾が物（建物・車・地面・拾える物）に当たった所に、弾痕と砂ぼこりを出す
	// blocker: 当たった物。物が消えるとき detach で弾痕を一緒に消すための目印で、中身は読まない。nullptr 不可
	void spawnSurfaceImpact( const GameLib::Math::Vector3& point, const GameLib::Math::Vector3& normal, const IRayBlocker* blocker );
	// 弾がゾンビ（死体も）に当たった所に血しぶきを出す。size で量と大きさを変える（頭は多く大きく）
	// direction: 弾の進む向き（長さ1）
	void spawnBloodHit( const GameLib::Math::Vector3& point, const GameLib::Math::Vector3& direction, BloodHitSize size );
	// 撃たれたゾンビの後ろの物（建物・車・地面・拾える物）の面に、飛び散った血の跡を付ける
	// point / normal / blocker: 付ける面（HitWorld::Hit の値）。direction: 弾の進む向き（長さ1。筋をこの向きへ伸ばす）
	void spawnBloodSplatter( const GameLib::Math::Vector3& point, const GameLib::Math::Vector3& normal,
		const GameLib::Math::Vector3& direction, const IRayBlocker* blocker, BloodStainSize size );
	// 撃たれたゾンビの足元の面に、落ちた滴の跡を付ける。引数は spawnBloodSplatter と同じ
	void spawnBloodDrip( const GameLib::Math::Vector3& point, const GameLib::Math::Vector3& normal, const IRayBlocker* blocker,
		BloodStainSize size );
	// zombieMesh（model の形のゾンビの見た目）の part が今切れたので、切り口に血しぶきを出し、千切れた部位を飛ばす
	// 切れた部位は applyDamage の直後ならまだ見えている（体から消えるのは次の update）ので、その姿勢を写す
	// model: 切り口の決まりと形を読む。借りるだけ。千切れた部位が消えるまで生きていること（GameWorld が持つ）
	void spawnSeveredLimb( const SkinnedMeshInstance& zombieMesh, const ZombieModel& model, BodyPart part,
		const GameLib::Math::Vector3& direction );
	// blocker に付いた弾痕・血の跡を消す。物が消える前（destroyLater で印を付けるとき）に呼ぶ
	void detach( const IRayBlocker* blocker );

	// 1ティック進める（古くする・落とす・消す）。ground: 千切れた部位が落ちる先（建物・車の上には乗らず、抜ける）。nullptr 不可
	void update( const IRayBlocker* ground );

	// 弾痕と血の跡（弾痕の上に乗る）を G-Buffer へデカールとして描く。GBuffer::beginDecals〜endDecals の間で、1フレームに1回まで
	// projectionView: カメラの射影×ビュー行列
	void drawDecals( const GameLib::Math::Matrix44& projectionView );
	// 砂ぼこり・血しぶきの板を描く（半透明。ライティングの後）。viewDirection: 視線の向き（長さ1。板を画面に平行にし、カメラ側から照らす）
	// eye: 目の位置（粒を目から遠い順に並べ替えて描くため）
	void drawImpactDust( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& viewDirection,
		const GameLib::Math::Vector3& eye );
	void drawBloodSpray( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& viewDirection,
		const GameLib::Math::Vector3& eye );
	// 千切れた部位を描く（不透明。G-Buffer へ・影のマップへ）。culling: 描くかどうか
	void drawSeveredLimbs( const DrawCulling& culling ) const;

private:
	BulletHoleList mBulletHoles;   // 弾が物に当たった跡
	ImpactDustList mImpactDust;    // 弾が物に当たった瞬間の砂ぼこり（遠くの着弾を見えるようにする）
	BloodSprayList mBloodSpray;    // 弾がゾンビに当たった瞬間・部位が切れた瞬間の血しぶき
	BloodStainList mBloodStains;   // 撃たれたゾンビの後ろの壁・足元に付いた血の跡
	SeveredLimbList mSeveredLimbs; // ゾンビから千切れた部位
	// 切れた瞬間の骨の姿勢（spawnSeveredLimb の作業領域。毎回確保し直さないために持つ）
	std::vector< GameLib::Math::Matrix34 > mSeverPose;
};

#endif
