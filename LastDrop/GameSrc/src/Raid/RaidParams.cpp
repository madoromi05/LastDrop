#include "GameSrc/Raid/RaidParams.h"

RaidParams& raidParams() {
	static RaidParams sParams; // 起動中ずっと 1 つ（F6 で書き換える）
	return sParams;
}
