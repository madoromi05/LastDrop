#include "GameSrc/Scene/SceneManager.h"
#include "GameSrc/Scene/IScene.h"
#include "GameSrc/Scene/GameResources.h"
#include "GameSrc/Scene/Game/GameScene.h"
#include "GameSrc/Scene/Result/RaidEndScene.h"
#include "GameSrc/Scene/Menu/MenuScene.h"
#include "GameSrc/Scene/Menu/MenuCatalog.h"
#include "GameSrc/Core/Pad.h"
#include "GameLib/GameLib.h"
#include "GameLib/Profiling/Profiler.h"

SceneManager::SceneManager() :
	mHasRequest( false ),
	mRequestedId( SCENE_TITLE ) {
}

SceneManager::~SceneManager() {
	destroy();
}

SceneManager& SceneManager::instance() {
	static SceneManager s;
	return s;
}

void SceneManager::start( SceneId first ) {
	ASSERT( !mResources && "SceneManager::start : already started." );
	mResources = std::make_unique< GameResources >();
	change( first );
}

std::unique_ptr< IScene > SceneManager::createScene( SceneId id ) const {
	// シーンを増やすときは SceneId(SceneManager.h) とこの分岐の2箇所だけ直せばよい
	switch ( id ) {
	case SCENE_TITLE:
		return std::make_unique< MenuScene >( mResources.get(), MenuCatalog::title() );
	case SCENE_GAME:
		return std::make_unique< GameScene >( mResources.get() );
	case SCENE_RAID_END:
		return std::make_unique< RaidEndScene >( mResources.get() );
	default:
		ASSERT( false && "SceneManager::createScene : unknown SceneId." );
		return nullptr;
	}
}

const char* SceneManager::loadName( SceneId id ) {
	switch ( id ) {
	case SCENE_TITLE:    return "Title";
	case SCENE_GAME:     return "Game";
	case SCENE_RAID_END: return "RaidEnd";
	default:             return "Unknown";
	}
}

void SceneManager::change( SceneId id ) {
	ASSERT( mResources && "SceneManager::change : call start first." );
	GameLib::Profiling::beginLoad( loadName( id ) );	// 切り替えからシーンの最初の絵までを、1 回のロードとして測る（起動中なら起動のロードに含める）
	// 新しいシーンを作り終えてから、前のシーンを消す（作る途中で前のシーンが消えていることが無い）
	std::unique_ptr< IScene > next = createScene( id );
	mCurrent = std::move( next );
	mClock.reset();										// 前のシーンで経った時間を、新しいシーンの更新に持ち込まない
}

void SceneManager::requestChange( SceneId id ) {
	mHasRequest = true;
	mRequestedId = id;
}

void SceneManager::update() {
	if ( !mCurrent ) {
		return;
	}
	// 入力は1回の描画につき1回だけ、どのシーンでもここで読む（update の回数に左右されないよう、beginFrame の前）
	{
		PROFILE_SCOPE( "Pad" );
		Pad::instance()->update();
	}
	// 計測の CSV の記録を始める／止めるのは F6 の Perf タブのボタン
	{
		PROFILE_SCOPE( "Scene.beginFrame" );
		mCurrent->beginFrame();
	}
	int ticks = mClock.ticksToRun();
	GameLib::Profiling::setTicks( ticks );
	for ( int i = 0; i < ticks; ++i ) {
		PROFILE_SCOPE( "Scene.update" );
		mCurrent->update();
	}
	{
		PROFILE_SCOPE( "Scene.draw" );
		mCurrent->draw();
	}
	// 操作できる絵を描いたら、ロードを閉じる（閉じるのはこのフレームの Present の後）
	if ( !mCurrent->isLoading() ) {
		GameLib::Profiling::endLoad();
	}
	// 予約された切り替えは、今のシーンの処理が全部終わってから行う（シーンの途中で自分自身を消さないように）
	if ( mHasRequest ) {
		mHasRequest = false;
		change( mRequestedId );
	}
}

FixedStepClock& SceneManager::clock() {
	return mClock;
}

void SceneManager::destroy() {
	// シーンが借りている物を先に消さないよう、シーン → 使い回す物 の順に消す
	mCurrent.reset();
	mResources.reset();
}
