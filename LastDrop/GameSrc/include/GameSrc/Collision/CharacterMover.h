#ifndef INCLUDED_CHARACTER_MOVER_H
#define INCLUDED_CHARACTER_MOVER_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/Collision/BoxCollider.h"
#include "GameSrc/Collision/CollisionWorld.h"

class ICollisionBody;

// 当たりながら滑って進む移動（Collide-and-Slide）。Unity の CharacterController.Move にあたる
// 進みたい量を受け取り、CollisionWorld に問い合わせて、壁や他のキャラにめり込まない移動量を返す
//   1. 動かす前にめり込んでいたら外へ出す（体の向きを変えて壁に入り込んだとき・他のキャラに寄ってこられたとき）
//   2. 進みたい向きへ掃引し、一番早く当たる所の少し手前（kSkinWidth）まで進む
//   3. 残りの移動量から当たった面へ向かう成分を除き（面に沿う向きへ曲げ）、2 からやり直す（最大 kMaxSlideIterations 回）
// 位置は持たない（持ち主が持つ）。持ち主はこのクラスを1つ持ち、移動を任せる（委譲）
class CharacterMover{
public:
	// self: 動かす本人。今の当たり判定（位置・向き・大きさ）をここから読み、自分自身には当たらないよう除く
	//       所有権は受け取らない。nullptr 不可（持ち主が自分を渡す）
	// 問い合わせ先（CollisionWorld）は後から setWorld で渡す（持ち主の部品は、付けた後で共有の棚から引くため）
	// 渡す前に動かさないこと
	explicit CharacterMover( const ICollisionBody* self );

	void setWorld( const CollisionWorld* world );	// world: 問い合わせ先。所有権は受け取らない。nullptr 不可。この Mover より長く生きていること。1 回だけ呼ぶ

	// 本人を今の位置から delta だけ動かそうとして、実際に動いた量を返す（めり込みを外へ出した分も含む）
	// 本人の位置は書き換えない（返した量を持ち主が自分の位置へ足す）。delta の y は無視する（地面を歩く）
	// 向きを変えるときは、向きを変えてから呼ぶ（変えた向きでめり込んだ分も、ここで外へ出す）
	GameLib::Math::Vector3 move( const GameLib::Math::Vector3& delta ) const;
	// move と同じだが、passThrough（ほかの動くもの 1 つ。nullptr なら無し）には当たらずに通り抜ける
	// ゾンビの攻撃の踏み込みが、攻撃する相手（自キャラ）の箱で止まらないようにするのに使う
	GameLib::Math::Vector3 movePassingThrough( const GameLib::Math::Vector3& delta, const ICollisionBody* passThrough ) const;
	// move と同じだが、ほかの動くもの（キャラ・拾える物）には全部当たらず、動かない形・建物でだけ止まる
	// 倒れる体（死体はほかを止めないので、ほかにも止められない）に使う
	GameLib::Math::Vector3 moveIgnoringBodies( const GameLib::Math::Vector3& delta ) const;
	// position（本人の足元）の高さを、真下の地面（歩道の段差など）の高さへ1ティック分だけ近づけた値を返す
	// 一気に合わせると、段差を越えた瞬間に視点が跳ねるので、決まった速さで上り下りする。地面が無ければ position.y のまま
	// 1ティックに1回、move の後の位置で呼ぶ（返した値を持ち主が自分の位置の y に入れる）
	float followGround( const GameLib::Math::Vector3& position ) const;

private:
	GameLib::Math::Vector3 move( const GameLib::Math::Vector3& delta, const CollisionWorld::BodyFilter& filter ) const;	// move の本体。filter に除かれた相手には当たらない
	void depenetrate( BoxCollider* box, const CollisionWorld::BodyFilter& filter ) const;								// box がめり込んでいれば、外へ出るまで box の位置を動かす

	const CollisionWorld* mWorld;
	const ICollisionBody* mSelf;
};

#endif
