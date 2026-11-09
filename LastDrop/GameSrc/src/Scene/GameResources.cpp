#include "GameSrc/Scene/GameResources.h"

GameResources::GameResources() {
	// 前に保存した設定があれば使う（無ければ既定値）。音量はここで一度流しておく（設定画面を開かなくても効くように）。
	mSettings.load();
	mSettings.applyAudio( mAudio );
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

RaidRecord& GameResources::raidRecord() {
	return mRaidRecord;
}

TownCache& GameResources::town() {
	return mTown;
}
