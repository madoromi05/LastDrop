#ifndef INCLUDED_EFFECT_MANAGER_H
#define INCLUDED_EFFECT_MANAGER_H

#include "GameSrc/Weapon/Bullet/BulletHoleList.h"
#include "GameSrc/Effect/ImpactDustList.h"
#include "GameSrc/Effect/BloodSprayList.h"
#include "GameSrc/Effect/BloodStainList.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix44.h"

class IRayBlocker;

// 弾が当たった後に出る、見せるだけの物（弾痕・砂ぼこり・血しぶき・血の跡）の持ち主
// （SummaryPlan/個別仕様/設計と分割/GameWorldSplit_Plan.txt 2-3）
// 頼む側（CombatResolver）は「ここに出して」と言うだけで、いつ消えるか・どれだけ出すか（頭なら多く）はここが決める
// どれも GameObject にしない（数が多く、まとめて描く・詰めて持つ方が速いため）
// 千切れた部位は数が少なく、1 つずつ寿命を持つので、ここではなく物にする（SeveredLimbComponent）
// 当たり判定の名簿には載らない
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
	// ゾンビの部位が千切れた切り口 jointPosition に、血しぶきを出す（当たったときより大きく、一番多く）
	// direction: 弾の進む向き（長さ1）。千切れた部位そのものは、頼む側が物として出す
	void spawnSeverBlood( const GameLib::Math::Vector3& jointPosition, const GameLib::Math::Vector3& direction );
	void detach( const IRayBlocker* blocker );	// blocker に付いた弾痕・血の跡を消す。物が消える前（destroyLater で印を付けるとき）に呼ぶ

	void update();								// 1ティック進める（古くする・落とす・消す）

	// 弾痕と血の跡（弾痕の上に乗る）を G-Buffer へデカールとして描く。GBuffer::beginDecals〜endDecals の間で、1フレームに1回まで
	// projectionView: カメラの射影×ビュー行列
	void drawDecals( const GameLib::Math::Matrix44& projectionView );
	// 砂ぼこり・血しぶきの板を描く（半透明。ライティングの後）。viewDirection: 視線の向き（長さ1。板を画面に平行にし、カメラ側から照らす）
	// eye: 目の位置（粒を目から遠い順に並べ替えて描くため）
	void drawImpactDust( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& viewDirection,
		const GameLib::Math::Vector3& eye );
	void drawBloodSpray( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& viewDirection,
		const GameLib::Math::Vector3& eye );

private:
	BulletHoleList mBulletHoles;						// 弾が物に当たった跡
	ImpactDustList mImpactDust;							// 弾が物に当たった瞬間の砂ぼこり（遠くの着弾を見えるようにする）
	BloodSprayList mBloodSpray;							// 弾がゾンビに当たった瞬間・部位が切れた瞬間の血しぶき
	BloodStainList mBloodStains;						// 撃たれたゾンビの後ろの壁・足元に付いた血の跡
};

#endif
