#include "GameConfig.h"
#include <fstream>
#include <string>

namespace {
	// 設定ファイル名（入力設定 input_p*.xml とは別系統。ここで一元管理する）。
	const char* const kConfigPath        = "gameconfig.txt";
	const char* const kKeyLandOnContact  = "landOnContact";
}

GameConfig::GameConfig() : mLandOnContact( false ) {} // 既定は「継続」。

bool GameConfig::landOnContact() const { return mLandOnContact; }
void GameConfig::setLandOnContact( bool land ) { mLandOnContact = land; }

void GameConfig::save() const {
	// 人が読めるキー値テキストで保存する。GameLib::FileIO は通さない
	// （設定ファイルの欠如や書込み失敗でゲームを止めないため）。
	std::ofstream ofs( kConfigPath );
	if (!ofs) { return; }
	ofs << kKeyLandOnContact << ' ' << ( mLandOnContact ? 1 : 0 ) << '\n';
}

bool GameConfig::load() {
	std::ifstream ifs( kConfigPath );
	if (!ifs) { return false; } // ファイルが無ければ既定値のまま。
	std::string key;
	while (ifs >> key) {
		if (key == kKeyLandOnContact) {
			int value = 0;
			ifs >> value;
			mLandOnContact = ( value != 0 );
		}
	}
	return true;
}
