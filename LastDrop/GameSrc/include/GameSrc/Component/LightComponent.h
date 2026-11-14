#ifndef INCLUDED_LIGHT_COMPONENT_H
#define INCLUDED_LIGHT_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Component/LightPreset.h"

class ILightSocket;

// 物に付いた明かり（銃の懐中電灯・街灯。画像の LightComponent。SummaryPlan/個別仕様/設計と分割/LightComponent_Plan.txt）
// この物の光を 1 つ答える。持つのは、光の種類・付いているか・光の出る所を答える相手だけ
// 光の出る所は、持つ側が渡した相手（ILightSocket）に聞く。渡されていなければ、同じ物の TransformComponent の位置と向き
// どちらも無ければ光らない。持たれている間だけ照らす種類（懐中電灯。LightPresets::needsSocket）は、相手が無ければ光らない
// 色・強さ・届く距離は、種類ごとに調整値から作る（LightPresets::make）
// 影のマップを付ける・ライトの一覧へ足すのはシーン（SceneLights が全部の LightComponent をたどる）
// 外せる部品（外すと、その物が光らなくなるだけ）
class LightComponent : public Component{
public:
	explicit LightComponent( LightPreset preset );	// 始めに付いているかは種類で決まる（LightPresets::startsOn）

	void update( bool togglePressed );				// 1 ティックに 1 回呼ぶ。togglePressed なら付ける／消す（押した瞬間）
	bool isOn() const;
	bool toggledThisTick() const;					// 直前の update で付けた／消したか（スイッチの音を鳴らす側が読む）
	// 光の出る所を答える相手を渡す（手に持った側が呼ぶ）。nullptr で外す。所有権は受け取らない。外すまで生きていること
	void setSocket( const ILightSocket* socket );
	LightPreset preset() const;
	bool castsShadow() const;						// 影を落とす種類か
	bool lightSource( LightSource* out ) const;		// 今の光。消えている・光の出る所が無ければ false（out は変えない）。out: nullptr 不可

private:
	LightPreset mPreset;
	bool mOn;
	bool mToggledThisTick;
	const ILightSocket* mSocket;	// 借りているだけ。nullptr なら同じ物の TransformComponent から照らす（持たれている間だけ照らす種類は照らさない）
};

#endif
