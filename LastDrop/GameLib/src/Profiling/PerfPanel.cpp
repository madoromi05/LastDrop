#ifdef ZF_EDITOR

#include "GameLib/Profiling/Profiler.h"
#include "Profiling/PerfPanel.h"
#include "Profiling/EventLog.h"
#include "Profiling/FrameRecorder.h"
#include "Profiling/FrameTimeline.h"
#include "Profiling/LoadTracker.h"
#include "Profiling/PerfStats.h"
#include "ThirdParty/imgui/imgui.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace GameLib{
namespace Profiling{
namespace PerfPanel{

namespace{
	const double kMillisecondsPerSecond = 1000.0;
	const float kGraphHeight = 48.f;          // グラフ 1 本の高さ（画素）
	const float kGraphScaleBudgets = 2.f;     // グラフの上端を予算の何倍にするか（33ms。1 回描き損ねたフレームまで見える）
	const float kBudgetLineThickness = 1.f;
	const ImU32 kBudgetLineColor = IM_COL32( 239, 159, 39, 255 ); // 橙（予算 16.67ms の線）
	const ImVec4 kSlowColor( 1.f, 0.45f, 0.45f, 1.f );            // 予算を大きく超えた印
	const ImVec4 kGpuBoundColor( 1.f, 0.62f, 0.15f, 1.f );
	const ImVec4 kCpuBoundColor( 0.4f, 0.8f, 1.f, 1.f );
	const double kSlowEventBudgetRatio = 1.5; // スパイクの JSON を書く境目（SpikeWriter）と同じ倍率
	const int kShownEvents = 10;              // Events に出す印の数（新しい順）
	const float kScopeIndent = 10.f;          // 区間の入れ子 1 段ぶんの字下げ（画素）

	struct ScopeStat{
		const char* name;
		int depth;
		double sumSeconds; // 1 フレームあたりの合計を、フレームの数だけ足した物
		double maxSeconds; // 1 フレームあたりの合計の、一番大きい物
	};

	// 同じ名前でも別の翻訳単位の文字列リテラルはポインタが違うことがあるので、中身で比べる
	int findScope( const std::vector< ScopeStat >& stats, const char* name ){
		for ( size_t i = 0; i < stats.size(); ++i ){
			if ( strcmp( stats[ i ].name, name ) == 0 ){
				return static_cast< int >( i );
			}
		}
		return -1;
	}

	float toMs( double seconds ){
		return static_cast< float >( seconds * kMillisecondsPerSecond );
	}

	// 予算の線を、直前に描いたグラフの上に引く
	void drawBudgetLine( float scaleMax ){
		ImVec2 minCorner = ImGui::GetItemRectMin();
		ImVec2 maxCorner = ImGui::GetItemRectMax();
		float budget = toMs( kFrameBudgetSeconds );
		float y = maxCorner.y - ( maxCorner.y - minCorner.y ) * ( budget / scaleMax );
		ImGui::GetWindowDrawList()->AddLine( ImVec2( minCorner.x, y ), ImVec2( maxCorner.x, y ), kBudgetLineColor, kBudgetLineThickness );
	}

	void drawSummary( const PerfSources& sources ){
		FrameAverages a = averageFrames( *sources.timeline, kAveragedFrames );
		if ( a.frames == 0 ){
			ImGui::TextDisabled( "No frames yet." );
			return;
		}
		ImGui::Text( "frame %.2f ms (max %.2f)  %.0f fps", toMs( a.frame ), toMs( a.longest ),
			( a.frame > 0.0 ) ? 1.0 / a.frame : 0.0 );
		// 細い窓でも切れないよう 2 行に分ける
		ImGui::Text( "cpu %.2f  present %.2f", toMs( a.cpu ), toMs( a.present ) );
		ImGui::Text( "sleep %.2f  other %.2f", toMs( a.sleep ), toMs( a.other ) );
		double gpu = sources.gpu->averageTotal( kAveragedFrames );
		if ( gpu >= 0.0 ){
			ImGui::Text( "gpu %.2f ms", toMs( gpu ) );
		}else{
			ImGui::TextDisabled( "gpu -- (not read yet)" );
		}
		const char* label = boundName( sources.bound );
		if ( sources.bound == BOUND_GPU ){
			ImGui::TextColored( kGpuBoundColor, "Bound: %s", label );
		}else if ( sources.bound == BOUND_CPU ){
			ImGui::TextColored( kCpuBoundColor, "Bound: %s", label );
		}else{
			ImGui::Text( "Bound: %s", label );
		}
	}

