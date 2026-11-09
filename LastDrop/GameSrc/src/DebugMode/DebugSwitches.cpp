#include "GameSrc/DebugMode/DebugSwitches.h"

DebugSwitches& debugSwitches() {
	static DebugSwitches sSwitches; // 起動中ずっと 1 つ（F6 で書き換える）
	return sSwitches;
}
