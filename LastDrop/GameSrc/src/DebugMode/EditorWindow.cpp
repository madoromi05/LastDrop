#include "GameSrc/DebugMode/EditorWindow.h"

#ifdef ZF_EDITOR

#include "GameSrc/DebugMode/TuningPanel.h"
#include "GameSrc/Scene/SceneManager.h"
#include "GameSrc/Core/FixedStepClock.h"
#include "GameSrc/Render/DeferredLighting.h"
#include "GameSrc/Render/LightingParams.h"
#include "GameSrc/Render/PostProcessParams.h"
#include "GameSrc/Render/ShadowParams.h"
#include "GameSrc/Render/SsaoParams.h"
#include "GameSrc/Render/PuddleParams.h"
#include "GameSrc/Render/SsrParams.h"
#include "GameSrc/Render/HiZBuffer.h"
#include "GameSrc/Render/LitQuadBatch.h"
#include "GameSrc/Render/DecalBatch.h"
#include "GameSrc/Render/StaticMesh/StaticMeshInstancing.h"
#include "GameSrc/Map/TownVisibility.h"
#include "GameSrc/Render/LodSelector.h"
#include "GameSrc/World/GameObjectWorld.h"
#include "GameSrc/Interaction/Inventory.h"
#include "GameSrc/Raid/RaidSession.h"
#include "GameSrc/Raid/RunState.h"
#include "GameSrc/Raid/RunRules.h"
#include "GameSrc/Weapon/WeaponCatalog.h"

#include "GameLib/Graphics/Manager.h"
#include "GameLib/Profiling/Profiler.h"
#include "GameLib/Input/Manager.h"
#include "GameLib/Input/GamePad.h"
#include "GameLib/Input/InputBuilder.h"
#include "ThirdParty/imgui/imgui.h"
#include <string>

namespace{
	// 窓の幅は画面の幅に合わせる。照準（画面の中央）を隠さないよう半分より細くし、大きい画面でも広げすぎない。
	const float kWindowWidthRatio = 0.45f;
	const float kMinWindowWidth = 260.f;  // これより細いと数字の行が読めない（画素）
	const float kMaxWindowWidth = 420.f;
	const float kSliderWidthRatio = 0.5f; // 時間の倍率のスライダーの幅（欄の幅に対して。ラベルとボタンを同じ行に残す）
	const float kMinTimeScale = 0.1f;     // 10 倍のスローまで
	const float kMaxTimeScale = 2.f;
	const float kNormalTimeScale = 1.f;
	const char* const kTimeScaleFormat = "%.2f";
	// G-Buffer の表示の Hi-Z で見る段のスライダー。上限は 4K の半分（1920）でも 1 画素まで届く段の数（それより上は一番粗い段に丸まる）。
	const int kMaxHiZViewLevel = 11;
	const float kHiZLevelSliderWidth = 80.f; // チェックボックスと同じ行に並べる幅（画素）
	// 無敵の行。ON は見落とさないよう赤くし、OFF は他の説明の行と同じ灰色にする。
	const ImVec4 kInvincibleOnColor( 1.f, 0.3f, 0.3f, 1.f );
	const int kShownGamePad = 0; // Pad の欄に出すパッド（自キャラを動かすのは 0 番だけ）

	bool gPlayerInvincible = false; // GameScene から受け取った、自キャラが無敵か
	const GameObjectWorld* gObjects = nullptr; // GameScene から受け取った、世界に置いた物の持ち主（無ければ nullptr）
	Inventory* gInventory = nullptr; // GameScene から受け取った、自キャラのカバン（無ければ nullptr）
	RaidSession* gRaid = nullptr;    // GameScene から受け取った、町の 1 日（無ければ nullptr）
	RunState* gRun = nullptr;        // GameScene・BaseScene から受け取った、5 日間の様子（無ければ nullptr）
	// Raid の欄の「残り 10 秒」（時間切れの流れをすぐ試すため）。
	const float kShortRemainingSeconds = 10.f;

