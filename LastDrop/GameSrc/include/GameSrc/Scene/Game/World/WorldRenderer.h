#ifndef INCLUDED_WORLD_RENDERER_H
#define INCLUDED_WORLD_RENDERER_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix44.h"
#include "GameSrc/Render/StaticMesh/StaticMeshInstancing.h"
#include "GameSrc/Render/StaticMesh/StaticMeshBatch.h"
#include <utility>
#include <vector>

class GameWorld;
class EffectManager;
class Building;
class StaticModelComponent;
class DrawCulling;

// 世界（GameWorld）を読んで描く係（View。SummaryPlan/個別仕様/設計と分割/GameWorldSplit_Plan.txt 2-1）
// 持ち主はシーン（GameScene）。世界は描き方を知らない（描く順・間引き・パイプラインを変えても、世界の進め方は変わらない）
// 世界の物は const で読むだけ。エフェクト（EffectManager）だけは描くときに並べ替えるので書いてよい形で借りる
// 物ごとの GPU の時間を印（Profiling::gpuMark）で分ける（Buildings・Props・Ground・Player・Zombies）
//
// 1 フレームに呼ぶ順（GameScene::draw）：
//   drawShadowCasters（影のマップごと）→ drawStatic → drawDecals → drawFirstPerson → drawCharacters →
//   （ライティング・霧・TAA）→ drawImpactDust → drawBloodSpray → drawMuzzleFlash
// 動かない物は手前に来やすい物から描く：建物（目に近い順）→ 車・置物 → 地面。先に描いた物に隠れる画素は、GPU が色の計算の前に捨てる
// （Early-Z）。建物と置物は形ごとにインスタンシングでまとめて描く（建物は形の中で目に近い順）。動かない物と動く物の間にデカール（弾痕）を描き、後から描く動く物がデカールを上書きして、
// キャラに弾痕が乗らないようにする
class WorldRenderer{
public:
	// world: 描く世界。effects: world の持つエフェクト（GameWorld::effects）。どれも所有権は受け取らない。nullptr 不可。この係より長く生きていること
	WorldRenderer( const GameWorld* world, EffectManager* effects );
	WorldRenderer( const WorldRenderer& ) = delete;
	WorldRenderer& operator=( const WorldRenderer& ) = delete;

	// 動かない物（建物・車・置物・地面）を描く
	// culling: 描くかどうか（DrawCulling::forCamera）。地面は映る範囲だけ、建物・置物は描く距離・小ささでも間引く
	// eye: 目の位置（FirstPersonCamera::eyePosition）。建物を近い順に並べるのに使う
	void drawStatic( const DrawCulling& culling, const GameLib::Math::Vector3& eye );
	// 弾痕を G-Buffer へデカールとして描く。drawStatic の後・drawFirstPerson の前に、GBuffer::beginDecals〜endDecals の間で呼ぶ。1フレームに1回まで
	// projectionView: カメラの射影×ビュー行列（FirstPersonCamera::projectionViewMatrix）
	void drawDecals( const GameLib::Math::Matrix44& projectionView );
	// 動く物を描く。drawDecals の後に、この順に呼ぶ。TAA のために動きの種類を分けて描く（
	// 呼ぶ側が間で GBuffer::setMotion を切り替える）。drawFirstPerson：手元（自キャラ・持っている銃・弾。いつも映るので間引かない）、
	// drawCharacters：ゾンビ・千切れた部位（culling は drawStatic と同じ物）
	void drawFirstPerson() const;
	void drawCharacters( const DrawCulling& culling ) const;
	// 影を落とす物を描く（ShadowMap::begin〜end の間）。ゾンビ・千切れた部位・建物・車・置物
	// 地面（受けるだけ）と弾頭（小さく速い）は描かない。culling：光の側から見た映る範囲と、月の影なら小ささ（全部の物を間引く）
	// includesPlayer：自キャラ（体・腕・銃）も描くか（懐中電灯の影には描かない。銃の影が画面の真ん中を埋めるため）
	void drawShadowCasters( const DrawCulling& culling, bool includesPlayer );
	// 着弾の砂ぼこりを描く（半透明。ライティングの後）
	// viewDirection: 視線の向き（長さ1。FirstPersonCamera::viewDirection）。粒の板を画面に平行にし、カメラ側から照らすため
	// eye: 目の位置（FirstPersonCamera::eyePosition）。粒を目から遠い順に並べ替えて描くため
	void drawImpactDust( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& viewDirection,
		const GameLib::Math::Vector3& eye );
	// ゾンビに当たった血しぶきを描く。呼ぶ場所の決まり・引数は drawImpactDust と同じ（砂ぼこりの後に呼ぶ）
	void drawBloodSpray( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& viewDirection,
		const GameLib::Math::Vector3& eye );
	// 撃った瞬間に銃口が光る板を描く。呼ぶ場所の決まりは drawImpactDust と同じ（加算なので順番は問わない）
	// 銃口の位置は、描くたびに自キャラの今の銃から取り直す
	void drawMuzzleFlash( const GameLib::Math::Matrix44& projectionView, const GameLib::Math::Vector3& viewDirection ) const;

private:
	void drawZombies( const DrawCulling& culling ) const;		// culling で描くと決まったゾンビだけ描く（drawCharacters・drawShadowCasters から）
	void drawSeveredLimbs( const DrawCulling& culling ) const;	// 千切れた部位の物を全部たどり、culling で描くと決まった物だけ描く（同じ 2 か所から。ゾンビの後）
	void drawPlayer() const;								// 自キャラの体と、手に持っている銃を描く（drawFirstPerson・drawShadowCasters から）
	void beginBatch();										// 建物か置物を描き始める。インスタンシングで描けるか（F6 の Instancing が ON で、今のパスのシェーダがある）をここで決める
	// culling で描くと決まった建物を、まとめて描くなら集め、1 軒ずつなら今描く。beginBatch〜endBatch の間に、描きたい順に呼ぶ
	// 隠れ（PVS）も culling に聞く（目から見る絵の culling だけが表を持つ）
	void addBuilding( const Building& building, const DrawCulling& culling );
	int buildingIndex( const Building& building ) const;	// building の GameWorld::buildings の並びでの番号（PVS の表の番号）
	void drawProps( const DrawCulling& culling );			// 置物を全部たどり、culling で描くと決まった物を描く（beginBatch〜endBatch は中で呼ぶ）
	void addProp( const StaticModelComponent& model, const DrawCulling& culling );	// culling で描くと決まった置物を、まとめて描くなら集め、1 つずつなら今描く。beginBatch〜endBatch の間に呼ぶ
	void endBatch();										// 集めた物を形ごとにまとめて描く（1 つずつ描いているときは何もしない）

	const GameWorld* mWorld;												// 借りているだけ
	EffectManager* mEffects;												// 借りているだけ
	StaticMeshInstancing mInstancing;										// 建物と置物をまとめて描く道具。シェーダは G-Buffer と影の両方で使う
	StaticMeshBatch mStaticBatch;											// 1 回の描画（G-Buffer・影の 1 枚）ぶんの建物か置物を集める作業領域
	bool mBatches;															// beginBatch で決めた、今の建物か置物をまとめて描くか
	std::vector< std::pair< float, const Building* > > mBuildingDrawOrder;	// 建物を目に近い順に描くための並び（目からの距離の 2 乗と建物）。drawStatic の作業領域（毎フレーム確保し直さないために持つ）
};

#endif