	void drawGraphs( const FrameTimeline& timeline ){
		// 古い方から並べる（右端が今）
		std::vector< float > frames;
		std::vector< float > cpus;
		for ( int ago = FrameTimeline::kHistory - 1; ago >= 0; --ago ){
			const FrameRecord* frame = timeline.closed( ago );
			if ( !frame || frame->suspended ){
				continue;
			}
			FrameTimes t = timesOf( *frame );
			frames.push_back( toMs( t.frame ) );
			cpus.push_back( toMs( t.cpu ) );
		}
		if ( frames.empty() ){
			return;
		}
		float scaleMax = toMs( kFrameBudgetSeconds ) * kGraphScaleBudgets;
		ImVec2 size( -1.f, kGraphHeight ); // 幅は欄いっぱい
		ImGui::PlotLines( "##frame", frames.data(), static_cast< int >( frames.size() ), 0, "frame", 0.f, scaleMax, size );
		drawBudgetLine( scaleMax );
		ImGui::PlotLines( "##cpu", cpus.data(), static_cast< int >( cpus.size() ), 0, "cpu", 0.f, scaleMax, size );
		drawBudgetLine( scaleMax );
	}

	void drawRecording( const PerfSources& sources ){
		if ( sources.recorder->isRecording() ){
			ImGui::PushStyleColor( ImGuiCol_Text, kSlowColor );
			bool clicked = ImGui::Button( "Stop CSV" );
			ImGui::PopStyleColor();
			ImGui::SameLine();
			ImGui::Text( "REC %d frames", sources.recorder->frameCount() );
			if ( clicked ){
				toggleRecording();
			}
		}else if ( ImGui::Button( "Record CSV" ) ){
			toggleRecording();
		}
		ImGui::SameLine();
		ImGui::TextDisabled( "spikes %d", sources.spikeFiles );
	}

	void drawScopes( const FrameTimeline& timeline ){
		std::vector< ScopeStat > stats;
		std::vector< double > frameTotals;
		int frameCount = 0;
		for ( int ago = 0; ago < FrameTimeline::kHistory - 1; ++ago ){
			const FrameRecord* frame = timeline.closed( ago );
			if ( !frame ){
				break;
			}
			if ( frame->suspended ){
				continue;
			}
			++frameCount;
			std::fill( frameTotals.begin(), frameTotals.end(), 0.0 );
			for ( int i = 0; i < frame->scopeCount; ++i ){
				const ScopeEvent& scope = frame->scopes[ i ];
				int index = findScope( stats, scope.name );
				if ( index < 0 ){
					stats.push_back( { scope.name, scope.depth, 0.0, 0.0 } );
					frameTotals.push_back( 0.0 );
					index = static_cast< int >( stats.size() ) - 1;
				}
				frameTotals[ index ] += secondsOf( scope );
			}
			for ( size_t i = 0; i < stats.size(); ++i ){
				stats[ i ].sumSeconds += frameTotals[ i ];
				stats[ i ].maxSeconds = std::max( stats[ i ].maxSeconds, frameTotals[ i ] );
			}
		}
		if ( frameCount == 0 ){
			return;
		}
		ImGui::TextDisabled( "ms per frame, last %d frames", frameCount );
		const ImGuiTableFlags kFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp;
		if ( ImGui::BeginTable( "scopes", 3, kFlags ) ){
			ImGui::TableSetupColumn( "scope" );
			ImGui::TableSetupColumn( "avg" );
			ImGui::TableSetupColumn( "max" );
			ImGui::TableHeadersRow();
			for ( const ScopeStat& stat : stats ){
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				// Indent( 0 ) は既定の幅で字下げしてしまうので、入れ子の時だけ呼ぶ
				float indent = kScopeIndent * stat.depth;
				if ( stat.depth > 0 ){
					ImGui::Indent( indent );
				}
				ImGui::TextUnformatted( stat.name );
				if ( stat.depth > 0 ){
					ImGui::Unindent( indent );
				}
				ImGui::TableNextColumn();
				ImGui::Text( "%.3f", toMs( stat.sumSeconds / frameCount ) );
				ImGui::TableNextColumn();
				ImGui::Text( "%.3f", toMs( stat.maxSeconds ) );
			}
			ImGui::EndTable();
		}
	}

	// 回数（addCount）ごとの 1 フレームあたりの平均と最大（直近 240 フレーム）。そのフレームに無い回数は 0 と数える
	void drawCounters( const FrameTimeline& timeline ){
		struct CounterStat{
			const char* name;
			double sum;
			double max;
		};
		std::vector< CounterStat > stats;
		int frameCount = 0;
		for ( int ago = 0; ago < FrameTimeline::kHistory - 1; ++ago ){
			const FrameRecord* frame = timeline.closed( ago );
			if ( !frame ){
				break;
			}
			if ( frame->suspended ){
				continue;
			}
			++frameCount;
			for ( int i = 0; i < frame->counterCount; ++i ){
				auto found = std::find_if( stats.begin(), stats.end(),
					[ & ]( const CounterStat& s ){ return strcmp( s.name, frame->counterNames[ i ] ) == 0; } );
				double value = frame->counterValues[ i ];
				if ( found == stats.end() ){
					stats.push_back( { frame->counterNames[ i ], value, value } );
				}else{
					found->sum += value;
					found->max = std::max( found->max, value );
				}
			}
		}
		if ( stats.empty() ){
			ImGui::TextDisabled( "No counters yet." );
			return;
		}
		ImGui::TextDisabled( "per frame, last %d frames", frameCount );
		const ImGuiTableFlags kFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp;
		if ( ImGui::BeginTable( "counters", 3, kFlags ) ){
			ImGui::TableSetupColumn( "counter" );
			ImGui::TableSetupColumn( "avg" );
			ImGui::TableSetupColumn( "max" );
			ImGui::TableHeadersRow();
			for ( const CounterStat& stat : stats ){
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted( stat.name );
				ImGui::TableNextColumn();
				ImGui::Text( "%.2f", stat.sum / frameCount );
				ImGui::TableNextColumn();
				ImGui::Text( "%.2f", stat.max );
			}
			ImGui::EndTable();
		}
	}