	// F6 で出し入れする物（ユーザー指定の 4 種類。銃とその弾を並べて置く）。ガソリンは車へ運ぶ遊びの物なので、ここでは触らない。
	const ItemId kDebugItems[] = { ITEM_AMMO_762, ITEM_RIFLE, ITEM_PISTOL, ITEM_AMMO_9MM };
	// 一度に足す・減らす数。上限は弾を 1 回で十分に持てる数（カバンの空きのほうが先に尽きる）。
	const int kMinItemAmount = 1;
	const int kMaxItemAmount = 999;
	const int kWeaponLimit = 1;   // 銃は 1 種類 1 丁（ItemCatalog.h の ITEM_PISTOL の説明）
	const ImVec4 kBagFullColor( 1.f, 0.85f, 0.2f, 1.f ); // 入りきらなかった知らせ（見落とさないよう黄色）

	int gItemAmount = kMinItemAmount; // Amount の欄の値
	// 最後に押した Add が入りきらなかったときの結果（入った数・入れたかった数）。全部入ったら wanted を 0 にして消す。
	struct AddResult{
		ItemId item;
		int added;
		int wanted;
	};
	AddResult gLastAdd = { ITEM_NONE, 0, 0 };

	// id を amount 個まで、入る分だけ足す。入った数を返す。
	int addItems( ItemId id, int amount ) {
		int added = 0;
		while ( added < amount && gInventory->canAdd( id ) ) {
			gInventory->add( id );
			++added;
		}
		return added;
	}

	// id を amount 個まで、持っている分だけ減らす（手に持っている銃を消すと、HoldComponent が素手にする）。
	void removeItems( ItemId id, int amount ) {
		for ( int n = gInventory->count( id ); n > 0 && amount > 0; --n, --amount ) {
			gInventory->remove( id );
		}
	}

	// カバンを全部空にする（ガソリンも含む）。中身を持つマスごとに取り出す。
	void clearInventory() {
		const ItemGrid& grid = gInventory->grid();
		for ( int cell = 0; cell < grid.cellCount(); ++cell ) {
			if ( grid.ownerCell( cell ) == cell ) {
				gInventory->takeAll( cell );
			}
		}
	}

	// 1 種類の行（Add・Remove・持っている数）。銃は 1 丁持っていれば Add を押せなくする。
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

	// カバンの中身を好きに出し入れする欄（GridInventory_Plan.txt。拾いに行かずに銃と弾を試すため）。
	// 上に行の数・Amount・Clear、その下に 1 種類 1 行。Add / Remove は Amount の数だけ（入る分・持っている分まで）。
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

