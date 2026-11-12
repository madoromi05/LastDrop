#ifndef INCLUDED_ZOMBIE_ROOT_MOTION_SPAN_H
#define INCLUDED_ZOMBIE_ROOT_MOTION_SPAN_H

#include "GameLib/Math/Vector3.h"

class ZombieRootMotion;
class SkinnedMeshInstance;
class CharacterMover;
class ICollisionBody;

// 攻撃・よろけ・倒れるアニメの間、腰の前後の動き（根の動き）を、壁・車に当たりながら置き場所へ移す区間（ゾンビ 1 体ぶん）
// アニメの間、移動の箱と考える所（bodyCenter）は始めた所（アンカー）に残し、置き場所（足元）だけを根に付いて行かせる
// 絵はその分だけ戻して置くので、何にも当たらなければ見た目は変わらず、当たったときだけ体が手前で止まる
// （SummaryPlan/個別仕様/ゾンビ/ZombieRootMotion_Plan.txt）
// ZombieRootMotion（アニメごとの根の動きの表。同じ種類の全員で共有）とは別物で、こちらは「この体が今その表のどこを流していて、
// どこから始めたか」を持つ。いつ始めて・どの表で・誰を通り抜けるかは持ち主（Zombie）が決める
// 足元は持ち主が持ったまま、関数ごとにポインタで渡す（足元は追う・引きずる・沈むでも使うので、区間の物にしない）
// メッシュを置くのは持ち主とこの区間の 2 か所（区間の間の絵の根はここが置く）
class ZombieRootMotionSpan{
public:
	// 置き場所を動かすとき、ほかの動くもの（キャラ・拾える物）のどれを通り抜けるか
	enum class Pass{
		None,      // どれにも当たる（よろけ）
		Target,    // 追う先（自キャラ）だけ通り抜ける（攻撃の踏み込み。止めると手が届かない）
		AllBodies, // 全部通り抜ける（倒れる体。死体はほかを止めないので、ほかにも止められない）
	};

	// mesh: 根の動きを読み、絵の根を置くメッシュ。mover: 当たりながら動かす係。どちらも所有権は受け取らない。nullptr 不可
	//       この区間より長く生きていること（どちらも同じ Zombie のメンバで、この区間より先に宣言する）
	ZombieRootMotionSpan( SkinnedMeshInstance* mesh, const CharacterMover* mover );
	ZombieRootMotionSpan( const ZombieRootMotionSpan& ) = delete;
	ZombieRootMotionSpan& operator=( const ZombieRootMotionSpan& ) = delete;

	// table のアニメを startTime から流し始めたので、区間を始める（table が nullptr なら前の区間を閉じるだけ）
	// 前の区間の途中でも、今見えている絵の根から始める（踏み込みの途中で倒れても、絵が飛ばない）。footing: 足元。nullptr 不可
	void start( const ZombieRootMotion* table, float startTime, Pass pass, GameLib::Math::Vector3* footing );
	// 見えているアニメ時刻の根の動きの分だけ、アンカーから当たりながら足元を動かし、絵の根をその分だけ戻して置く
	// メッシュの update の後、部位の箱を置く前に呼ぶ。区間の外なら何もしない
	// passThrough: Pass::Target のとき通り抜ける体（nullptr 可）。footing: 足元。nullptr 不可
	void apply( const ICollisionBody* passThrough, GameLib::Math::Vector3* footing );
	void end( GameLib::Math::Vector3* footing );												// 区間を閉じる（アニメが歩き・立つへ戻った・体を引きずり始める）。足元を絵の根へ戻す。区間の外なら何もしない
	void freeze();																				// 区間を閉じ、足元と絵の根のずれはそのまま残す（倒れ終わった死体。もう動かない）
	void moveFooting( const GameLib::Math::Vector3& moved, GameLib::Math::Vector3* footing );	// 攻撃中に寄るなど、足元そのものを moved だけ動かした（区間の間はアンカーも同じだけ動かす）

	bool isActive() const;                        // 区間の中か
	const GameLib::Math::Vector3& anchor() const; // 区間を始めた所（ワールド）。区間の中だけ意味がある
	GameLib::Math::Vector3 meshRoot( const GameLib::Math::Vector3& footing ) const;

private:
	SkinnedMeshInstance* mMesh;				// 借りているだけ
	const CharacterMover* mMover;			// 借りているだけ
	const ZombieRootMotion* mTable;			// 今の区間の表。nullptr なら区間の外
	Pass mPass;								// 今の区間で通り抜ける相手
	GameLib::Math::Vector3 mAnchor;			// 区間を始めた所（ワールド）。移動の箱と bodyCenter はここ
	GameLib::Math::Vector3 mStartOffset;	// 区間を始めた時刻の根の水平の位置（骨格の根の空間）
	float mLastTime;						// 最後に足元を動かしたアニメ時刻（同じなら当たり直さない。止まった死体）
	GameLib::Math::Vector3 mMeshOffset;		// 絵の根 − 足元（ワールド。水平だけ。区間の間は −根の動き）。区間の外は 0。ただし倒れ終わった死体は最後の値のまま
};

#endif
