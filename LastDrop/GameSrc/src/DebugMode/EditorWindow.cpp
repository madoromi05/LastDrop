#include "GameSrc/DebugMode/EditorWindow.h"

#ifdef ZF_EDITOR

#include "GameSrc/DebugMode/TuningPanel.h"
#include "GameSrc/DebugMode/DebugSwitches.h"
#include "GameSrc/Scene/Game/World/ZombieDirector.h"
#include "GameSrc/Scene/SceneManager.h"
#include "GameSrc/Core/FixedStepClock.h"
#include "GameSrc/Render/Lighting/DeferredLighting.h"
#include "GameSrc/Render/Lighting/LightingParams.h"
#include "GameSrc/Render/PostProcess/PostProcessParams.h"
#include "GameSrc/Render/Shadow/ShadowParams.h"
#include "GameSrc/Render/PostProcess/SsaoParams.h"
#include "GameSrc/Render/PostProcess/PuddleParams.h"
#include "GameSrc/Render/PostProcess/SsrParams.h"
#include "GameSrc/Render/PostProcess/HiZBuffer.h"
#include "GameSrc/Render/LitQuadBatch.h"
#include "GameSrc/Render/DecalBatch.h"
#include "GameSrc/Render/StaticMesh/StaticMeshInstancing.h"
#include "GameSrc/Map/TownVisibility.h"
#include "GameSrc/Render/Culling/LodSelector.h"
#include "GameSrc/World/GameObjectWorld.h"
#include "GameSrc/Interaction/Item/Inventory.h"
#include "GameSrc/Scene/Game/World/InteractionSystem.h"
#include "GameSrc/Scene/Game/World/GameWorld.h"
#include "GameSrc/Raid/RaidSession.h"
#include "GameSrc/Raid/RaidRecord.h"
#include "GameSrc/Weapon/WeaponCatalog.h"
#include "GameSrc/Math/ScalarMath.h"

#include "GameLib/Graphics/Manager.h"
#include "GameLib/Profiling/Profiler.h"
#include "Framework.h"
#include "GameLib/Input/Manager.h"
#include "GameLib/Input/GamePad.h"
#include "GameLib/Input/InputBuilder.h"
#include "ThirdParty/imgui/imgui.h"
#include <algorithm>
#include <string>

namespace{
	const float kWindowWidthRatio = 0.45f;					// 窓の幅は画面の幅に合わせる。照準（画面の中央）を隠さないよう半分より細くし、大きい画面でも広げすぎない
	const float kMinWindowWidth = 260.f;					// これより細いと数字の行が読めない（画素）
	const float kMaxWindowWidth = 420.f;
	const float kSliderWidthRatio = 0.5f;					// 時間の倍率のスライダーの幅（欄の幅に対して。ラベルとボタンを同じ行に残す）
	const float kMinTimeScale = 0.1f;						// 10 倍のスローまで
	const float kMaxTimeScale = 2.f;
	const float kNormalTimeScale = 1.f;
	const char* const kTimeScaleFormat = "%.2f";
	const int kMaxHiZViewLevel = 11;						// G-Buffer の表示の Hi-Z で見る段のスライダー。上限は 4K の半分（1920）でも 1 画素まで届く段の数（それより上は一番粗い段に丸まる）
	const float kHiZLevelSliderWidth = 80.f;				// チェックボックスと同じ行に並べる幅（画素）
	const ImVec4 kWarningOnColor( 1.f, 0.3f, 0.3f, 1.f );	// 無敵・ゾンビを止めている、の知らせ。見落とさないよう赤くする
	const int kShownGamePad = 0;							// Pad の欄に出すパッド（自キャラを動かすのは 0 番だけ）

	bool gPlayerInvincible = false; // GameScene から受け取った、自キャラが無敵か
	bool gFreeCameraActive = false; // GameScene から受け取った、観察用カメラを使っているか
	const GameObjectWorld* gObjects = nullptr; // GameScene から受け取った、世界に置いた物の持ち主（無ければ nullptr）
	Inventory* gInventory = nullptr; // GameScene から受け取った、自キャラのカバン（無ければ nullptr）
	RaidSession* gRaid = nullptr;    // GameScene から受け取った、1 回の出撃（無ければ nullptr）
	RaidRecord* gRaidRecord = nullptr; // SceneManager::start から受け取った、出撃の間で受け渡す物（起動の後はどのシーンでもある）
	InteractionSystem* gInteraction = nullptr; // GameScene から受け取った、漁れる場所を探す係（無ければ nullptr）
	GameWorld* gWorld = nullptr;     // GameScene から受け取った、今の出撃の世界（今日の条件を切り替える。無ければ nullptr）
	const char* const kRandomConditionName = "random"; // 日の条件の一覧の、固定しない（次の出撃から毎回ランダム）の行

