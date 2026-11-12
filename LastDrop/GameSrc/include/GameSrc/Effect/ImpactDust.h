#ifndef INCLUDED_IMPACT_DUST_H
#define INCLUDED_IMPACT_DUST_H

#include "GameLib/Math/Vector3.h"
#include <random>

// 弾が物に当たった瞬間に舞う砂ぼこり1回分。当たった点から粉の粒を面の外へ飛ばし、減速させ、少しだけ落とし、
// 広がりながら薄くして、決まった時間で終わる（isFinished）
// 粒は中心近くに多く、外へ遠く飛ぶ粒ほど少なく・薄く・大きくして、外ほど粉の密度が下がるようにする
// 粒の位置・大きさ・濃さを答えるだけで、描かない（まとめて描くのは ImpactDustList）
// 遠くの着弾を見えるようにするのが目的（弾痕は遠くでは小さく、薄くなる）
class ImpactDust{
public:
	static const int kParticleNumber = 8; // 1回分の粒の数

	// point: 当たった位置。normal: 当たった面の外向きの法線（長さ1）。どちらも HitWorld::Hit の値をそのまま渡す
	// random: 粒の向き・速さのばらつきに使う。所有権は受け取らない。nullptr 不可
	ImpactDust( const GameLib::Math::Vector3& point, const GameLib::Math::Vector3& normal, std::mt19937* random );

	void update();            // 1ティック進める。終わった物に呼んでも何もしない
	bool isFinished() const;  // 決まった時間がたったか（持ち主が消す）

	const GameLib::Math::Vector3& particlePosition( int index ) const; // 粒 index（0〜kParticleNumber-1）の今の位置（ワールド）
	float particleRadius( int index ) const; // 粒 index の今の板の幅の半分（m。外へ飛ぶ粒ほど大きい）
	float particleOpacity( int index ) const; // 粒 index の今の不透明度（0〜1。外へ飛ぶ粒ほど薄い）
	// 粒 index の板を視線の周りに回す角度（度。出たときに乱数で決め、そのまま回り続けない）
	// 同じ煙のコマ送りの絵が、どの粒も同じ向きに並んで判子を押したように見えないようにする
	float particleAngle( int index ) const;
	float progress() const; // 出てから消えるまでの進み具合（0〜1。煙のコマ送りの絵のコマを決めるのにも使う）

private:
	GameLib::Math::Vector3 mPosition[ kParticleNumber ]; // 粒の位置（ワールド）
	GameLib::Math::Vector3 mVelocity[ kParticleNumber ]; // 粒の速さ（m/ティック）
	float mReach[ kParticleNumber ];                     // 粒の外向き度（0 = 一番遅く中心に残る 〜 1 = 一番速く外へ飛ぶ）
	float mAngle[ kParticleNumber ];                     // 粒の板の向き（度）
	int mAge;                                            // 出てからのティック数
};

#endif
