#ifndef INCLUDED_ANIMATION_POSE_SAMPLER_H
#define INCLUDED_ANIMATION_POSE_SAMPLER_H

#include "GameLib/Scene/Tree.h"
#include "GameLib/Scene/Animation.h"
#include "GameLib/Math/Matrix34.h"
#include <vector>

class SkinnedMeshData;

// 1つのアニメの、好きな時刻のポーズを求める（見せているアニメとは別に、自分の骨格を1つ持つ）。
// 見せているポーズ（SkinnedMeshInstance）には触らない。求めたポーズをどう使うか（腕だけ重ねるなど）は使う側が決める。
// 例: 歩いている脚はそのままに、腕だけリロードのアニメにする（FirstPersonBody::setArmOverride へ渡す）。
class AnimationPoseSampler{
public:
	// data: 骨格とアニメを引く。所有権は受け取らない。nullptr 不可、isReady() 済みであること。この係より長く生きていること（MeshCache が持つ）。
	// animationName: アニメの名前。見つからなければデバッグ出力で知らせ、isReady が false になる（sample は何もしない）。
	AnimationPoseSampler( const SkinnedMeshData* data, const char* animationName );

	bool isReady() const; // アニメが見つかったか
	// アニメ時刻 frame（フレーム）のポーズを pose へ書く（骨格の根から見たボーンの姿勢。SkinnedMeshData::computeBonePose の並び）。
	// pose は骨の数に揃える（空の vector を渡してよい）。アニメが無ければ false（pose は変えない）。pose: nullptr 不可。
	bool sample( float frame, std::vector< GameLib::Math::Matrix34 >* pose );

private:
	const SkinnedMeshData* mData;
	GameLib::Scene::Tree mSkeleton;
	GameLib::Scene::Animation mAnimation;
};

#endif
