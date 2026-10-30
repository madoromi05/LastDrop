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
#include "GameSrc/Render/LitQuadBatch.h"

#include "GameLib/Graphics/Manager.h"
#include "GameLib/Profiling/Profiler.h"
#include "ThirdParty/imgui/imgui.h"

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
	// 無敵の行。ON は見落とさないよう赤くし、OFF は他の説明の行と同じ灰色にする。
	const ImVec4 kInvincibleOnColor( 1.f, 0.3f, 0.3f, 1.f );

	bool gPlayerInvincible = false; // GameScene から受け取った、自キャラが無敵か

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
		// 霧を掛けるか（NightFinish_Plan.txt 9・11 章）。同じ場面で切り替えて、絵と GPU の時間（gpu.Fog）を見比べる。
		ImGui::Checkbox( "Fog", &fogEnabled() );
		ImGui::SameLine();
		ImGui::TextDisabled( "(distance + height)" );
		// SSAO を作るか（SsaoTranslucent_Plan.txt 8・10 章）。絵は G-Buffer の表示の SSAO で見る。時間は gpu.SSAO。
		ImGui::Checkbox( "SSAO", &ssaoEnabled() );
		ImGui::SameLine();
		// ぼかしを切ると、点の並びを回したざらつきがそのまま見える（調整用）。
		ImGui::Checkbox( "SSAO blur", &ssaoBlurEnabled() );
		ImGui::SameLine();
		ImGui::TextDisabled( "(view: G-Buffer > SSAO)" );
		// 半透明の板を壁・地面に刺さった所で薄くするか（SsaoTranslucent_Plan.txt 7・8 章）。切ると板の縁の線が見える。
		ImGui::Checkbox( "Soft particles", &softParticlesEnabled() );
		ImGui::SameLine();
		ImGui::TextDisabled( "(dust, blood, muzzle flash)" );
		// カラーグレーディングとディザ（NightFinish_Plan.txt 9 章）。値は Tuning の PostProcessParams。縞は Dither を切ると見える。
		ImGui::Checkbox( "Color grading", &colorGradingEnabled() );
		ImGui::SameLine();
		ImGui::Checkbox( "Dither", &ditherEnabled() );
		// FXAA（NightFinish_Plan.txt 8・9 章）。値は Tuning の PostProcessParams（fxaa〜）。切ると今までどおり画面へ直接写す。
		ImGui::Checkbox( "FXAA", &fxaaEnabled() );
		ImGui::SameLine();
		ImGui::TextDisabled( "(edges: buildings, car)" );
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
		drawPlannedHeader( "Zombies / Audio (next)" );
	}
	ImGui::End();
}

void setPlayerInvincible( bool invincible ) {
	gPlayerInvincible = invincible;
}

} //namespace EditorWindow

#else // ZF_EDITOR

namespace EditorWindow{

void draw() {
}

void setPlayerInvincible( bool ) {
}

} //namespace EditorWindow

#endif // ZF_EDITOR
