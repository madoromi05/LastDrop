#ifndef INCLUDED_SKINNED_MESH_INSTANCE_H
#define INCLUDED_SKINNED_MESH_INSTANCE_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Graphics/VertexBuffer.h"
#include "GameLib/Scene/Tree.h"
#include "GameLib/Scene/Animation.h"
#include <vector>

class SkinnedMeshData;
class BonePoseModifier;

// スキンメッシュを置いた「1体」。自分の骨格（ポーズ）・変形後の頂点・位置と向きを持つ。
// 頂点・重み・テクスチャ・アニメは SkinnedMeshData を借りる（所有しない）。
// 同じキャラを何体出すときも、データは1つでインスタンスだけ増やせばよい。
//
// アニメの切り替えは2通り。setAnimation は即座に切り替え、crossFadeTo は切り替え直前に見えていたポーズから
// 新しいアニメへ数フレームかけて混ぜる（クロスフェード）。混ぜるのはボーンごとの姿勢行列（MatrixBlend）。
//
// 1体ごとに頂点バッファ（頂点数×約48バイト）と CPU スキニングの負荷がかかる。
// コピーすると骨格と頂点バッファを2体で共有してしまうので、コピーは禁止（std::unique_ptr 等で持つ）。
class SkinnedMeshInstance{
public:
	// data: 形と見た目。所有権は受け取らない。nullptr 不可、isReady() 済みであること。
	//       このインスタンスより長く生きていること（MeshCache が持つ）。
	explicit SkinnedMeshInstance( const SkinnedMeshData* data );
	SkinnedMeshInstance( const SkinnedMeshInstance& ) = delete;
	SkinnedMeshInstance& operator=( const SkinnedMeshInstance& ) = delete;

	void setPosition( const GameLib::Math::Vector3& );	// 置き場所（ワールド座標）
	void setAngleY( float degree );						// Y軸まわりの向き（度）
	// 再生するアニメを名前で設定する。見つからなければデバッグ出力してレスト姿勢のまま。
	// startTime: 再生開始時刻（フレーム）。同じアニメの複数体をずらして、動きが揃わないようにする。
	void setAnimation( const char* name, float startTime );
	// 取得済みのアニメで即座に切り替える（混ぜない）。空の Animation ならレスト姿勢になる。
	void setAnimation( GameLib::Scene::Animation, float startTime );
	// 今見えているポーズから、animation へ fadeFrames フレームかけて滑らかに切り替える。
	// フェード中に呼び直してもよい（その時点で見えている混ざったポーズから、改めて混ぜ始める）。
	// startTime: 新しいアニメの再生開始時刻（フレーム）。歩き⇔走りで足の位相を合わせるときに使う。
	void crossFadeTo( GameLib::Scene::Animation animation, float startTime, float fadeFrames );
	// 再生中のアニメが1周のどこまで進んだか（0以上1未満）。アニメが無い・長さ0なら0。
	float animationPhase() const;
	// 描く直前にポーズを書き換える処理（一人称で頭を消す・視点に合わせて上半身を曲げる等）。
	// 所有権は受け取らない。nullptr で外す。このインスタンスより長く生きていること。
	void setPoseModifier( BonePoseModifier* );

	// アニメを1フレーム進めて、ポーズどおりに頂点を変形する。描画前に毎フレーム1回呼ぶ。
	void update();
	// update で変形した頂点を描く。
	void draw() const;

	// ボーンの今の姿勢をワールド座標の行列で返す（置き場所・向き込み。最後の update で見せたポーズ。書き換え後）。
	// 武器など、骨に付けて一緒に動かす物の置き場所に使う。
	// boneIndex: SkinnedMeshData::findBone の番号。範囲外、またはまだ update していなければ false（out は変えない）。
	// 骨格の単位のまま返す（Mixamo なら cm の縮尺 0.01 が入っている）ので、付ける物の側で合わせる。
	bool boneMatrix( int boneIndex, GameLib::Math::Matrix34* out ) const;
	// 置き場所と向きの行列。骨格の根から見た位置（ボーンの姿勢・BonePoseModifier が求めた点）をワールドへ置くのに掛ける。
	GameLib::Math::Matrix34 rootMatrix() const;

private:
	// フェード中なら、切り替え前のポーズと今のポーズを混ぜて mBonePose を上書きし、フェードを1フレーム進める。
	void blendFadingPose();

	const SkinnedMeshData* mData;
	GameLib::Scene::Tree mSkeleton;						// この体だけのポーズ。根は原点・向き0のまま（置き場所は deform で掛ける）
	GameLib::Scene::Animation mAnimation;				// 再生中のアニメ（Tree は持っているアニメを教えてくれないので自分で覚える）
	float mTime;										// 次の update で使うアニメ時刻（フレーム）

	// ボーンの姿勢は、骨格の根から見た行列で持つ（置き場所・向きを含まない）。
	// クロスフェード中にキャラが移動・旋回しても、切り替え前のポーズが元の場所に取り残されないようにするため。
	std::vector< GameLib::Math::Matrix34 > mBonePose;		// 今フレームに見せるポーズ（フェード中は混ぜた後）
	// 実際に見せるポーズ（mBonePose に書き換えを掛けた写し）。書き換えを mBonePose に直接入れると、
	// クロスフェードの混ぜ元にも入り込み、混ぜた後にもう一度掛かって二重になるため分けて持つ。
	std::vector< GameLib::Math::Matrix34 > mShownPose;
	BonePoseModifier* mPoseModifier;				// 無ければ nullptr（アニメのポーズをそのまま見せる）
	std::vector< GameLib::Math::Matrix34 > mFadeFromPose;	// 切り替え直前に見えていたポーズ（フェード中だけ使う）
	float mFadeFrames;									// フェードの長さ（フレーム）。0 ならフェードしていない
	float mFadeElapsed;									// フェード開始からの経過（フレーム）
	bool mHasPose;										// mBonePose を一度でも計算したか（まだ何も見せていないならフェードしない）

	std::vector< GameLib::Math::Matrix34 > mSkinMatrices;	// deform の作業領域（ボーン数ぶん）
	GameLib::Graphics::VertexBuffer mVertexBuffer;		// 変形後の頂点。毎フレーム書き換える
	GameLib::Math::Vector3 mPosition;
	float mAngleY;
};

#endif
