#include "GameSrc/Interaction/LootParams.h"

LootParams& lootParams() {
	static LootParams sParams; // 起動中ずっと 1 つ（F6 で書き換える）
	return sParams;
}
