#ifndef INCLUDED_ZOMBIE_MODEL_COMPONENT_H
#define INCLUDED_ZOMBIE_MODEL_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameLib/GameLib.h"

class ZombieModel;

// ゾンビの形・アニメ・当たり判定の設計図（ZombieModel。同じ種類の全員で共有する）を持つ部品
// （SummaryPlan/個別仕様/設計と分割/ComponentSystem3_Plan.txt の ★2）。
// ゾンビの部品（ZombieState・ZombieChase・ZombieHealth など）は、付けるときに設計図を受け取らず、ここから借りる。
// どの設計図のゾンビにするかを決めるのは、この部品を付ける 1 か所だけになる。付けたときから在るので awake から借りてよい。
class ZombieModelComponent : public Component{
public:
	// model: 所有権は受け取らない。nullptr 不可。この部品より長く生きていること（GameWorld が持つ）。
	explicit ZombieModelComponent( const ZombieModel* model ) : mModel( model ){
		ASSERT( model && "ZombieModelComponent : model is null." );
	}

	const ZombieModel* model() const { return mModel; }

private:
	const ZombieModel* mModel; // 借りているだけ
};

#endif
