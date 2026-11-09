#ifndef INCLUDED_GAMELIB_PROFILING_PERF_PANEL_H
#define INCLUDED_GAMELIB_PROFILING_PERF_PANEL_H

#ifdef ZF_EDITOR

namespace GameLib{
namespace Profiling{

struct PerfSources;

// エディタの Perf の欄の中身。EditorWindow が ImGui の窓の中で呼ぶ。
//   ・数字：frame（平均・最大・fps）、cpu / present / sleep / other、gpu、CPU と GPU のどちらが重いか
//   ・グラフ：直近 240 フレームの frame と cpu（橙の線が予算 16.67ms）
//   ・記録：CSV の記録の開始・停止（F7 と同じ）、書いたスパイクの JSON の数
//   ・Scopes：区間（PROFILE_SCOPE）ごとの 1 フレームあたりの平均と最大（直近 240 フレーム）
//   ・Counters：回数（addCount）ごとの 1 フレームあたりの平均と最大（直近 240 フレーム）
//   ・GPU：GPU の区間（gpuMark）ごとの平均
//   ・Events：最近の印（どのボタン・出来事の時に重いか）
//   ・Load：直前のロードの合計と、時間がかかった処理 5 つ
namespace PerfPanel{

void draw( const PerfSources& sources );

} //namespace PerfPanel

} //namespace Profiling
} //namespace GameLib

#endif // ZF_EDITOR

#endif