	// F6 で出し入れする物（銃とその弾を並べて置く）
	// ガソリンは 2 マスの持ち帰る物なので、ここでは触らない
	const ItemId kDebugItems[] = { ITEM_AMMO_762, ITEM_RIFLE, ITEM_PISTOL, ITEM_AMMO_9MM, ITEM_BANDAGE };
	// 一度に足す・減らす数。上限は弾を 1 回で十分に持てる数（カバンの空きのほうが先に尽きる）
	const int kMinItemAmount = 1;
	const int kMaxItemAmount = 999;
	const int kWeaponLimit = 1;   // 銃は 1 種類 1 丁（ItemCatalog.h の ITEM_PISTOL の説明）
	const ImVec4 kBagFullColor( 1.f, 0.85f, 0.2f, 1.f ); // 入りきらなかった知らせ（見落とさないよう黄色）

	int gItemAmount = kMinItemAmount; // Amount の欄の値
	// 最後に押した Add が入りきらなかったときの結果（入った数・入れたかった数）。全部入ったら wanted を 0 にして消す
	struct AddResult{
		ItemId item;
		int added;
		int wanted;
	};
	AddResult gLastAdd = { ITEM_NONE, 0, 0 };

	// id を amount 個まで、入る分だけ足す。入った数を返す
	int addItems( ItemId id, int amount ) {
		int added = 0;
		while ( added < amount && gInventory->canAdd( id ) ) {
			gInventory->add( id );
			++added;
		}
		return added;
	}

	// id を amount 個まで、持っている分だけ減らす（手に持っている銃を消すと、HoldComponent が素手にする）
	void removeItems( ItemId id, int amount ) {
		for ( int n = gInventory->count( id ); n > 0 && amount > 0; --n, --amount ) {
			gInventory->remove( id );
		}
	}

	// カバンを全部空にする（ガソリンも含む）。中身を持つマスごとに取り出す
	void clearInventory() {
		const ItemGrid& grid = gInventory->grid();
		for ( int cell = 0; cell < grid.cellCount(); ++cell ) {
			if ( grid.ownerCell( cell ) == cell ) {
				gInventory->takeAll( cell );
			}
		}
	}