	// 町の 1 日と 5 日間の様子を動かす欄（R2_Plan.txt 10 章の ⑥〜⑧ を、遊んで待たずに試すため）。
	void drawRaid() {
		if ( gRaid ) {
			ImGui::Text( "Time left: %.0f s", gRaid->clock().remainingSeconds() );
			ImGui::SameLine();
			if ( ImGui::SmallButton( "10 s left" ) ) {
				gRaid->setRemainingSeconds( kShortRemainingSeconds );
			}
			ImGui::SameLine();
			if ( ImGui::SmallButton( "Extract now" ) ) {
				gRaid->forceExtract();
			}
			ImGui::Text( "Kills today: %d", gRaid->kills() );
		}
		if ( !gRun ) {
			if ( !gRaid ) {
				ImGui::TextDisabled( "No raid." );
			}
			return;
		}
		ImGui::Text( "Day %d / %d", gRun->day(), RunState::kDayCount );
		ImGui::SameLine();
		if ( ImGui::SmallButton( "Day -" ) && gRun->day() > 1 ) {
			gRun->setDay( gRun->day() - 1 );
		}
		ImGui::SameLine();
		if ( ImGui::SmallButton( "Day +" ) && gRun->day() < RunState::kDayCount ) {
			gRun->setDay( gRun->day() + 1 );
		}
		ImGui::Text( "Kills before today: %d   Best: %d", gRun->kills(), gRun->bestScore() );
		ImGui::Text( "Stash items: %d points", RunRules::itemPoints( gRun->stash() ) );
		// 倉庫へ 1 マスぶん（重ねられる物はいっぱいまで。銃は弾倉いっぱいで）足す。
		for ( ItemId id : kDebugItems ) {
			const ItemSpec& spec = itemSpec( id );
			ImGui::PushID( id );
			if ( ImGui::SmallButton( "Add to stash" ) ) {
				ItemStack stack = { id, spec.stackSize };
				WeaponId weapon = weaponForItem( id );
				if ( weapon != WEAPON_NONE ) {
					stack.loadedRounds = weaponSpec( weapon ).magazineCapacity;
				}
				gRun->stash().placeAnywhere( stack );
			}
			ImGui::SameLine();
			ImGui::Text( "%s: %d", spec.saveName, gRun->stash().total( id ) );
			ImGui::PopID();
		}
		if ( ImGui::SmallButton( "Save" ) ) {
			gRun->save();
		}
		ImGui::SameLine();
		// 保存を消して 1 日目から（ハイスコアも消す）。町の中で押すと、今の日はそのまま続き、終わった時の結果が新しい回に入る。
		if ( ImGui::SmallButton( "Erase save" ) ) {
			RunState::erase();
			gRun->setBestScore( 0 );
			RunRules::startNewRun( gRun );
		}
	}

	const char* tagName( GameObjectTag tag ) {
		switch ( tag ) {
			case GameObjectTag::Player: return "Player";
			case GameObjectTag::Weapon: return "Weapon";
			case GameObjectTag::Zombie: return "Zombie";
			case GameObjectTag::Fuel: return "Fuel";
			case GameObjectTag::AmmoDrop: return "AmmoDrop";
			case GameObjectTag::ParkedCar: return "ParkedCar";
			case GameObjectTag::WeaponDrop: return "WeaponDrop";
		}
		return "?";
	}

	// 世界に置いた物を作った順に 1 行ずつ（名前・タグ・付いている部品の数）。ゾンビが 30 体いても読めるよう、表にして縦に流す。
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

	// パッドの 0 番の今の入力（どこから読んでいるか・押しているボタン・スティック）。
	// DualSense などを対応表（GameLib の GamePadMapping）で Xbox の並びに置き換えているので、押したボタンが正しい名前で出るかを見る。
	void drawGamePad() {
		GameLib::Input::GamePad pad = GameLib::Input::Manager::instance().gamePad( kShownGamePad );
		ImGui::Text( "Source: %s", pad.sourceName() );
		std::string pressed;
		for ( int i = 0; i < GameLib::Input::GamePad::BUTTON_MAX; ++i ) {
			if ( !pad.isOn( static_cast< GameLib::Input::GamePad::Button >( i ) ) ) {
				continue;
			}
			// ボタンの名前は GameLib が持つ（割り当ての表示と同じ名前）。
			GameLib::Input::BindingRecord record{ GameLib::Input::CollectorId::InGame, GameLib::Input::WorkId::Up, GameLib::Input::DeviceKind::GamePadButton, i };
			pressed += GameLib::Input::BindingDisplayName( record ) + " ";
		}
		ImGui::Text( "Pressed: %s", pressed.c_str() );
		ImGui::Text( "Left  %+.2f %+.2f", pad.stickX( GameLib::Input::GamePad::STICK_LEFT ), pad.stickY( GameLib::Input::GamePad::STICK_LEFT ) );
		ImGui::Text( "Right %+.2f %+.2f", pad.stickX( GameLib::Input::GamePad::STICK_RIGHT ), pad.stickY( GameLib::Input::GamePad::STICK_RIGHT ) );
	}

