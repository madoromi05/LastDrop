#ifndef INCLUDED_LIGHT_COMPONENT_H
#define INCLUDED_LIGHT_COMPONENT_H

#include "GameSrc/World/Component.h"

// 物に付いた明かり（銃の懐中電灯。PbrNightLighting_Plan.txt 9 章。画像の LightComponent）の付いている・消えているだけを持つ
// 始めは消えている。どこを照らすか（銃口の位置と向き）は付いている物を持つ側（HoldComponent::muzzle）、
// 色・強さ・角度は調整値（LightingParams）にあり、ライトの一覧へ足すのはシーン（GameScene::addFlashlight）
class LightComponent : public Component{
public:
	LightComponent() : mOn( false ), mToggledThisTick( false ){}

	// 1 ティックに 1 回呼ぶ。togglePressed なら付ける／消す（押した瞬間）
	void update( bool togglePressed ){
		mToggledThisTick = togglePressed;
		if ( togglePressed ){
			mOn = !mOn;
		}
	}
	bool isOn() const { return mOn; }
	bool toggledThisTick() const { return mToggledThisTick; } // 直前の update で付けた／消したか（スイッチの音を鳴らす側が読む）

private:
	bool mOn;
	bool mToggledThisTick;
};

#endif