	// 1 種類の行（Add・Remove・持っている数）。銃は 1 丁持っていれば Add を押せなくする
	void drawItemRow( ItemId id ) {
		const ItemSpec& spec = itemSpec( id );
		int count = gInventory->count( id );
		bool atLimit = ( spec.kind == ITEM_KIND_WEAPON ) && ( count >= kWeaponLimit );
		ImGui::PushID( id );
		ImGui::BeginDisabled( atLimit );
		if ( ImGui::SmallButton( "Add" ) ) {
			int added = addItems( id, gItemAmount );
			gLastAdd = { id, added, ( added < gItemAmount ) ? gItemAmount : 0 };
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled( count == 0 );
		if ( ImGui::SmallButton( "Remove" ) ) {
			removeItems( id, gItemAmount );
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::Text( "%s: %d", spec.saveName, gInventory->count( id ) );
		ImGui::PopID();
	}

	// カバンの中身を好きに出し入れする欄（拾いに行かずに銃と弾を試すため）
	// 上に行の数・Amount・Clear、その下に 1 種類 1 行。Add / Remove は Amount の数だけ（入る分・持っている分まで）
	void drawInventory() {
		if ( !gInventory ) {
			ImGui::TextDisabled( "No bag." );
			return;
		}
		const ItemGrid& grid = gInventory->grid();
		ImGui::Text( "Cells: %d x %d", grid.columns(), grid.rows() );
		ImGui::SameLine();
		if ( ImGui::SmallButton( "Row -" ) && gInventory->canSetRows( grid.rows() - 1 ) ) {
			gInventory->setRows( grid.rows() - 1 );
		}
		ImGui::SameLine();
		if ( ImGui::SmallButton( "Row +" ) ) {
			gInventory->setRows( grid.rows() + 1 );
		}
		ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x * kSliderWidthRatio );
		if ( ImGui::InputInt( "Amount", &gItemAmount ) ) {
			gItemAmount = ( gItemAmount < kMinItemAmount ) ? kMinItemAmount : ( ( gItemAmount > kMaxItemAmount ) ? kMaxItemAmount : gItemAmount );
		}
		ImGui::SameLine();
		if ( ImGui::SmallButton( "Clear" ) ) {
			clearInventory();
			gLastAdd.wanted = 0;
		}
		for ( ItemId id : kDebugItems ) {
			drawItemRow( id );
		}
		if ( gLastAdd.wanted > 0 ) {
			ImGui::TextColored( kBagFullColor, "Bag full: %s added %d / %d", itemSpec( gLastAdd.item ).saveName, gLastAdd.added, gLastAdd.wanted );
		}
	}

	// 出撃を動かす欄（すぐ脱出・次の出撃の条件。遊んで待たずに試すため）
	void drawRaid() {
		if ( gRaid ) {
			ImGui::Text( "Time: %.0f s   Kills: %d", gRaid->clock().elapsedSeconds(), gRaid->kills() );
			ImGui::SameLine();
			if ( ImGui::SmallButton( "Extract now" ) ) {
				gRaid->forceExtract();
			}
		}
		if ( !gRaidRecord ) {
			if ( !gRaid ) {
				ImGui::TextDisabled( "No raid." );
			}
			return;
		}
		// 日の条件。一覧から選ぶと今の出撃にすぐ効き（GameWorld::changeDayCondition）、
		// 次の出撃（Retry・もう一度）にも引き継ぐ。random は次の出撃から毎回ランダムに戻すだけ（今の出撃の条件はそのまま）
		if ( gWorld ) {
			ImGui::Text( "Today: %s", dayConditionEffect( gWorld->dayConditionKind() ).saveName );
		}
		bool forced = gRaidRecord->hasForcedCondition();
		const char* preview = forced ? dayConditionEffect( gRaidRecord->forcedCondition() ).saveName : kRandomConditionName;
		ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x * kSliderWidthRatio );
		if ( ImGui::BeginCombo( "Condition", preview ) ) {
			if ( ImGui::Selectable( kRandomConditionName, !forced ) ) {
				gRaidRecord->clearForcedCondition();
			}
			for ( int i = 0; i < static_cast< int >( DayCondition::Count ); ++i ) {
				DayCondition condition = static_cast< DayCondition >( i );
				bool selected = forced && ( gRaidRecord->forcedCondition() == condition );
				if ( ImGui::Selectable( dayConditionEffect( condition ).saveName, selected ) ) {
					gRaidRecord->forceCondition( condition );
					if ( gWorld ) {
						gWorld->changeDayCondition( condition );
					}
				}
				if ( selected ) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		if ( gRaid && ImGui::SmallButton( "Retry now" ) ) {
			SceneManager::instance().requestChange( SceneManager::SCENE_GAME );
		}
	}

	// 漁れる場所の欄。開ける秒・空になる確率・包帯の効き目は Tuning の Loot
	void drawLoot() {
		if ( !gInteraction ) {
			ImGui::TextDisabled( "No raid." );
			return;
		}
		if ( ImGui::SmallButton( "Reroll all" ) ) {
			gInteraction->rerollLoot( static_cast< int >( GameLib::Framework::instance().time() ) );
		}
		ImGui::SameLine();
		if ( ImGui::SmallButton( "Mark all searched" ) ) {
			gInteraction->markAllLootSearched();
		}
		ImGui::TextDisabled( "(Debug > Zombie info shows contents above each spot)" );
	}

	const char* tagName( GameObjectTag tag ) {
		switch ( tag ) {
			case GameObjectTag::Player: return "Player";
			case GameObjectTag::Weapon: return "Weapon";
			case GameObjectTag::Zombie: return "Zombie";
			case GameObjectTag::Fuel: return "Fuel";
			case GameObjectTag::AmmoDrop: return "AmmoDrop";
			case GameObjectTag::ItemDrop: return "ItemDrop";
			case GameObjectTag::LootContainer: return "LootContainer";
			case GameObjectTag::EscapeCar: return "EscapeCar";
			case GameObjectTag::FireHydrant: return "FireHydrant";
			case GameObjectTag::StreetLamp: return "StreetLamp";
			case GameObjectTag::SeveredLimb: return "SeveredLimb";
		}
		return "?";
	}

	// 世界に置いた物を作った順に 1 行ずつ（名前・タグ・付いている部品の数）。ゾンビが 30 体いても読めるよう、表にして縦に流す
	void drawObjects() {
		if ( !gObjects ) {
			ImGui::TextDisabled( "No world." );
			return;
		}
		const ImGuiTableFlags kTableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;
		const int kColumnNumber = 3;
		if ( !ImGui::BeginTable( "Objects", kColumnNumber, kTableFlags ) ) {
			return;
		}
		ImGui::TableSetupColumn( "Name" );
		ImGui::TableSetupColumn( "Tag" );
		ImGui::TableSetupColumn( "Parts" );
		ImGui::TableHeadersRow();
		gObjects->forEachObject( []( const GameObject& object ) {
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted( object.name().c_str() );
			ImGui::TableNextColumn();
			ImGui::TextUnformatted( tagName( object.tag() ) );
			ImGui::TableNextColumn();
			ImGui::Text( "%d", object.componentNumber() );
		} );
		ImGui::EndTable();
	}

	// パッドの 0 番の今の入力（どこから読んでいるか・押しているボタン・スティック）
	// DualSense などを対応表（GameLib の GamePadMapping）で Xbox の並びに置き換えているので、押したボタンが正しい名前で出るかを見る
	void drawGamePad() {
		GameLib::Input::GamePad pad = GameLib::Input::Manager::instance().gamePad( kShownGamePad );
		ImGui::Text( "Source: %s", pad.sourceName() );
		std::string pressed;
		for ( int i = 0; i < GameLib::Input::GamePad::BUTTON_MAX; ++i ) {
			if ( !pad.isOn( static_cast< GameLib::Input::GamePad::Button >( i ) ) ) {
				continue;
			}
			// ボタンの名前は GameLib が持つ（割り当ての表示と同じ名前）
			GameLib::Input::BindingRecord record{ GameLib::Input::CollectorId::InGame, GameLib::Input::WorkId::Up, GameLib::Input::DeviceKind::GamePadButton, i };
			pressed += GameLib::Input::BindingDisplayName( record ) + " ";
		}
		ImGui::Text( "Pressed: %s", pressed.c_str() );
		ImGui::Text( "Left  %+.2f %+.2f", pad.stickX( GameLib::Input::GamePad::STICK_LEFT ), pad.stickY( GameLib::Input::GamePad::STICK_LEFT ) );
		ImGui::Text( "Right %+.2f %+.2f", pad.stickX( GameLib::Input::GamePad::STICK_RIGHT ), pad.stickY( GameLib::Input::GamePad::STICK_RIGHT ) );
	}

	// タブの外（窓の一番上）に、遊びの感触を変えている切り替えだけ赤く出す（どのタブを見ていても見落とさないように）
	void drawActiveCheats() {
		if ( gPlayerInvincible ) {
			ImGui::TextColored( kWarningOnColor, "Invincible: ON" );
		}
		if ( zombiesFrozen() ) {
			if ( gPlayerInvincible ) {
				ImGui::SameLine();
			}
			ImGui::TextColored( kWarningOnColor, "Zombies frozen" );
		}
	}

	// 確認用の切り替え。本編へは DebugSwitches・zombiesFrozen を通して頼む
	// 頼み事（無敵・観察用カメラ・ゾンビを出す）は出撃の世界があるときだけ押せる（ロード中・タイトルで押した頼みが、後の出撃で急に効かないように）
	void drawDebug() {
		DebugSwitches& switches = debugSwitches();
		bool inRaid = ( gWorld != nullptr );
		if ( !inRaid ) {
			ImGui::TextDisabled( "No raid: player / zombie buttons are off." );
		}
		ImGui::SeparatorText( "Player" );
		ImGui::BeginDisabled( !inRaid );
		// チェックは今の状態を映すだけ。押したら切り替えを頼み、持ち主（自キャラの体力・観察用カメラ）が次の描画で切り替える
		bool invincible = gPlayerInvincible;
		if ( ImGui::Checkbox( "Invincible", &invincible ) ) {
			switches.toggleInvincible = true;
		}
		bool freeCamera = gFreeCameraActive;
		if ( ImGui::Checkbox( "Free camera", &freeCamera ) ) {
			switches.toggleFreeCamera = true;
		}
		ImGui::SameLine();
		ImGui::TextDisabled( "(WASD, Space up, Ctrl down, Shift fast)" );
		ImGui::EndDisabled();

		ImGui::SeparatorText( "Zombies" );
		// 止めている間は、考える・動く・アニメ・攻撃・湧くを全部止める（撃てば体力は減り、倒れるのは戻してから）
		ImGui::Checkbox( "Freeze zombies", &zombiesFrozen() );
		ImGui::SameLine();
		ImGui::TextDisabled( "(AI, animation, attacks, spawning)" );
		ImGui::BeginDisabled( !inRaid );
		if ( ImGui::Button( "Spawn zombie" ) ) {
			switches.spawnZombies = true;
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::TextDisabled( "(8 m ahead, facing you)" );

		ImGui::SeparatorText( "Show" );
		ImGui::Checkbox( "Collision", &switches.showCollision );
		ImGui::SameLine();
		ImGui::TextDisabled( "(+ ray from screen center)" );
		ImGui::Checkbox( "Pickups", &switches.showPickups );
		ImGui::SameLine();
		ImGui::TextDisabled( "(boxes through walls)" );
		ImGui::Checkbox( "Zombie info", &switches.showZombieInfo );
		ImGui::SameLine();
		ImGui::TextDisabled( "(health, paths, spawner, stuck marks)" );
		ImGui::Checkbox( "NavMesh", &switches.showNavMesh );
		ImGui::SameLine();
		ImGui::TextDisabled( "(floor zombies can walk on)" );
		ImGui::Checkbox( "Town info", &switches.showTownInfo );
		ImGui::SameLine();
		ImGui::TextDisabled( "(lights, PVS, loot)" );
		ImGui::Checkbox( "Perf numbers", &GameLib::Profiling::overlayVisible() );
		ImGui::SameLine();
		ImGui::TextDisabled( "(top right, while F6 is closed)" );
	}

	void drawScene() {
		FixedStepClock& clock = SceneManager::instance().clock();
		bool paused = clock.isPaused();
		if ( ImGui::Checkbox( "Pause", &paused ) ) {
			clock.setPaused( paused );
		}
		ImGui::SameLine();
		ImGui::BeginDisabled( !paused );
		if ( ImGui::Button( "Step 1 tick" ) ) {
			clock.requestStep();
		}
		ImGui::EndDisabled();
		float scale = clock.timeScale();
		ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x * kSliderWidthRatio );
		if ( ImGui::SliderFloat( "Time scale", &scale, kMinTimeScale, kMaxTimeScale, kTimeScaleFormat ) ) {
			clock.setTimeScale( scale );
		}
		ImGui::SameLine();
		if ( ImGui::Button( "1x" ) ) {
			clock.setTimeScale( kNormalTimeScale );
		}
	}

	// チェックボックスと、その右の薄い字のひと言（何が切り替わるか・どこで見るか）を 1 行に並べる
	void toggleWithHint( const char* label, bool* value, const char* hint ) {
		ImGui::Checkbox( label, value );
		ImGui::SameLine();
		ImGui::TextDisabled( "%s", hint );
	}

	// 列挙型の値を 1 つ選ぶコンボ。Enum::Count が値の数、nameOf が値の名前
	template< class Enum > void enumCombo( const char* label, Enum* value, const char* ( *nameOf )( Enum ) ) {
		if ( ImGui::BeginCombo( label, nameOf( *value ) ) ) {
			for ( int i = 0; i < static_cast< int >( Enum::Count ); ++i ) {
				Enum candidate = static_cast< Enum >( i );
				if ( ImGui::Selectable( nameOf( candidate ), candidate == *value ) ) {
					*value = candidate;
				}
			}
			ImGui::EndCombo();
		}
	}

	// 結果画面の確認（Scene タブ）。出撃を最後まで遊ばずに、脱出成功・ゲームオーバーの見た目を確かめる
	const int kPreviewStartSeconds = 754;	// 初めの値。12:34 で、分も秒も 2 桁になる
	const int kPreviewStartKills = 23;
	const int kMinPreviewValue = 0;			// 時間が負だと ResultText::time が止まる

	int gPreviewSeconds = kPreviewStartSeconds;
	int gPreviewKills = kPreviewStartKills;
	DayCondition gPreviewCondition = DayCondition::Normal;

	const char* dayConditionName( DayCondition condition ) {
		return dayConditionEffect( condition ).saveName;
	}

	// 入力した値で最後の結果を置き換え、結果画面へ切り替える。切り替えは今のシーンの処理が終わってから（SceneManager::requestChange）
	void showResult( RaidEnding ending ) {
		RaidResult result;
		result.ending = ending;
		result.seconds = static_cast< float >( gPreviewSeconds );
		result.kills = gPreviewKills;
		result.condition = gPreviewCondition;
		gRaidRecord->setLastResult( result );
		SceneManager::instance().requestChange( SceneManager::SCENE_RAID_END );
	}

	// タイトル・出撃中・結果画面のどこからでも開ける。結果画面で押し直すと、その値で作り直す
	void drawResultPreview() {
		ImGui::SeparatorText( "Result screen" );
		ImGui::PushID( "ResultPreview" );	// World タブの Raid の欄と同じ名前のコンボを置くため
		ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x * kSliderWidthRatio );
		if ( ImGui::InputInt( "Time (s)", &gPreviewSeconds ) ) {
			gPreviewSeconds = std::max( gPreviewSeconds, kMinPreviewValue );
		}
		ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x * kSliderWidthRatio );
		if ( ImGui::InputInt( "Kills", &gPreviewKills ) ) {
			gPreviewKills = std::max( gPreviewKills, kMinPreviewValue );
		}
		ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x * kSliderWidthRatio );
		enumCombo( "Condition", &gPreviewCondition, dayConditionName );
		if ( ImGui::Button( "Show escaped" ) ) {
			showResult( RaidEnding::Escaped );
		}
		ImGui::SameLine();
		if ( ImGui::Button( "Show game over" ) ) {
			showResult( RaidEnding::Killed );
		}
		ImGui::PopID();
	}

	void drawVSyncAndLightingPreset() {
		GameLib::Graphics::Manager graphics = GameLib::Graphics::Manager::instance();
		bool vsync = graphics.isVSync();
		if ( ImGui::Checkbox( "VSync", &vsync ) ) {
			graphics.setVSync( vsync );
		}
		ImGui::TextDisabled( "Turn VSync off to see CPU / GPU bound." );
		// 町全体の光を昼・夜の値へ入れ替える（見比べ・不具合探し用。遊びは夜）
		// 細かい値は Tuning の LightingParams で動かす
		if ( ImGui::Button( "Night" ) ) {
			lightingParams() = LightingParams();
		}
		ImGui::SameLine();
		if ( ImGui::Button( "Day" ) ) {
			lightingParams() = dayLightingParams();
		}
		ImGui::SameLine();
		ImGui::TextDisabled( "(lighting preset)" );
	}

	// 描く物を減らす仕組みの切り替え。同じ場面で切り替えて、絵と回数・GPU の時間を見比べる
	void drawGeometryToggles() {
		toggleWithHint( "Shadows", &shadowsEnabled(), "(flashlight + moon cascades)" );	// 影を描くか。時間は gpu.Shadow
		toggleWithHint( "Instancing", &instancingEnabled(), "(buildings + props)" );	// 建物と置物を形ごとにまとめて描くか。切ると 1 つずつ描く。回数は Draw.gbuffer・Draw.shadow、時間は gpu.Buildings・gpu.Props
		toggleWithHint( "PVS", &pvsEnabled(), "(hidden buildings + zombies)" );			// 簡易 PVS で、建物の向こうに隠れた建物・ゾンビを描かないか。回数は Pvs.building・Pvs.zombie
		toggleWithHint( "LOD", &lodEnabled(), "(car, fuel cans)" );						// 置物を遠くで粗い形（LOD）で描くか。境目は Tuning の Lod。回数は Lod.1・Lod.2・Draw.triangles
		toggleWithHint( "Light culling", &lightCullingEnabled(), "(off-screen + out of range)" );	// 届かないライトを計算しないか。回数は Light.selected、時間は gpu.Lighting
	}

	// 画面全体のパスと、見た目の仕上げの切り替え
	void drawEffectToggles() {
		toggleWithHint( "Fog", &fogEnabled(), "(distance + height)" );	// 霧を掛けるか。時間は gpu.Fog
		// 空の月の絵。値は Tuning の LightingParams（moonRadius〜moonHaloFalloff）
		// 切ると今までのグラデーションだけの空（水たまりの月も消える）。時間は gpu.Lighting
		toggleWithHint( "Sky detail", &skyDetailEnabled(), "(moon)" );
		// SSAO を作るか。絵は G-Buffer の表示の SSAO で見る。時間は gpu.SSAO
		ImGui::Checkbox( "SSAO", &ssaoEnabled() );
		ImGui::SameLine();
		toggleWithHint( "SSAO blur", &ssaoBlurEnabled(), "(view: G-Buffer > SSAO)" );	// ぼかしを切ると、点の並びを回したざらつきがそのまま見える（調整用）
		// 映り込み。値は Tuning の SSR。切ると空と地面の色の映り込みに戻る。時間は gpu.SSR・gpu.SSR composite
		toggleWithHint( "SSR", &ssrEnabled(), "(view: G-Buffer > SSR / SSR color)" );
		// Hi-Z。切ると今までの直線の進み方（段階12 の計測が終わったら直線は消す）。時間は gpu.Hi-Z・gpu.SSR
		ImGui::Checkbox( "Hi-Z", &hiZEnabled() );
		ImGui::SameLine();
		ImGui::SetNextItemWidth( kHiZLevelSliderWidth );
		ImGui::SliderInt( "Hi-Z view level", &hiZViewLevel(), 0, kMaxHiZViewLevel );
		ImGui::SameLine();
		ImGui::TextDisabled( "(view: G-Buffer > Hi-Z)" );
		// 粗い面のぼけた映り込み。切ると鏡の向きだけ・ぼかしも今までのまま。TAA を切るとざらつくのが正しい
		toggleWithHint( "Stochastic SSR", &ssrStochasticEnabled(), "(roughness 0.4-0.7, Tuning > SSR maxRoughness)" );
		// 雨上がりの水たまり。値は Tuning の Puddle。G-Buffer の表示 Roughness で黒い形に見える
		toggleWithHint( "Puddles", &puddlesEnabled(), "(road only, view: G-Buffer > Roughness)" );
		toggleWithHint( "Decals", &decalsEnabled(), "(bullet holes)" );								// 弾痕のデカール。切ると弾痕が描かれない（板の弾痕に戻すのではない）。時間は gpu.Decals
		toggleWithHint( "Soft particles", &softParticlesEnabled(), "(dust, blood, muzzle flash)" );	// 半透明の板を壁・地面に刺さった所で薄くするか。切ると板の縁の線が見える
	}

	void drawColorControls() {
		// カラーグレーディングとディザ。値は Tuning の PostProcessParams。縞は Dither を切ると見える
		ImGui::Checkbox( "Color grading", &colorGradingEnabled() );
		ImGui::SameLine();
		ImGui::Checkbox( "Dither", &ditherEnabled() );
		// 色の表（LUT）。Export で今の調整を Resource/UI/lut_export.tga に焼き、外の道具で直して
		// Resource/UI/lut.tga の名前で置き、Reload で読む。強さは Tuning の PostProcessParams の lutStrength と同じ値
		ColorLutControl& lut = colorLutControl();
		if ( ImGui::Button( "Export LUT" ) ) {
			lut.exportRequested = true;
		}
		ImGui::SameLine();
		if ( ImGui::Button( "Reload LUT" ) ) {
			lut.reloadRequested = true;
		}
		ImGui::SameLine();
		ImGui::TextDisabled( lut.loaded ? "(LUT: lut.tga 1024x32)" : "(LUT: none)" );
		ImGui::BeginDisabled( !lut.loaded );
		ImGui::SliderFloat( "LUT strength", &postProcessParams().lutStrength, 0.f, 1.f );
		ImGui::EndDisabled();
	}

	void drawViewSelectors() {
		// アンチエイリアス。Off / FXAA / TAA から 1 つ。FXAA の値は Tuning の PostProcessParams（fxaa〜）
		// TAA は描く行列を毎フレームずらし、霧の後に履歴と混ぜる（値は Tuning の TAA。時間は gpu.TAA）
		enumCombo( "Anti-aliasing", &antiAliasingMode(), antiAliasingName );
		enumCombo( "G-Buffer", &gbufferView(), gbufferViewName );	// G-Buffer の中身の表示。Final 以外を選ぶと、ライティングの代わりに中身を画面に出す
	}

	// Render タブ。上から、VSync と光の入れ替え → 描く物を減らす仕組み → 画面全体のパス → 色 → 表示の選択
	void drawRender() {
		drawVSyncAndLightingPreset();
		drawGeometryToggles();
		drawEffectToggles();
		drawColorControls();
		drawViewSelectors();
	}

	// World タブ（世界の中身を触る 4 つの欄。どれも長くなるので、タブの中では見出しで開け閉めする）
	void drawWorld() {
		if ( ImGui::CollapsingHeader( "Objects" ) ) {
			drawObjects();
		}
		if ( ImGui::CollapsingHeader( "Inventory", ImGuiTreeNodeFlags_DefaultOpen ) ) {
			drawInventory();
		}
		if ( ImGui::CollapsingHeader( "Raid", ImGuiTreeNodeFlags_DefaultOpen ) ) {
			drawRaid();
		}
		if ( ImGui::CollapsingHeader( "Loot" ) ) {
			drawLoot();
		}
	}
}

