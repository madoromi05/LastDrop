#ifndef INCLUDED_BLOOD_STAIN_LIST_H
#define INCLUDED_BLOOD_STAIN_LIST_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Graphics/Texture.h"
#include "GameSrc/Render/DecalBatch.h"
#include <vector>

class IRayBlocker;

// 血の跡（撃ったゾンビの後ろの壁・地面に飛び散った血と、足元に落ちた滴の跡）の持ち主（SummaryPlan/個別仕様/グラフィック/BloodStain_Plan.txt。グラフィック段階10）。
// 弾痕（BulletHoleList）と同じく G-Buffer へデカールとして、当たった面そのものに色と凹凸の法線を書く。数・残る時間・絵が違うので別の名簿にする。
// どこに付けるか（後ろ 2m・真下 1.5m・後ろのゾンビには付けない）は頼む側（CombatResolver）、大きさは EffectManager が決める。
// ここは「足す・古くする・描く」だけ。見た目だけで、遊び（ゾンビが血をたどる など）には使わない（GraphicsExtra_Spec.txt Q7）。
// 絵は 4 種類を 2×2 に並べた 1 枚（tools/texgen/blood_stain_gen.py）。飛び散った血は 3 種類を順に回し、滴の跡は 1 種類を向きをばらして貼る。
class BloodStainList{
public:
	BloodStainList();
	BloodStainList( const BloodStainList& ) = delete;
	BloodStainList& operator=( const BloodStainList& ) = delete;

	// 飛び散った血を足す。絵の筋は direction を面に写した向きへ伸ばす（跡の形で撃った向きが分かる）。
	// point / normal: 付ける面の上の点と外向きの法線（長さ1。HitWorld::Hit の値）。direction: 弾の進む向き（長さ1）。
	// 面にほぼ垂直に撃ったとき（面に写した向きが無いとき）は、絵の向きをばらす。halfSize: 絵の幅の半分（m）。
	// attachedTo: 跡が付いた物。物が消えるとき removeAttachedTo で一緒に消すための目印で、中身は読まない。nullptr 不可。
	void addSplatter( const GameLib::Math::Vector3& point, const GameLib::Math::Vector3& normal, const GameLib::Math::Vector3& direction,
		float halfSize, const IRayBlocker* attachedTo );
	// 落ちた滴の跡を足す。向きはばらす。引数は addSplatter と同じ。
	void addDrip( const GameLib::Math::Vector3& point, const GameLib::Math::Vector3& normal, float halfSize, const IRayBlocker* attachedTo );
	// 1ティック古くし、残す時間を過ぎた跡を消す。
	void update();
	// attachedTo に付いた跡を全部消す。物を消す前に呼ぶ（BulletHoleList::removeAttachedTo と同じ理由）。
	void removeAttachedTo( const IRayBlocker* attachedTo );
	// 全部の跡を描く。1フレームに1回まで。GBuffer::beginDecals〜endDecals の間で、弾痕の後に呼ぶ（血が穴の上に乗る）。
	// 絵の読み込みが終わるまでは描かない。F6 の Decals を切っている間も描かない。projectionView: カメラの射影×ビュー行列。
	void draw( const GameLib::Math::Matrix44& projectionView );

private:
	struct Stain{
		GameLib::Math::Vector3 center; // 付けた位置（面の上）
		GameLib::Math::Vector3 right;  // 絵の横の半分（中心から右の辺まで。ワールド）
		GameLib::Math::Vector3 up;     // 絵の縦の半分（中心から上の辺まで。ワールド）
		GameLib::Math::Vector3 normal; // 付けた面の外向きの法線
		const IRayBlocker* attachedTo; // 目印だけ（読まない）
		int cell;                      // 2×2 の絵のどれか（0 左上・1 右上・2 左下・3 右下）
		int age;                       // 付いてからのティック数
	};
	// 絵の右の向き（面に沿う。長さ1）・大きさ・絵の番号で 1 つ足す。数を超えたら一番古い物を消す。
	void add( const GameLib::Math::Vector3& point, const GameLib::Math::Vector3& normal, const GameLib::Math::Vector3& rightDirection,
		float halfSize, int cell, const IRayBlocker* attachedTo );
	// 面の上で、次の回す角度の向き（長さ1）を作り、角度を進める（向きをばらすとき）。
	GameLib::Math::Vector3 nextTurnedDirection( const GameLib::Math::Vector3& normal );

	std::vector< Stain > mStains;          // 古い順
	float mNextTurnDegree;                 // 向きをばらすときに次に使う角度（度）
	int mNextSplatterCell;                 // 次に使う飛び散った血の絵
	GameLib::Graphics::Texture mTexture;   // 血の跡の絵（Resource/blood_stain。色とアルファ）
	GameLib::Graphics::Texture mNormalMap; // 血の厚みの法線（Resource/blood_stain_normal）
	DecalBatch mDecals;                    // 跡をためて、G-Buffer へ描く
};

#endif
