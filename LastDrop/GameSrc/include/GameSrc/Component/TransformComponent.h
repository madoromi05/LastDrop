#ifndef INCLUDED_TRANSFORM_COMPONENT_H
#define INCLUDED_TRANSFORM_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Math/AngleMath.h"
#include "GameLib/Math/Vector3.h"

// 物の置き場所（足元の位置）と向き（Y 軸まわりの度。0 は +Z）。ほかの部品はこれを読み・書きする
// 親子（Unity の Transform の親子）は持たない。手に持つ銃は、持つ側（HoldComponent）が手の骨から置き場所を求める
// 見た目の部品（StaticModelComponent・SkinnedModelComponent）へ写すのは、動かした部品の役目（いつ写すかで絵の遅れが決まるため）
class TransformComponent : public Component{
public:
	TransformComponent( const GameLib::Math::Vector3& position, float angleY ) :
		mPosition( position ),
		mAngleY( AngleMath::wrapSigned( angleY ) ){}

	const GameLib::Math::Vector3& position() const { return mPosition; }
	// 書き換える部品が、今の置き場所を引数（Vector3*）でそのまま渡せるように、書ける参照も返す
	GameLib::Math::Vector3& position(){ return mPosition; }
	float angleY() const { return mAngleY; }
	float& angleY(){ return mAngleY; }

private:
	GameLib::Math::Vector3 mPosition;
	float mAngleY;
};

#endif
