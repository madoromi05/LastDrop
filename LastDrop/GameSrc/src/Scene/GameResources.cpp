#include "GameSrc/Scene/GameResources.h"
#include "GameSrc/Raid/RunRules.h"

GameResources::GameResources() {
	// 前に保存した設定があれば使う（無ければ既定値）。音量はここで一度流しておく（設定画面を開かなくても効くように）。
	mSettings.load();
	mSettings.applyAudio( mAudio );
	// 5 日間の続き（save.xml）。無ければ 1 日目の始めの倉庫で始める（R2_Plan.txt Q16）。
	RunRules::loadOrStart( &mRun );
}

MeshCache& GameResources::meshes() {
	return mMeshes;
}

AudioEngine& GameResources::audio() {
	return mAudio;
}

UiRenderer& GameResources::uiRenderer() {
	return mUiRenderer;
}

GameSettings& GameResources::settings() {
	return mSettings;
}

RunState& GameResources::run() {
	return mRun;
}

TownCache& GameResources::town() {
	return mTown;
}
