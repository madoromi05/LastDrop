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
#include "GameSrc/World/GameObjectWorld.h"

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

	const char* tagName( GameObjectTag tag ) {
		switch ( tag ) {
			case GameObjectTag::Player: return "Player";
			case GameObjectTag::Weapon: return "Weapon";
			case GameObjectTag::Zombie: return "Zombie";
			case GameObjectTag::Fuel: return "Fuel";
			case GameObjectTag::AmmoDrop: return "AmmoDrop";
			case GameObjectTag::EscapeCar: return "EscapeCar";
			case GameObjectTag::FuelStation: return "FuelStation";
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

} //namespace EditorWindow

#else // ZF_EDITOR

namespace EditorWindow{

void draw() {
}

void setPlayerInvincible( bool ) {
}

void setObjects( const GameObjectWorld* ) {
}

} //namespace EditorWindow

#endif // ZF_EDITOR
