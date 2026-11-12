#ifndef INCLUDED_ZOMBIE_STRIDE_H
#define INCLUDED_ZOMBIE_STRIDE_H

#include "GameLib/Scene/Animation.h"
#include <vector>

class SkinnedMeshData;

// その場で歩くアニメ（根の骨が進まない walk）の、1フレームごとに体を前へ進める長さの表（ルートモーションの代わり）
// 地面についている足（左右の足先のうち低い方）が後ろへ動いた長さだけ体を前へ進めれば、足は地面を滑らない
// 歩き方がびっこを引いていて（左右で歩幅も速さも違う）一定の速さでは足が滑るので、フレームごとの表にする
// 表はメッシュとアニメが同じなら同じなので、種類ごとに1つだけ作る（ZombieModel が持つ。歩きと走りで 1 つずつ）。作るときに 1 周ぶんの姿勢を計算する
class ZombieStride{
public:
	// data: 形と骨。所有権は受け取らない。nullptr 不可、isReady() 済み。姿勢を計算するのに構築中だけ使う（覚えない）
	// walk: その場で歩くアニメ。空なら表を作らない（isValid が false）
	// leftToe / rightToe: 左右の足先の骨（SkinnedMeshData::findBone の番号）。どちらかが -1 なら表を作らない
	ZombieStride( const SkinnedMeshData* data, GameLib::Scene::Animation walk, int leftToe, int rightToe );

	bool isValid() const;					// 表があるか（無ければ、使う側が決まった速さで歩かせる）
	float distanceAt( float time ) const;	// アニメ時刻 time（フレーム。1周を超えてもよい）から次の1フレームの間に、体を前へ進める長さ（m）
	float cycleLength() const;				// 1周で進む長さ（m）。確認用

private:
	std::vector< float > mDistances; // フレームごとの進む長さ（m）。添え字はアニメ時刻（フレーム）
	float mPeriod;                   // アニメの1周（フレーム）
};

#endif
