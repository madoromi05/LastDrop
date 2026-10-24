#ifdef ZF_EDITOR

#include "Profiling/PerfOverlay.h"
#include "Profiling/EventLog.h"
#include "Profiling/FrameRecorder.h"
#include "Profiling/FrameTimeline.h"
#include "Profiling/LoadTracker.h"
#include "Profiling/PerfStats.h"
#include "ThirdParty/imgui/imgui.h"

namespace GameLib{
namespace Profiling{
namespace PerfOverlay{

namespace{
	const double kMillisecondsPerSecond = 1000.0;
	const float kMargin = 4.f;            // 画面の端からの間（画素）
	const float kBackgroundAlpha = 0.45f; // 明るい場面でも読めるくらいの背景
	const ImVec4 kRecordingColor( 1.f, 0.38f, 0.38f, 1.f ); // 赤。記録しっぱなしで遊び続けないように目立たせる
	const ImVec4 kLoadingColor( 1.f, 0.82f, 0.38f, 1.f );   // 黄
	const ImVec2 kTopRightPivot( 1.f, 0.f );
}

void draw( const PerfSources& sources ){
	if ( !ImGui::GetCurrentContext() ){
		return;
	}
	FrameAverages averages = averageFrames( *sources.timeline, kAveragedFrames );
	const FrameRecord* last = sources.timeline->closed( 0 );
	if ( averages.frames == 0 || !last ){
		return;
	}
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos( ImVec2( viewport->WorkPos.x + viewport->WorkSize.x - kMargin, viewport->WorkPos.y + kMargin ),
		ImGuiCond_Always, kTopRightPivot );
	ImGui::SetNextWindowBgAlpha( kBackgroundAlpha );
	const ImGuiWindowFlags kFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoNav;
	if ( ImGui::Begin( "##PerfOverlay", nullptr, kFlags ) ){
		ImGui::Text( "frame %.2fms max %.2f %.0ffps", averages.frame * kMillisecondsPerSecond,
			averages.longest * kMillisecondsPerSecond, ( averages.frame > 0.0 ) ? 1.0 / averages.frame : 0.0 );
		double gpu = sources.gpu->averageTotal( kAveragedFrames );
		if ( gpu >= 0.0 ){
			ImGui::Text( "cpu %.2f present %.2f gpu %.2f", averages.cpu * kMillisecondsPerSecond,
				averages.present * kMillisecondsPerSecond, gpu * kMillisecondsPerSecond );
		}else{
			ImGui::Text( "cpu %.2f present %.2f gpu --", averages.cpu * kMillisecondsPerSecond,
				averages.present * kMillisecondsPerSecond );
		}
		if ( sources.loads->isLoading() ){
			ImGui::TextColored( kLoadingColor, "ticks %d  LOADING", last->ticks );
		}else{
			ImGui::Text( "ticks %d  Bound: %s", last->ticks, boundName( sources.bound ) );
		}
		if ( sources.recorder->isRecording() ){
			ImGui::TextColored( kRecordingColor, "REC %d", sources.recorder->frameCount() );
		}
		const LoadTracker::Summary& load = sources.loads->lastSummary();
		if ( !load.name.empty() ){
			ImGui::Text( "load %s#%d %.2fs", load.name.c_str(), load.attempt, load.seconds );
		}
		// 直近の印：そのフレームの時間・cpu と、後に続くフレーム（EventLog::kFollowFrames 枚）の一番長い時間。
		const std::deque< EventRecord >& recent = sources.events->recent();
		if ( !recent.empty() ){
			const EventRecord& event = recent.back();
			ImGui::Text( "%s %.2fms cpu %.2f max%df %.2f", event.name,
				event.frameSeconds * kMillisecondsPerSecond, event.cpuSeconds * kMillisecondsPerSecond,
				EventLog::kFollowFrames, event.worstAfterSeconds * kMillisecondsPerSecond );
		}
	}
	ImGui::End();
}

} //namespace PerfOverlay
} //namespace Profiling
} //namespace GameLib

#endif // ZF_EDITOR
