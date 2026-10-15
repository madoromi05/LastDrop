#ifndef INCLUDED_BULLET_HOLE_LIST_H
#define INCLUDED_BULLET_HOLE_LIST_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Graphics/Texture.h"
#include "GameSrc/Render/LitQuadBatch.h"
#include <vector>

class IRayBlocker;

// 弾が物（建物・車・地面・拾える物）に当たった跡。当たった面に貼った小さな四角い板として描く（SummaryPlan/BulletHole_Plan.txt の案1）。
// 同時に残すのは決まった数まで。超えたら古い物から消し、超えなくても決まった時間で消す（途中から少しずつ薄くなる）。
// どこに足すか（ゾンビには付けない、など）は持ち主（GameWorld）が決める。ここは「足す・古くする・描く」だけ。
// 板は面の上に浮かせて置くだけなので、角や窓枠の端に当たると板がはみ出す（気になったら案2：面に沿って切り取る）。
// 壁や地面と同じライトで照らす（LitQuadBatch。2026-10-15 ユーザー指定）。当たった面の法線で照らすので、壁と同じ明るさになる。
class BulletHoleList{
public:
	BulletHoleList();
	BulletHoleList( const BulletHoleList& ) = delete;
	BulletHoleList& operator=( const BulletHoleList& ) = delete;

	// point: 当たった位置。normal: 当たった面の外向きの法線（長さ1）。どちらも HitWorld::Hit の値をそのまま渡す。
	// attachedTo: 跡が付いた物。物が消えるとき removeAttachedTo で一緒に消すための目印で、中身は読まない。nullptr 不可。
	void add( const GameLib::Math::Vector3& point, const GameLib::Math::Vector3& normal, const IRayBlocker* attachedTo );
	// 1ティック古くし、残す時間を過ぎた跡を消す。
	void update();
	// attachedTo に付いた跡を全部消す。物を消す前に呼ぶ（消した後だと、同じ場所に作られた別の物と見分けられないため）。
	void removeAttachedTo( const IRayBlocker* attachedTo );
	// 全部の跡を1回で描く。1フレームに1回まで。ライトを設定した後に呼ぶ（LitQuadBatch::draw と同じ）。
	// 物を描いた後、同じ描画先（PostProcess の beginScene〜endScene の間）で呼ぶ（壁の向こうの跡を、壁の奥行きで隠すため）。
	// 絵の読み込みが終わるまでは描かない。projectionView: カメラの射影×ビュー行列。
	void draw( const GameLib::Math::Matrix44& projectionView );

private:
	struct Hole{
		GameLib::Math::Vector3 center; // 板の中心（当たった位置から法線の向きへ浮かせた所）
		GameLib::Math::Vector3 right;  // 板の横の半分（中心から右の辺まで。ワールド）
		GameLib::Math::Vector3 up;     // 板の縦の半分（中心から上の辺まで。ワールド）
		GameLib::Math::Vector3 normal; // 当たった面の外向きの法線（照らす向き）
		const IRayBlocker* attachedTo; // 目印だけ（読まない）
		int age;                       // 付いてからのティック数
	};
	// 板1枚を、古さに合わせた不透明度で mQuads へためる。
	void addQuad( const Hole& hole );

	std::vector< Hole > mHoles;           // 古い順
	float mNextTurnDegree;                // 次に足す板を、法線まわりに回す角度（度）
	GameLib::Graphics::Texture mTexture;  // 弾痕の絵（Resource/bullet_hole.tga）
	LitQuadBatch mQuads;                  // 板をためて、ライトを当てて描く
};

#endif
