#ifndef INCLUDED_SKINNED_MESH_INSTANCE_H
#define INCLUDED_SKINNED_MESH_INSTANCE_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Scene/Animation.h"
#include "GameSrc/Render/SkinnedMesh/AnimationPlayer.h"
#include <vector>

class SkinnedMeshData;
class BonePoseModifier;

// スキンメッシュを置いた「1体」。自分の骨格（ポーズ）・変形後の頂点・位置と向きを持つ
// 頂点・重み・テクスチャ・アニメは SkinnedMeshData を借りる（所有しない）
// 同じキャラを何体出すときも、データは1つでインスタンスだけ増やせばよい
//
// アニメの再生（時刻・クロスフェード・ポーズを求める）は AnimationPlayer（MidPrioritySplit_Plan.txt 6 章）
// ここはアニメの口を残して中で渡し、そのポーズに書き換えを掛けて、置き場所へ置いて描く
// アニメの切り替えは2通り。setAnimation は即座に切り替え、crossFadeTo は切り替え直前に見えていたポーズから
// 新しいアニメへ数フレームかけて混ぜる（クロスフェード）
//
// 頂点の変形は GPU が行う（SkinnedMeshData::draw）。1体ごとに持つのは骨格とポーズだけで、頂点の数には比例しない
// コピーすると骨格を2体で共有してしまうので、コピーは禁止（std::unique_ptr 等で持つ）
class SkinnedMeshInstance{
public:
	// アニメの再生のしかた（中身は AnimationPlayer::PlayMode。SkinnedMeshInstance::PLAY_ONCE の形で書けるよう名前を残す）
	typedef AnimationPlayer::PlayMode PlayMode;
	static const PlayMode PLAY_LOOP = AnimationPlayer::PLAY_LOOP; // 最後まで来たら始めへ戻る（歩き・待機）
	static const PlayMode PLAY_ONCE = AnimationPlayer::PLAY_ONCE; // 最後のフレームで止まる（倒れる）

	// data: 形と見た目。所有権は受け取らない。nullptr 不可、isReady() 済みであること
	//       このインスタンスより長く生きていること（MeshCache が持つ）
	explicit SkinnedMeshInstance( const SkinnedMeshData* data );
	SkinnedMeshInstance( const SkinnedMeshInstance& ) = delete;
	SkinnedMeshInstance& operator=( const SkinnedMeshInstance& ) = delete;

	void setPosition( const GameLib::Math::Vector3& );											// 置き場所（ワールド座標）
	void setAngleY( float degree );																// Y軸まわりの向き（度）
	// 再生するアニメを名前で設定する。見つからなければデバッグ出力してレスト姿勢のまま
	// startTime: 再生開始時刻（フレーム）。同じアニメの複数体をずらして、動きが揃わないようにする
	// mode: 再生のしかた。省略するとループ
	void setAnimation( const char* name, float startTime, PlayMode mode = PLAY_LOOP );
	void setAnimation( GameLib::Scene::Animation, float startTime, PlayMode mode = PLAY_LOOP );	// 取得済みのアニメで即座に切り替える（混ぜない）。空の Animation ならレスト姿勢になる
	// 今見えているポーズから、animation へ fadeFrames フレームかけて滑らかに切り替える
	// フェード中に呼び直してもよい（その時点で見えている混ざったポーズから、改めて混ぜ始める）
	// startTime: 新しいアニメの再生開始時刻（フレーム）。歩き⇔走りで足の位相を合わせるときに使う
	void crossFadeTo( GameLib::Scene::Animation animation, float startTime, float fadeFrames, PlayMode mode = PLAY_LOOP );
	// 再生の速さ（1 で 1 ティックに 1 フレーム。0.5 なら半分の速さ）。アニメを切り替える（setAnimation・crossFadeTo）と 1 に戻る
	// アニメの一部だけをゆっくり見せたいときに、そのアニメの間だけ変える。0 より大きいこと
	void setPlaybackRate( float rate );
	float animationTime() const;																// 今見えているアニメの時刻（フレーム。最後の update で使った時刻）。アニメの何フレーム目で何かを起こすときに読む
	float animationPhase() const;																// 再生中のアニメが1周のどこまで進んだか（0以上1未満）。アニメが無い・長さ0なら0
	bool isAnimationFinished() const;															// PLAY_ONCE のアニメが最後のフレームまで来て、止まっているか。PLAY_LOOP なら常に false
	// 描く直前にポーズを書き換える処理（一人称で頭を消す・視点に合わせて上半身を曲げる等）
	// 所有権は受け取らない。nullptr で外す。このインスタンスより長く生きていること
	void setPoseModifier( BonePoseModifier* );
	// 体の位置そのものを動かす書き換え（ゾンビのフラつき）。setPoseModifier より先に掛かり、
	// 見た目だけでなく animatedBoneMatrix（部位の当たり判定が付く姿勢）にも入る
	// 見た目だけの書き換え（のけぞり等）と分けるのは、狙いにくさを作る揺れは当たり判定も動かさないと意味が無いため
	// 所有権は受け取らない。nullptr で外す（無ければアニメのポーズのまま）。このインスタンスより長く生きていること
	void setBodyPoseModifier( BonePoseModifier* );