	void drawInvincible() {
		if ( gPlayerInvincible ) {
			ImGui::TextColored( kInvincibleOnColor, "Invincible (F5): ON" );
		} else {
			ImGui::TextDisabled( "Invincible (F5): OFF" );
		}
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

	void drawRender() {
		GameLib::Graphics::Manager graphics = GameLib::Graphics::Manager::instance();
		bool vsync = graphics.isVSync();
		if ( ImGui::Checkbox( "VSync", &vsync ) ) {
			graphics.setVSync( vsync );
		}
		ImGui::TextDisabled( "Turn VSync off to see CPU / GPU bound." );
		// 町全体の光を昼・夜の値へ入れ替える（PbrNightLighting_Plan.txt 11 章。見比べ・不具合探し用。遊びは夜）。
		// 細かい値は Tuning の LightingParams で動かす。
		if ( ImGui::Button( "Night" ) ) {
			lightingParams() = LightingParams();
		}
		ImGui::SameLine();
		if ( ImGui::Button( "Day" ) ) {
			lightingParams() = dayLightingParams();
		}
		ImGui::SameLine();
		ImGui::TextDisabled( "(lighting preset)" );
		// 影を描くか（Shadow_Plan.txt 9・11 章）。同じ場面で切り替えて、絵と GPU の時間（gpu.Shadow）を見比べる。
		ImGui::Checkbox( "Shadows", &shadowsEnabled() );
		ImGui::SameLine();
		ImGui::TextDisabled( "(flashlight + moon cascades)" );
		// 建物を形ごとにまとめて描くか（OptimizationA_Plan.txt 8-3）。切ると 1 軒ずつ描く。回数は Draw.gbuffer・Draw.shadow、時間は gpu.Buildings。
		ImGui::Checkbox( "Instancing", &instancingEnabled() );
		ImGui::SameLine();
		ImGui::TextDisabled( "(buildings)" );
		// 簡易 PVS で、建物の向こうに隠れた建物・ゾンビを描かないか（OptimizationC_Plan.txt 7 章）。回数は Pvs.building・Pvs.zombie。
		ImGui::Checkbox( "PVS", &pvsEnabled() );
		ImGui::SameLine();
		ImGui::TextDisabled( "(hidden buildings + zombies)" );
		// 置物を遠くで粗い形（LOD）で描くか（OptimizationC_Plan.txt 8 章）。境目は Tuning の Lod。回数は Lod.1・Lod.2・Draw.triangles。
		ImGui::Checkbox( "LOD", &lodEnabled() );
		ImGui::SameLine();
		ImGui::TextDisabled( "(car, fuel cans)" );
		// 霧を掛けるか（NightFinish_Plan.txt 9・11 章）。同じ場面で切り替えて、絵と GPU の時間（gpu.Fog）を見比べる。
		ImGui::Checkbox( "Fog", &fogEnabled() );
		ImGui::SameLine();
		ImGui::TextDisabled( "(distance + height)" );
		// 空の月の絵（MoonStarsLut_Plan.txt 8・10 章）。値は Tuning の LightingParams（moonRadius〜moonHaloFalloff）。
		// 切ると今までのグラデーションだけの空（水たまりの月も消える）。時間は gpu.Lighting。
		ImGui::Checkbox( "Sky detail", &skyDetailEnabled() );
		ImGui::SameLine();
		ImGui::TextDisabled( "(moon)" );
		// SSAO を作るか（SsaoTranslucent_Plan.txt 8・10 章）。絵は G-Buffer の表示の SSAO で見る。時間は gpu.SSAO。
		ImGui::Checkbox( "SSAO", &ssaoEnabled() );
		ImGui::SameLine();
		// ぼかしを切ると、点の並びを回したざらつきがそのまま見える（調整用）。
		ImGui::Checkbox( "SSAO blur", &ssaoBlurEnabled() );
		ImGui::SameLine();
		ImGui::TextDisabled( "(view: G-Buffer > SSAO)" );
		// 映り込み（SsrDecalMaterial_Plan.txt 7・9 章）。値は Tuning の SSR。切ると空と地面の色の映り込みに戻る。時間は gpu.SSR・gpu.SSR composite。
		ImGui::Checkbox( "SSR", &ssrEnabled() );
		ImGui::SameLine();
		ImGui::TextDisabled( "(view: G-Buffer > SSR / SSR color)" );
		// Hi-Z（HiZSsr_Plan.txt 6・8 章）。切ると今までの直線の進み方（段階12 の計測が終わったら直線は消す）。時間は gpu.Hi-Z・gpu.SSR。
		ImGui::Checkbox( "Hi-Z", &hiZEnabled() );
		ImGui::SameLine();
		ImGui::SetNextItemWidth( kHiZLevelSliderWidth );
		ImGui::SliderInt( "Hi-Z view level", &hiZViewLevel(), 0, kMaxHiZViewLevel );
		ImGui::SameLine();
		ImGui::TextDisabled( "(view: G-Buffer > Hi-Z)" );
		// 粗い面のぼけた映り込み（HiZSsr_Plan.txt 7・8 章）。切ると鏡の向きだけ・ぼかしも今までのまま。TAA を切るとざらつくのが正しい。
		ImGui::Checkbox( "Stochastic SSR", &ssrStochasticEnabled() );
		ImGui::SameLine();
		ImGui::TextDisabled( "(roughness 0.4-0.7, Tuning > SSR maxRoughness)" );
		// 雨上がりの水たまり（SsrDecalMaterial_Plan.txt 6・9 章）。値は Tuning の Puddle。G-Buffer の表示 Roughness で黒い形に見える。
		ImGui::Checkbox( "Puddles", &puddlesEnabled() );
		ImGui::SameLine();
		ImGui::TextDisabled( "(road only, view: G-Buffer > Roughness)" );
		// 弾痕のデカール（SsrDecalMaterial_Plan.txt 8・9 章）。切ると弾痕が描かれない（板の弾痕に戻すのではない）。時間は gpu.Decals。
		ImGui::Checkbox( "Decals", &decalsEnabled() );
		ImGui::SameLine();
		ImGui::TextDisabled( "(bullet holes)" );
		// 半透明の板を壁・地面に刺さった所で薄くするか（SsaoTranslucent_Plan.txt 7・8 章）。切ると板の縁の線が見える。
		ImGui::Checkbox( "Soft particles", &softParticlesEnabled() );
		ImGui::SameLine();
		ImGui::TextDisabled( "(dust, blood, muzzle flash)" );
		// カラーグレーディングとディザ（NightFinish_Plan.txt 9 章）。値は Tuning の PostProcessParams。縞は Dither を切ると見える。
		ImGui::Checkbox( "Color grading", &colorGradingEnabled() );
		ImGui::SameLine();
		ImGui::Checkbox( "Dither", &ditherEnabled() );
		// 色の表（LUT。MoonStarsLut_Plan.txt 7・8 章）。Export で今の調整を Resource/lut_export.tga に焼き、外の道具で直して
		// Resource/lut.tga の名前で置き、Reload で読む。強さは Tuning の PostProcessParams の lutStrength と同じ値。
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
		// アンチエイリアス（Taa_Plan.txt Q4・9 章）。Off / FXAA / TAA から 1 つ。FXAA の値は Tuning の PostProcessParams（fxaa〜）。
		// TAA は描く行列を毎フレームずらし、霧の後に履歴と混ぜる（値は Tuning の TAA。時間は gpu.TAA）。
		AntiAliasing& antiAliasing = antiAliasingMode();
		if ( ImGui::BeginCombo( "Anti-aliasing", antiAliasingName( antiAliasing ) ) ) {
			for ( int i = 0; i < static_cast< int >( AntiAliasing::Count ); ++i ) {
				AntiAliasing candidate = static_cast< AntiAliasing >( i );
				if ( ImGui::Selectable( antiAliasingName( candidate ), candidate == antiAliasing ) ) {
					antiAliasing = candidate;
				}
			}
			ImGui::EndCombo();
		}
		// G-Buffer の中身の表示（GBuffer_Plan.txt 8-2）。Final 以外を選ぶと、ライティングの代わりに中身を画面に出す。
		GBufferView& view = gbufferView();
		if ( ImGui::BeginCombo( "G-Buffer", gbufferViewName( view ) ) ) {
			for ( int i = 0; i < static_cast< int >( GBufferView::Count ); ++i ) {
				GBufferView candidate = static_cast< GBufferView >( i );
				if ( ImGui::Selectable( gbufferViewName( candidate ), candidate == view ) ) {
					view = candidate;
				}
			}
			ImGui::EndCombo();
		}
	}

	// 手順 5（調整値の構造体）で中身を入れる見出し。開けないよう灰色にして、予定だけ見せる。
	void drawPlannedHeader( const char* label ) {
		ImGui::BeginDisabled();
		ImGui::CollapsingHeader( label, ImGuiTreeNodeFlags_Leaf );
		ImGui::EndDisabled();
	}
}

namespace EditorWindow{

void draw() {
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	float width = viewport->WorkSize.x * kWindowWidthRatio;
	width = ( width < kMinWindowWidth ) ? kMinWindowWidth : ( ( width > kMaxWindowWidth ) ? kMaxWindowWidth : width );
	// 右端に上から下まで固定する（窓を動かしたり大きさを変えたりしない。imgui.ini も書かない）。
	ImGui::SetNextWindowPos( ImVec2( viewport->WorkPos.x + viewport->WorkSize.x - width, viewport->WorkPos.y ) );
	ImGui::SetNextWindowSize( ImVec2( width, viewport->WorkSize.y ) );
	const ImGuiWindowFlags kFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
	if ( ImGui::Begin( "Editor (F6)", nullptr, kFlags ) ) {
		// 開いたままでも遊べるので、窓を触る方法を一番上に出す（GameLib/DebugUi の入力の受け渡し）。
		ImGui::TextDisabled( "Hold Alt to use the mouse here." );
		// 見出しを閉じていても見えるよう、見出しの外に置く。
		drawInvincible();
		if ( ImGui::CollapsingHeader( "Scene", ImGuiTreeNodeFlags_DefaultOpen ) ) {
			drawScene();
		}
		// Render は短いので Perf より上に置く（Perf の下だとスクロールしないと VSync に届かない。2026-10-24 画面で確認）。
		if ( ImGui::CollapsingHeader( "Render", ImGuiTreeNodeFlags_DefaultOpen ) ) {
			drawRender();
		}
		if ( ImGui::CollapsingHeader( "Perf", ImGuiTreeNodeFlags_DefaultOpen ) ) {
			GameLib::Profiling::drawPerfPanel();
		}
		// 調整値（自キャラ・銃・視点・ゾンビの群れ）。ゾンビの値も入ったので、見出しは「Tuning」にまとめた。
		if ( ImGui::CollapsingHeader( "Tuning" ) ) {
			TuningPanel::draw();
		}
		if ( ImGui::CollapsingHeader( "Objects" ) ) {
			drawObjects();
		}
		if ( ImGui::CollapsingHeader( "Inventory" ) ) {
			drawInventory();
		}
		if ( ImGui::CollapsingHeader( "Raid" ) ) {
			drawRaid();
		}
		if ( ImGui::CollapsingHeader( "Pad" ) ) {
			drawGamePad();
		}
		drawPlannedHeader( "Zombies / Audio (next)" );
	}
	ImGui::End();
}

void setPlayerInvincible( bool invincible ) {
	gPlayerInvincible = invincible;
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

void setRun( RunState* run ) {
	gRun = run;
}

} //namespace EditorWindow

#else // ZF_EDITOR

namespace EditorWindow{

void draw() {
}

void setPlayerInvincible( bool ) {
}

void setObjects( const GameObjectWorld* ) {
}

void setInventory( Inventory* ) {
}

void setRaid( RaidSession* ) {
}

void setRun( RunState* ) {
}

} //namespace EditorWindow

#endif // ZF_EDITOR
