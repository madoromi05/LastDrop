#ifndef INCLUDED_TUNING_PANEL_H
#define INCLUDED_TUNING_PANEL_H

// F6 のエディタの「Player / Camera / Weapon」欄の中身（Debug・Profile ビルドだけ）。計画書は DebugTools_Profiling_Plan.txt の 5-5。
//   上：Preset（名前・Save・ファイルの一覧から Load）、Copy as C++、Reset all。読めなかった所は黄色い字で一覧にする
//   下：調整値の構造体ごとの小見出しと項目（TuningCatalog::drawAll）
// 値を変えるとすぐ効く（どの値も使う所が毎フレーム読んでいる）。起動中だけで、自動では保存も読み込みもしない。
namespace TuningPanel{

// エディタの欄が開いているときに、1 回の描画につき 1 回呼ぶ。Release では何もしない。
void draw();

} //namespace TuningPanel

#endif
