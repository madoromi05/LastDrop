#ifndef INCLUDED_IHITTABLE_H
#define INCLUDED_IHITTABLE_H

class HitboxSet;

// 弾が当たりうるもの（主人公・ゾンビ）。部位ごとの当たり判定を差し出す
// HitWorld が命中の問い合わせと当たり判定の表示に使う
class IHittable{
public:
	virtual ~IHittable(){}
	// 部位ごとの当たり判定。返す参照の寿命はこのオブジェクトと同じ
	virtual const HitboxSet& hitboxes() const = 0;
};

#endif