	void update();	// アニメを1フレーム（1ティック）進めて、ポーズを求める
	// アニメの時刻とクロスフェードの進みだけを 1 ティック進め、ポーズは求めない（見えるポーズは前の update のまま）
	// 遠い・映らない体のアニメを間引くティックに、update の代わりに呼ぶ（AnimationThrottle）。時刻は止めないので、
	// 次の update では間引かなかった時と同じ時刻のポーズになる（動きが遅れず、飛ぶだけ）。animationTime も進む
	void advance();
	// 最後の update のポーズで描く（骨の行列を求めて GPU で変形する）。まだ update していなければ何も描かない
	// 骨の行列は、ポーズか置き場所が変わってから最初の draw でだけ作る（同じフレームの 2 回目からは作った物を使う）
	void draw() const;

	// ボーンの今の姿勢をワールド座標の行列で返す（置き場所・向き込み。最後の update で見せたポーズ。書き換え後）
	// 武器など、骨に付けて一緒に動かす物の置き場所に使う
	// boneIndex: SkinnedMeshData::findBone の番号。範囲外、またはまだ update していなければ false（out は変えない）
	// 骨格の単位のまま返す（Mixamo なら cm の縮尺 0.01 が入っている）ので、付ける物の側で合わせる
	bool boneMatrix( int boneIndex, GameLib::Math::Matrix34* out ) const;
	// boneMatrix と同じだが、見た目だけの書き換え（setPoseModifier）を掛ける前の、アニメどおりのポーズで返す
	// （体の書き換え setBodyPoseModifier は掛けた後）
	// 部位の当たり判定に使う（一人称で頭を潰した・視点に合わせて曲げた見た目ではなく、体の本来の位置で判定するため）
	bool animatedBoneMatrix( int boneIndex, GameLib::Math::Matrix34* out ) const;
	// 全部の骨の boneMatrix（見えている姿勢・ワールド）を、骨の番号の順に out へ書く（前の中身は捨てる）
	// まだ update していなければ、全部を根の位置（rootMatrix）に置く。体から千切れた部位を、切れた瞬間の姿勢で写すのに使う
	// out: nullptr 不可
	void shownWorldPose( std::vector< GameLib::Math::Matrix34 >* out ) const;
	GameLib::Math::Matrix34 rootMatrix() const;	// 置き場所と向きの行列。骨格の根から見た位置（ボーンの姿勢・BonePoseModifier が求めた点）をワールドへ置くのに掛ける
	// ワールドの向き（移動を含まない）・点を、骨格の根の空間（置き場所と向きを掛ける前）へ直す（rootMatrix の逆を掛ける）
	// 弾の向きでのけぞらせる・相手の方へ顔を向けるなど、骨を曲げる書き換え（BonePoseModifier）へ渡す向き・点を作るのに使う
	GameLib::Math::Vector3 toRootDirection( const GameLib::Math::Vector3& worldDirection ) const;
	GameLib::Math::Vector3 toRootPoint( const GameLib::Math::Vector3& worldPoint ) const;

private:
	const std::vector< GameLib::Math::Matrix34 >& bodyPose() const;	// 体のポーズ（体の書き換えが無ければアニメのポーズ、あれば mBodyPose）

	const SkinnedMeshData* mData;
	AnimationPlayer mPlayer; // アニメの再生（骨格・時刻・クロスフェード・アニメのポーズ）
	// 実際に見せるポーズ（アニメのポーズに書き換えを掛けた写し）。書き換えをアニメのポーズに直接入れると、
	// クロスフェードの混ぜ元にも入り込み、混ぜた後にもう一度掛かって二重になるため分けて持つ
	std::vector< GameLib::Math::Matrix34 > mShownPose;
	BonePoseModifier* mPoseModifier;				// 無ければ nullptr（アニメのポーズをそのまま見せる）
	// 体の書き換えを掛けた写し（アニメのポーズ ＋ mBodyPoseModifier）。当たり判定と、見せるポーズの元にする
	// アニメのポーズに直接入れないのは、mShownPose と同じくクロスフェードの混ぜ元に入り込ませないため
	std::vector< GameLib::Math::Matrix34 > mBodyPose;
	BonePoseModifier* mBodyPoseModifier;			// 無ければ nullptr（mBodyPose は使わずアニメのポーズを体のポーズとみなす）

	mutable std::vector< GameLib::Math::Matrix34 > mSkinMatrices;	// 描くときに求める骨の行列（ボーン数ぶん）。毎回確保し直さないための作業領域なので、const の draw から書けるよう mutable にする
	// mSkinMatrices が今のポーズ・置き場所より古いか（update・setPosition・setAngleY で立て、draw で作り直して下ろす）
	mutable bool mSkinStale;
	GameLib::Math::Vector3 mPosition;
	float mAngleY;
};

#endif
