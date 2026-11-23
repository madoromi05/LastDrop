#ifndef INCLUDED_BULLET_HOLE_LIST_H
#define INCLUDED_BULLET_HOLE_LIST_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Graphics/Texture.h"
#include "GameSrc/Render/DecalBatch.h"
#include <vector>

class IRayBlocker;

// 弾が物（建物・車・地面・拾える物）に当たった跡。G-Buffer へデカール（DecalBatch）として、当たった面そのものに色と凹みの法線を書く
// 同時に残すのは決まった数まで。超えたら古い物から消し、超えなくても決まった時間で消す（途中から少しずつ薄くなる）
// どこに足すか（ゾンビには付けない、など）は頼む側（CombatResolver。EffectManager を通す）が決める。ここは「足す・古くする・描く」だけ
// 面そのものに書くので、角や窓枠の端に当たってもはみ出さない（案2 の「面に沿って切り取る」を作らずに済んだ）
// G-Buffer に書くので、壁や地面と同じライト・影・霧・SSAO がそのまま掛かる
class BulletHoleList{
public:
	BulletHoleList();
	BulletHoleList( const BulletHoleList& ) = delete;
	BulletHoleList& operator=( const BulletHoleList& ) = delete;

	// point: 当たった位置。normal: 当たった面の外向きの法線（長さ1）。どちらも HitWorld::Hit の値をそのまま渡す
	// attachedTo: 跡が付いた物。物が消えるとき removeAttachedTo で一緒に消すための目印で、中身は読まない。nullptr 不可
	void add( const GameLib::Math::Vector3& point, const GameLib::Math::Vector3& normal, const IRayBlocker* attachedTo );
	void update();											// 1ティック古くし、残す時間を過ぎた跡を消す
	void removeAttachedTo( const IRayBlocker* attachedTo );	// attachedTo に付いた跡を全部消す。物を消す前に呼ぶ（消した後だと、同じ場所に作られた別の物と見分けられないため）
	// 全部の跡を描く。1フレームに1回まで。GBuffer::beginDecals〜endDecals の間に呼ぶ（動かない物を描いた後・動く物の前）
	// 絵の読み込みが終わるまでは描かない。F6 の Decals を切っている間も描かない。projectionView: カメラの射影×ビュー行列
	void draw( const GameLib::Math::Matrix44& projectionView );

private:
	struct Hole{
		GameLib::Math::Vector3 center; // 当たった位置（面の上）
		GameLib::Math::Vector3 right;  // 絵の横の半分（中心から右の辺まで。ワールド）
		GameLib::Math::Vector3 up;     // 絵の縦の半分（中心から上の辺まで。ワールド）
		GameLib::Math::Vector3 normal; // 当たった面の外向きの法線（デカールの箱の厚さの向き）
		const IRayBlocker* attachedTo; // 目印だけ（読まない）
		int age;                       // 付いてからのティック数
	};
	void addDecal( const Hole& hole );	// 1 つを、古さに合わせた不透明度で mDecals へためる

	std::vector< Hole > mHoles;           // 古い順
	float mNextTurnDegree;                // 次に足す板を、法線まわりに回す角度（度）
	GameLib::Graphics::Texture mTexture;  // 弾痕の絵（Resource/bullet_hole。色とアルファ）
	GameLib::Graphics::Texture mNormalMap; // 弾痕の凹みの法線（Resource/bullet_hole_normal）
	DecalBatch mDecals;                   // 跡をためて、G-Buffer へ描く
};

#endif
