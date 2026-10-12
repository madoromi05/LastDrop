#include "GameResources.h"

GameResources::GameResources() {
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
