#include "GameSrc/Scene/GameResources.h"
#include "GameSrc/Map/TownEditFile.h"
#include "GameLib/GameLib.h"

GameResources::GameResources() :
	mMusic( &mAudio ) {
	// 前に保存した設定があれば使う（無ければ既定値）。音量はここで一度流しておく（設定画面を開かなくても効くように）
	mSettings.load();
	mSettings.applyAudio( mAudio );
	// 町の手直しがあれば読む（無ければ町はそのまま）。使わない行の理由は町を組み立てるときに出る。読めなかった行の理由はここで出す
	// 読めなかった行の理由は F6 の Town タブにも出すので残す（その行は手直しに無いので、Bake が黙ってファイルを書き直さないように）
	std::vector< std::string > warnings;
	TownEditFile::load( TownEditFile::kPath, &mTownEdits, &warnings );
	for ( const std::string& warning : warnings ) {
		GameLib::cout << "TownEdit: " << warning.c_str() << GameLib::endl;
	}
	mTownEditFile.loaded( mTownEdits, warnings );
}

MeshCache& GameResources::meshes() {
	return mMeshes;
}

AudioEngine& GameResources::audio() {
	return mAudio;
}

MusicPlayer& GameResources::music() {
	return mMusic;
}

UiRenderer& GameResources::uiRenderer() {
	return mUiRenderer;
}

GameSettings& GameResources::settings() {
	return mSettings;
}

RaidRecord& GameResources::raidRecord() {
	return mRaidRecord;
}

TownCache& GameResources::town() {
	return mTown;
}

TownEdits& GameResources::townEdits() {
	return mTownEdits;
}

TownEditFileState& GameResources::townEditFile() {
	return mTownEditFile;
}
