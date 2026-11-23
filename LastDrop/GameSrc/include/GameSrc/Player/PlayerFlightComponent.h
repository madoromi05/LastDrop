#ifndef INCLUDED_PLAYER_FLIGHT_COMPONENT_H
#define INCLUDED_PLAYER_FLIGHT_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Player/PlayerCommand.h"
#include "GameLib/Math/Vector3.h"

// 自キャラを飛ばす確認用の部品（町やゾンビを好きな所から見て回る。Debug・Profile ビルドだけ PlayerFactory が付ける）
// 飛んでいる間は、歩く代わりに次の動きで体ごと進む（目もそのまま付いて来る）：
//   W/S は視線の向きそのまま（見下ろして W なら降りていく）、A/D は水平、Space で上がり Ctrl で下がる。Shift を押している間はさらに速い
//   壁・地面はすり抜ける。撃つ・覗く・E で使う・カバンは歩いているときと同じ
// 飛ぶ・やめるは F6 の Debug タブの Fly（DebugSwitches::toggleFlight を GameEditorBridge が読んで toggle を呼ぶ）
// やめるとその場から地面の高さへ少しずつ降りる（PlayerComponent の歩く移動が CharacterMover::followGround で地面へ寄せる）
// 動かすのは PlayerComponent（この部品は自分では動かず、進む量を返すだけ）
class PlayerFlightComponent : public Component{
public:
	PlayerFlightComponent();

	void toggle();
	bool isFlying() const;
	// このティックに進む量（m）。command の前後・横・上下と、カメラの向き（度。FirstPersonCamera::yaw / pitch）から
	GameLib::Math::Vector3 step( const PlayerCommand& command, float cameraYaw, float cameraPitch ) const;

private:
	bool mFlying;
};

#endif
