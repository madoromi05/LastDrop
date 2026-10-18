#ifndef INCLUDED_IDAMAGEABLE_H
#define INCLUDED_IDAMAGEABLE_H

struct DamageInfo;

// ダメージを受けられるもの（ゾンビ。後で自キャラも）。
// 当たった後にどう減るか（部位の倍率・体力・倒れるか）は受ける側だけが知っている。渡す側（GameWorld）は量と部位を渡すだけ。
class IDamageable{
public:
	virtual ~IDamageable(){}
	virtual void applyDamage( const DamageInfo& damage ) = 0;
};

#endif
