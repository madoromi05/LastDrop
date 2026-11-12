#ifndef INCLUDED_DRAW_CULLING_H
#define INCLUDED_DRAW_CULLING_H

#include "GameSrc/Render/Culling/ViewFrustum.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix44.h"

class HitboxSet;
class TownVisibility;
class StaticMeshData;
class StaticMeshLods;
class LodSelector;

// 物を描くかどうかを決める 1 か所（SummaryPlan/個別仕様/最適化と計測/OptimizationA_Plan.txt 6 章）。次の 3 つで間引く
//   映る範囲：ViewFrustum（今まで通り）
//   描く距離：霧でほとんど見えなくなる距離より遠い物（目から見る絵だけ。影は月の範囲がもともと狭い）
//   小ささ  ：画面（影なら影のマップ）の上で 2 画素より小さい物（缶・落ちている物・切れた腕や脚だけ）
// ViewFrustum と同じく、描くかを決めるだけに使い、当たり判定やゲームの進行には使わないこと（見ているかどうかで結果が変わってはいけないため）
//   隠れ    ：目から見る絵だけ、簡易 PVS（TownVisibility）で見えないとした建物・ゾンビ（影には使わない）
// 置物の細かさの段（LOD）もここで選ぶ：目から見る絵では画面の上の大きさで選び直し、
// 懐中電灯の影・月の 1 枚目は最後に選んだ段、月の 2・3 枚目は一番粗い段（影は形がぼけて差が見えにくい）。LOD1・LOD2 で描いた数は Lod.1・Lod.2
// 間引いた数を回数の印に足す：Cull.zombie（間引いたゾンビ。理由は問わない）・Cull.distance（距離）・Cull.small（小ささ）・
// Pvs.building・Pvs.zombie（隠れ）
class DrawCulling{
public:
	// 何を描くか。間引き方と、回数の印の振り分けが変わる
	enum class Target{
		Large,  // 建物・車・自キャラ：映る範囲と距離
		Small,  // 缶・落ちている物・切れた腕や脚：それに加えて、小さすぎれば描かない
		Zombie, // ゾンビ：映る範囲と距離（間引いたら Cull.zombie にも数える）
	};

	// 骨に付けた箱・骨の位置から作った箱を広げる幅（m）。手・髪・構えた銃など、箱の外へ出る所の分
	static const float kPoseMargin;

	// 映る範囲だけで決める（懐中電灯の影。近くしか照らさないので距離も小ささも見ない）
	// projectionView：光の側の射影×ビュー
	explicit DrawCulling( const GameLib::Math::Matrix44& projectionView );
	// 目から見る絵（G-Buffer）。projectionView：カメラの射影×ビュー（TAA でずらす前）。eye：目の位置
	// drawDistance：これより遠い物は描かない（0 以下なら距離で間引かない。霧を切っている間）
	// fieldOfViewY：縦の画角（度）。screenHeight：描く絵の高さ（画素）
	// visibility：建物・ゾンビを隠れで間引く表（nullptr なら隠れでは間引かない。F6 の PVS を切っている間）。所有権は受け取らない
	static DrawCulling forCamera( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& eye, float drawDistance,
		float fieldOfViewY, int screenHeight, const TownVisibility* visibility );
	// 月の影の 1 枚（正射影）。texelSize：その枚の影のマップ 1 画素の幅（m。CascadedShadowMap::texelSize）
	// coarsestLod：置物をいつも一番粗い段で描くか（月の 2・3 枚目）。false なら最後に目から見て選んだ段
	static DrawCulling forOrthographicShadow( const GameLib::Math::Matrix44& projectionView, float texelSize, bool coarsestLod );

	// 霧の半分の距離（m）から、描く一番遠い距離（距離の霧で 1/255 未満しか残らない距離。m）
	// 高さの霧は足さない（足すと霧は濃くなるだけなので、距離の霧だけで決めれば見える物を消すことは無い）
	static float drawDistanceForFog( float fogHalfDistance );

	// boundsMin〜boundsMax の軸に沿った箱（ワールド）を描くか
	bool shows( const GameLib::Math::Vector3& boundsMin, const GameLib::Math::Vector3& boundsMax, Target target ) const;
	// 部位の箱（hitboxes）を全部囲む箱に kPoseMargin を足した物で shows。箱が 1 つも置かれていなければ描く（間引かない側）
	// Target::Zombie なら、足元（囲む箱の底の真ん中）が目のいるますから見えうるかも見る（隠れ）
	bool showsPosed( const HitboxSet& hitboxes, Target target ) const;
	// building 番の建物（GameWorld::buildings の並びの番号）が、隠れで間引かれないか。間引いたら Pvs.building に数える
	// 映る範囲・距離は見ないので、shows と両方で決める
	bool showsBuildingIndex( int building ) const;
	// boundsMin〜boundsMax（ワールド）に置いた物を、shapes のどの段で描くか。selector：その物の段を覚える係（目から見る絵の時だけ選び直す）
	// F6 の LOD を切っている間は元の形。selector：nullptr 不可
	const StaticMeshData* chooseLod( const StaticMeshLods& shapes, LodSelector* selector, const GameLib::Math::Vector3& boundsMin,
		const GameLib::Math::Vector3& boundsMax ) const;
	// 映る範囲だけ（地面。まとめて描くので距離・小ささでは間引かない）
	const ViewFrustum& frustum() const;

private:
	// 置物の段の決め方
	enum class LodPolicy{
		Select,   // 画面の上の大きさで選び直す（目から見る絵）
		Current,  // 最後に選んだ段（懐中電灯の影・月の 1 枚目）
		Coarsest, // 一番粗い段（月の 2・3 枚目）
	};

	DrawCulling( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& eye, float drawDistance,
		float minRadiusPerDistance, float minDiameter, float pixelsPerRadiusPerDistance, LodPolicy lodPolicy );
	// 間引いた理由を、回数の印に足す
	static void count( const char* reason, Target target );

	ViewFrustum mFrustum;
	const TownVisibility* mVisibility; // 借りているだけ（nullptr なら隠れでは間引かない）
	int mViewCell;                     // 目のいるます（TownVisibility::cellAt）
	GameLib::Math::Vector3 mEye;
	float mDrawDistance;         // これより遠い物は描かない（0 以下なら見ない）
	float mMinRadiusPerDistance; // 目から見る絵：箱の半径 ÷ 目からの距離がこれより小さければ描かない（0 なら見ない）
	float mMinDiameter;          // 正射影の影：箱の直径（m）がこれより小さければ描かない（0 なら見ない）
	float mPixelsPerRadiusPerDistance; // 目から見る絵：「半径 ÷ 距離」を画面の上の半径（画素）に直す倍率（LOD を選ぶ）
	LodPolicy mLodPolicy;
};

#endif
