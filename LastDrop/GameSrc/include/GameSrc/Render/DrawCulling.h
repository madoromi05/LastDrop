#ifndef INCLUDED_DRAW_CULLING_H
#define INCLUDED_DRAW_CULLING_H

#include "GameSrc/Render/ViewFrustum.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix44.h"

class HitboxSet;

// 物を描くかどうかを決める 1 か所（SummaryPlan/個別仕様/OptimizationA_Plan.txt 6 章）。次の 3 つで間引く。
//   映る範囲：ViewFrustum（今まで通り）
//   描く距離：霧でほとんど見えなくなる距離より遠い物（目から見る絵だけ。影は月の範囲がもともと狭い）
//   小ささ  ：画面（影なら影のマップ）の上で 2 画素より小さい物（缶・落ちている物・切れた腕や脚だけ）
// ViewFrustum と同じく、描くかを決めるだけに使い、当たり判定やゲームの進行には使わないこと（見ているかどうかで結果が変わってはいけないため）。
// 間引いた数を回数の印に足す：Cull.zombie（間引いたゾンビ。理由は問わない）・Cull.distance（距離）・Cull.small（小ささ）。
class DrawCulling{
public:
	// 何を描くか。間引き方と、回数の印の振り分けが変わる。
	enum class Target{
		Large,  // 建物・車・自キャラ：映る範囲と距離
		Small,  // 缶・落ちている物・切れた腕や脚：それに加えて、小さすぎれば描かない
		Zombie, // ゾンビ：映る範囲と距離（間引いたら Cull.zombie にも数える）
	};

	// 骨に付けた箱・骨の位置から作った箱を広げる幅（m）。手・髪・構えた銃など、箱の外へ出る所の分（Q5）。
	static const float kPoseMargin;

	// 映る範囲だけで決める（懐中電灯の影。近くしか照らさないので距離も小ささも見ない）。
	// projectionView：光の側の射影×ビュー。
	explicit DrawCulling( const GameLib::Math::Matrix44& projectionView );
	// 目から見る絵（G-Buffer）。projectionView：カメラの射影×ビュー（TAA でずらす前）。eye：目の位置。
	// drawDistance：これより遠い物は描かない（0 以下なら距離で間引かない。霧を切っている間）。
	// fieldOfViewY：縦の画角（度）。screenHeight：描く絵の高さ（画素）。
	static DrawCulling forCamera( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& eye, float drawDistance,
		float fieldOfViewY, int screenHeight );
	// 月の影の 1 枚（正射影）。texelSize：その枚の影のマップ 1 画素の幅（m。CascadedShadowMap::texelSize）。
	static DrawCulling forOrthographicShadow( const GameLib::Math::Matrix44& projectionView, float texelSize );

	// 霧の半分の距離（m）から、描く一番遠い距離（距離の霧で 1/255 未満しか残らない距離。m）。
	// 高さの霧は足さない（足すと霧は濃くなるだけなので、距離の霧だけで決めれば見える物を消すことは無い）。
	static float drawDistanceForFog( float fogHalfDistance );

	// boundsMin〜boundsMax の軸に沿った箱（ワールド）を描くか。
	bool shows( const GameLib::Math::Vector3& boundsMin, const GameLib::Math::Vector3& boundsMax, Target target ) const;
	// 部位の箱（hitboxes）を全部囲む箱に kPoseMargin を足した物で shows。箱が 1 つも置かれていなければ描く（間引かない側）。
	bool showsPosed( const HitboxSet& hitboxes, Target target ) const;
	// 映る範囲だけ（地面。まとめて描くので距離・小ささでは間引かない）。
	const ViewFrustum& frustum() const;

private:
	DrawCulling( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& eye, float drawDistance,
		float minRadiusPerDistance, float minDiameter );
	// 間引いた理由を、回数の印に足す。
	static void count( const char* reason, Target target );

	ViewFrustum mFrustum;
	GameLib::Math::Vector3 mEye;
	float mDrawDistance;         // これより遠い物は描かない（0 以下なら見ない）
	float mMinRadiusPerDistance; // 目から見る絵：箱の半径 ÷ 目からの距離がこれより小さければ描かない（0 なら見ない）
	float mMinDiameter;          // 正射影の影：箱の直径（m）がこれより小さければ描かない（0 なら見ない）
};

#endif
