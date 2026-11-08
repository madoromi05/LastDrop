#ifndef INCLUDED_ARM_POSE_COMPONENT_H
#define INCLUDED_ARM_POSE_COMPONENT_H

#include "GameLib/Math/Matrix34.h"
#include "GameSrc/World/Component.h"
#include "GameSrc/Player/Animator.h"
#include "GameSrc/Render/SkinnedMesh/AnimationPoseSampler.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

class SkinnedMeshData;
class GunComponent;
class HoldComponent;
class FirstPersonBodyComponent;

// 銃を持つ腕のポーズの部品（SummaryPlan/個別仕様/HighPrioritySplit_Plan.txt 4-1。2026-11-09 HoldComponent から分けた）。
// 持っている銃（HoldComponent）と構えから、腕の効き具合を決めて一人称の体（FirstPersonBodyComponent）へ渡す：
//   リロードで腕をアニメへ差し替える割合・両手持ち・走って胸元に抱える寄せ具合・覗き具合。
//   腕のポーズを持つ銃（拳銃）は、構え（覗いていない間・覗いた）のアニメの 1 フレーム目を覚え、いつも腕をそれに差し替える。
// 銃が替わったのは自分で気付く（update のたびに持っている銃を見て、前と違えば構えのポーズを取り直す）。
// だから HoldComponent はこの部品を知らない（依存は ArmPose → Hold の一方通行）。
// 外してよい部品（外すと、腕はいつも体のアニメのまま。HoldComponent も外していれば何もしない）。
class ArmPoseComponent : public Component{
public:
	ArmPoseComponent();

	// 腕の効き具合を決めて体へ渡す（PlayerComponent が、構え・リロードを決めた後に毎ティック呼ぶ）。
	// next: このティックのアニメの状態（走っているなら胸元へ寄せる）。
	void update( Animator::State next );
	float carryWeight() const; // 走って胸元に抱えている割合（0〜1）

	// Component（SkinnedModelComponent・FirstPersonBodyComponent が要る。HoldComponent は外せるので使う所で引く）
	// awake：持つ側の形を借りる。start：体を借り、構えの段階から両手持ちの効き具合を始める
	void awake() override;
	void start() override;

private:
	// 持っている銃 gun（素手なら nullptr）の構えのポーズを取り直す。handBone: 銃を持つ手の骨。
	void sampleStancePoses( const GunComponent* gun, int handBone );
	// 腕を差し替えるアニメ animation のポーズを求める物（名前ごとに 1 つ作って使い回す）。awake の後に呼ぶ。
	AnimationPoseSampler* armSampler( const char* animation );
	// from と to をボーンごとに weight の割合で混ぜて out へ書く（位置は直線・向きは球面。MatrixBlend）。from と to は同じ骨の数。
	static void blendPoses( const std::vector< GameLib::Math::Matrix34 >& from, const std::vector< GameLib::Math::Matrix34 >& to,
		float weight, std::vector< GameLib::Math::Matrix34 >* out );

	const SkinnedMeshData* mHolderData;
	FirstPersonBodyComponent* mBody;
	const GunComponent* mPosedGun; // 構えのポーズを取った銃（替わったら取り直す。中身は読まず、見分けるだけ）
	int mPosedHandBone;            // そのときの銃を持つ手の骨
	// 腕を差し替えるアニメ（リロード・拳銃の構え）のポーズを求める物。名前ごと（形を借りるので awake の後に作る）。
	std::map< std::string, std::unique_ptr< AnimationPoseSampler > > mArmSamplers;
	std::vector< GameLib::Math::Matrix34 > mHoldPose; // 腕のポーズを持つ銃（拳銃）の構え（持ち替えたときに求める）
	bool mHasHoldPose;                                 // mHoldPose を使うか（持っている銃に腕のポーズがあり、求められた）
	std::vector< GameLib::Math::Matrix34 > mReadyPose; // 覗いていない間の構え（WeaponSpec::readyAnimation。持ち替えたときに求める）
	bool mHasReadyPose;                                 // mReadyPose を使うか
	std::vector< GameLib::Math::Matrix34 > mReadyBodyPose; // mReadyPose を作ったときの体のアニメ（WeaponSpec::readyBodyAnimation）のポーズ
	bool mHasReadyBodyPose;                              // mReadyBodyPose を使うか（右手を、腕を寄せる基準にする）
	std::vector< GameLib::Math::Matrix34 > mArmPose;  // 覗いていない間の構えと構えを混ぜた腕のポーズ（FirstPersonBody が次の姿勢の計算まで借りる）
	std::vector< GameLib::Math::Matrix34 > mReloadPose; // 求めたポーズ（FirstPersonBody が次の姿勢の計算まで借りる）
	float mHoldWeight;  // 両手持ちの効き具合（FirstPersonBody::setHoldWeight へ渡す）
	float mCarryWeight; // 胸元への寄せ具合（FirstPersonBody::setCarryWeight へ渡す）
};

#endif
