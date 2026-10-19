#ifndef INCLUDED_CHARACTER_MOVER_H
#define INCLUDED_CHARACTER_MOVER_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/Collision/BoxCollider.h"

class CollisionWorld;
class ICollisionBody;

// 当たりながら滑って進む移動（Collide-and-Slide）。Unity の CharacterController.Move にあたる。
// 進みたい量を受け取り、CollisionWorld に問い合わせて、壁や他のキャラにめり込まない移動量を返す。
//   1. 動かす前にめり込んでいたら外へ出す（体の向きを変えて壁に入り込んだとき・他のキャラに寄ってこられたとき）
//   2. 進みたい向きへ掃引し、一番早く当たる所の少し手前（kSkinWidth）まで進む
//   3. 残りの移動量から当たった面へ向かう成分を除き（面に沿う向きへ曲げ）、2 からやり直す（最大 kMaxSlideIterations 回）
// 位置は持たない（持ち主が持つ）。持ち主はこのクラスを1つ持ち、移動を任せる（委譲）。
class CharacterMover{
public:
	// world: 問い合わせ先。所有権は受け取らない。nullptr 不可。この Mover より長く生きていること。
	// self: 動かす本人。今の当たり判定（位置・向き・大きさ）をここから読み、自分自身には当たらないよう除く。
	//       所有権は受け取らない。nullptr 不可（持ち主が自分を渡す）。
	CharacterMover( const CollisionWorld* world, const ICollisionBody* self );

	// 本人を今の位置から delta だけ動かそうとして、実際に動いた量を返す（めり込みを外へ出した分も含む）。
	// 本人の位置は書き換えない（返した量を持ち主が自分の位置へ足す）。delta の y は無視する（地面を歩く）。
	// 向きを変えるときは、向きを変えてから呼ぶ（変えた向きでめり込んだ分も、ここで外へ出す）。
	GameLib::Math::Vector3 move( const GameLib::Math::Vector3& delta ) const;
	// position（本人の足元）の高さを、真下の地面（歩道の段差など）の高さへ1ティック分だけ近づけた値を返す。
	// 一気に合わせると、段差を越えた瞬間に視点が跳ねるので、決まった速さで上り下りする。地面が無ければ position.y のまま。
	// 1ティックに1回、move の後の位置で呼ぶ（返した値を持ち主が自分の位置の y に入れる）。
	float followGround( const GameLib::Math::Vector3& position ) const;

private:
	// box がめり込んでいれば、外へ出るまで box の位置を動かす。
	void depenetrate( BoxCollider* box ) const;

	const CollisionWorld* mWorld;
	const ICollisionBody* mSelf;
};

#endif