namespace EditorWindow{

void draw() {
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	float width = viewport->WorkSize.x * kWindowWidthRatio;
	width = ScalarMath::clamp( width, kMinWindowWidth, kMaxWindowWidth );
	// 右端に上から下まで固定する（窓を動かしたり大きさを変えたりしない。imgui.ini も書かない）
	ImGui::SetNextWindowPos( ImVec2( viewport->WorkPos.x + viewport->WorkSize.x - width, viewport->WorkPos.y ) );
	ImGui::SetNextWindowSize( ImVec2( width, viewport->WorkSize.y ) );
	const ImGuiWindowFlags kFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
	if ( ImGui::Begin( "Editor (F6)", nullptr, kFlags ) ) {
		ImGui::TextDisabled( "Hold Alt to use the mouse here." );	// 開いたままでも遊べるので、窓を触る方法を一番上に出す（GameLib/DebugUi の入力の受け渡し）
		drawActiveCheats();											// どのタブを見ていても見えるよう、タブの外に置く
		// タブで欄を切り替える。並びは使う回数の多い順（確認用の切り替え → 時間 → 絵 → 計測 → 調整値 → 世界の中身 → パッド）
		// 窓は毎回同じ名前で作るので、ImGui が選んでいるタブを覚えている（imgui.ini は書かないので、起動し直すと Debug に戻る）
		if ( ImGui::BeginTabBar( "EditorTabs", ImGuiTabBarFlags_FittingPolicyScroll ) ) {
			if ( ImGui::BeginTabItem( "Debug" ) ) {
				drawDebug();
				ImGui::EndTabItem();
			}
			if ( ImGui::BeginTabItem( "Scene" ) ) {
				drawScene();
				drawResultPreview();
				ImGui::EndTabItem();
			}
			if ( ImGui::BeginTabItem( "Render" ) ) {
				drawRender();
				ImGui::EndTabItem();
			}
			if ( ImGui::BeginTabItem( "Perf" ) ) {
				GameLib::Profiling::drawPerfPanel();
				ImGui::EndTabItem();
			}
			// 調整値（自キャラ・銃・視点・ゾンビの群れ）
			if ( ImGui::BeginTabItem( "Tuning" ) ) {
				TuningPanel::draw();
				ImGui::EndTabItem();
			}
			if ( ImGui::BeginTabItem( "World" ) ) {
				drawWorld();
				ImGui::EndTabItem();
			}
			if ( ImGui::BeginTabItem( "Pad" ) ) {
				drawGamePad();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	ImGui::End();
}

void setPlayerInvincible( bool invincible ) {
	gPlayerInvincible = invincible;
}

void setFreeCameraActive( bool active ) {
	gFreeCameraActive = active;
}

void setObjects( const GameObjectWorld* objects ) {
	gObjects = objects;
}

void setInventory( Inventory* inventory ) {
	gInventory = inventory;
}

void setRaid( RaidSession* session ) {
	gRaid = session;
}

void setRaidRecord( RaidRecord* record ) {
	gRaidRecord = record;
}

void setInteraction( InteractionSystem* interaction ) {
	gInteraction = interaction;
}

void setWorld( GameWorld* world ) {
	gWorld = world;
}

} //namespace EditorWindow

#else // ZF_EDITOR

namespace EditorWindow{

void draw() {
}

void setPlayerInvincible( bool ) {
}

void setFreeCameraActive( bool ) {
}

void setObjects( const GameObjectWorld* ) {
}

void setInventory( Inventory* ) {
}

void setRaid( RaidSession* ) {
}

void setRaidRecord( RaidRecord* ) {
}

void setInteraction( InteractionSystem* ) {
}

void setWorld( GameWorld* ) {
}

} //namespace EditorWindow

#endif // ZF_EDITOR
