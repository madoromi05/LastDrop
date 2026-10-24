#include "GameSrc/DebugMode/EditorWindow.h"

#ifdef ZF_EDITOR

#include "GameSrc/Scene/SceneManager.h"
#include "GameSrc/Core/FixedStepClock.h"

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
		drawPlannedHeader( "Player / Camera / Weapon (next)" );
		drawPlannedHeader( "Zombies / Audio (next)" );
	}
	ImGui::End();
}

} //namespace EditorWindow

#else // ZF_EDITOR

namespace EditorWindow{

void draw() {
}

} //namespace EditorWindow

#endif // ZF_EDITOR
