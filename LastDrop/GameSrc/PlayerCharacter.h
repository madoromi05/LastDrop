#ifndef INCLUDED_PLAYER_CHARACTER_H
#define INCLUDED_PLAYER_CHARACTER_H

#include "GameLib/Math/Vector3.h"
#include "Animator.h"
#include <memory>

class SkinnedMeshData;
class SkinnedMeshInstance;

// TPS で操作する自キャラ。WASD をカメラの向き基準の移動方向に直して地面(Y=0)を歩かせ、
// 進む方向へ滑らかに振り向かせ、静止／歩き／走り のアニメを切り替える。
// カメラは持たない（向きを update の引数で受け取る）ので、カメラの種類を変えてもこのクラスは変わらない。
// 見た目は SkinnedMeshInstance、アニメの状態遷移は Animator に任せる（委譲）。
//
// v4 アクション用の Player（ワイヤー・射撃）とは別物。当たり判定・ジャンプはまだ持たない。
class PlayerCharacter{
public:
	// data: 形・見た目・アニメ（"idle" / "walk" / "run"）。所有権は受け取らない。nullptr 不可、isReady() 済みであること。
	//       このキャラより長く生きていること（MeshCache が持つ）。
	// position / angleY: 最初の足元の位置と向き（度。0 は +Z）。
	PlayerCharacter( const SkinnedMeshData* data, const GameLib::Math::Vector3& position, float angleY );
	~PlayerCharacter();
	PlayerCharacter( const PlayerCharacter& ) = delete;
	PlayerCharacter& operator=( const PlayerCharacter& ) = delete;

	// 入力を読んで1フレーム動かし、アニメを進めて頂点を変形する。
	// cameraYaw: カメラの水平の向き（度。ThirdPersonCamera::yaw）。W がこの向きへの前進になる。
	void update( float cameraYaw );
	void draw() const;

	const GameLib::Math::Vector3& position() const;	// 足元の位置（カメラの追従先）

private:
	// 入力から移動方向（水平・長さ1）を求める。入力が無ければ false。
	bool readMoveDirection( float cameraYaw, GameLib::Math::Vector3* direction ) const;
	// 向きを direction へ、1フレームの旋回上限まで近づける。
	void turnToward( const GameLib::Math::Vector3& direction );
	// アニメの状態を切り替える（同じ状態なら何もしない）。
	void changeState( Animator::State );

	std::unique_ptr< SkinnedMeshInstance > mMesh;
	Animator mAnimator;
	GameLib::Math::Vector3 mPosition;
	float mAngleY; // 向き（度）。setAngleY と同じ約束で、0 は +Z
};

#endif
