#ifndef INCLUDED_EDITOR_COLORS_H
#define INCLUDED_EDITOR_COLORS_H

#ifdef ZF_EDITOR

#include "ThirdParty/imgui/imgui.h"

// F6 のエディタの欄が使う字の色。同じ意味の色を欄ごとに決め直さず、ここの 1 つを使う（Debug・Profile ビルドだけ）
namespace EditorColors{

inline const ImVec4 kWarning( 1.f, 0.85f, 0.2f, 1.f );	// 警告・できなかった知らせ（見落とさないよう黄色）

} //namespace EditorColors

#endif // ZF_EDITOR

#endif