	void drawGpu( const GpuHistory& gpu ){
		struct Segment{
			const char* name;
			double sumSeconds;
			int count;
		};
		std::vector< Segment > segments;
		for ( int ago = 0; ago < kAveragedFrames; ++ago ){
			const GpuTimer::Result* result = gpu.latest( ago );
			if ( !result ){
				break;
			}
			for ( int i = 0; i < result->segmentCount; ++i ){
				auto found = std::find_if( segments.begin(), segments.end(),
					[ & ]( const Segment& s ){ return strcmp( s.name, result->names[ i ] ) == 0; } );
				if ( found == segments.end() ){
					segments.push_back( { result->names[ i ], result->segmentSeconds[ i ], 1 } );
				}else{
					found->sumSeconds += result->segmentSeconds[ i ];
					++found->count;
				}
			}
		}
		if ( segments.empty() ){
			ImGui::TextDisabled( "No GPU timings yet." );
			return;
		}
		ImGui::TextDisabled( "ms, average of last %d frames", kAveragedFrames );
		for ( const Segment& segment : segments ){
			ImGui::Text( "%-12s %.3f", segment.name, toMs( segment.sumSeconds / segment.count ) );
		}
	}

	void drawEvents( const EventLog& events ){
		const std::deque< EventRecord >& recent = events.recent();
		if ( recent.empty() ){
			ImGui::TextDisabled( "No events yet." );
			return;
		}
		double slowSeconds = kFrameBudgetSeconds * kSlowEventBudgetRatio;
		const ImGuiTableFlags kFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp;
		if ( ImGui::BeginTable( "events", 4, kFlags ) ){
			ImGui::TableSetupColumn( "event" );
			ImGui::TableSetupColumn( "frame" );
			ImGui::TableSetupColumn( "cpu" );
			ImGui::TableSetupColumn( "max10f" );
			ImGui::TableHeadersRow();
			int shown = 0;
			for ( auto it = recent.rbegin(); it != recent.rend() && shown < kShownEvents; ++it, ++shown ){
				bool slow = it->worstAfterSeconds > slowSeconds;
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				if ( slow ){
					ImGui::TextColored( kSlowColor, "%s", it->name );
				}else{
					ImGui::TextUnformatted( it->name );
				}
				ImGui::TableNextColumn();
				ImGui::Text( "%.2f", toMs( it->frameSeconds ) );
				ImGui::TableNextColumn();
				ImGui::Text( "%.2f", toMs( it->cpuSeconds ) );
				ImGui::TableNextColumn();
				ImGui::Text( "%.2f", toMs( it->worstAfterSeconds ) );
			}
			ImGui::EndTable();
		}
	}

	void drawLoad( const LoadTracker& loads ){
		const LoadTracker::Summary& load = loads.lastSummary();
		if ( load.name.empty() ){
			ImGui::TextDisabled( "No load finished yet." );
			return;
		}
		ImGui::Text( "%s #%d  %.2f s", load.name.c_str(), load.attempt, load.seconds );
		for ( const LoadItem& item : load.slowest ){
			ImGui::TextWrapped( "%7.1f ms  %s  %s", toMs( item.seconds ), item.category.c_str(), item.name.c_str() );
		}
	}
}

void draw( const PerfSources& sources ){
	drawSummary( sources );
	drawGraphs( *sources.timeline );
	drawRecording( sources );
	if ( ImGui::TreeNodeEx( "Scopes", ImGuiTreeNodeFlags_DefaultOpen ) ){
		drawScopes( *sources.timeline );
		ImGui::TreePop();
	}
	if ( ImGui::TreeNodeEx( "Counters", ImGuiTreeNodeFlags_DefaultOpen ) ){
		drawCounters( *sources.timeline );
		ImGui::TreePop();
	}
	if ( ImGui::TreeNodeEx( "GPU", ImGuiTreeNodeFlags_DefaultOpen ) ){
		drawGpu( *sources.gpu );
		ImGui::TreePop();
	}
	if ( ImGui::TreeNodeEx( "Events", ImGuiTreeNodeFlags_DefaultOpen ) ){
		drawEvents( *sources.events );
		ImGui::TreePop();
	}
	if ( ImGui::TreeNode( "Load" ) ){
		drawLoad( *sources.loads );
		ImGui::TreePop();
	}
}

} //namespace PerfPanel
} //namespace Profiling
} //namespace GameLib

#endif // ZF_EDITOR
