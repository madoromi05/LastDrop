#ifndef INCLUDED_GAMELIB_PROFILING_PERF_OVERLAY_H
#define INCLUDED_GAMELIB_PROFILING_PERF_OVERLAY_H

#ifdef ZF_EDITOR

namespace GameLib{
namespace Profiling{

struct PerfSources;

// 画面の右上に計測の数字を出す（計画書 4 章「見た目」の、エディタを閉じている時の表示）。
// 左上は体の図の HUD なので右上に置く。ImGui の小さな窓（クリックできない・半透明）に、直近のフレームの平均を出す。
// 例：
//   frame 16.67ms max 18.20 60fps
//   cpu 4.21 present 12.40 gpu 6.80
//   ticks 1  Bound: GPU         （ロード中は LOADING）
//   REC 1234                    （CSV に記録している間だけ。赤）
//   load Game#2 1.23s           （ロードが 1 回でも終わった後）
//   Shot 10.20ms cpu 0.61 max10f 12.04 （直近の印。印のフレームと、後に続く 10 フレームの一番長い時間）
// 表示を切り替えるキーは画面に出さない（2026-10-21 ユーザー指定）。
// 2026-10-24 に ImGui を入れたので、drawDebugString から ImGui に変えた（明るい場面でも読めるよう背景を敷く）。
namespace PerfOverlay{

// DebugUi::beginFrame と render の間に呼ぶ。
void draw( const PerfSources& sources );

} //namespace PerfOverlay

} //namespace Profiling
} //namespace GameLib

#endif // ZF_EDITOR

#endif
