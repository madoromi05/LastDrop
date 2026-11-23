#ifndef INCLUDED_GAMELIB_DEBUG_UI_H
#define INCLUDED_GAMELIB_DEBUG_UI_H

#include <cstdint>

// 開発用のエディタ（Dear ImGui）の土台
// Debug と Profile ビルド（ZF_EDITOR）だけで動き、Release ではすべて何もしない関数になる（ImGui 自体もビルドしない）
//
// ここが持つのは ImGui の準備・片付け・1 フレームの始めと描画・F6 での開け閉め・入力の受け渡しだけ
// 窓の中身（Editor の窓）はゲーム側（GameSrc/DebugMode/EditorWindow）が ImGui で描く
//
// 入力の受け渡し（FPS ならではの取り合い）
//   ・閉じている間：ImGui はマウスもカーソルも触らない（カーソルロックはゲームのまま）
//   ・開いている間：ふだんはゲームのまま遊べる（視点・射撃・カーソルロック）
//                   Alt を押している間だけ、マウスをゲームに渡さずエディタで使う（撃たない・視点を回さない・カーソルは自由）
//                   キーボードは ImGui に文字を打っている間だけ止める（歩きながら値を見られる・触れる）
// F6 は ImGui から直接読む。文字を打っていてキーがゲームに届かない間も閉じられるように
namespace GameLib{
namespace DebugUi{

#ifdef ZF_EDITOR

void create( void* windowHandle );	// Graphics::Manager を作った後に呼ぶ（D3D11 のデバイスを使うため）
void destroy();						// Graphics::Manager を消す前に呼ぶ
// 窓のメッセージを ImGui に渡す。WndProc の先頭で呼ぶ。ImGui が使い切ったら true（ゲーム側では扱わない）
// 公開ヘッダを windows.h から切り離すため、型は整数で受ける（WPARAM / LPARAM と同じ大きさ）
bool handleWindowMessage( void* window, unsigned message, std::uintptr_t wParam, std::intptr_t lParam );
void beginFrame();					// 1 フレームの始め。Framework::preUpdate の、入力を読んだ直後に呼ぶ。ここから render までの間に ImGui で描ける
void render();						// ImGui の絵を描く。Framework::postUpdate の、Present の前に呼ぶ（ポストプロセスはかからない）
bool isOpen();						// エディタが開いているか（F6）

#else // ZF_EDITOR

inline void create( void* ){}
inline void destroy(){}
inline bool handleWindowMessage( void*, unsigned, std::uintptr_t, std::intptr_t ){ return false; }
inline void beginFrame(){}
inline void render(){}
inline bool isOpen(){ return false; }

#endif // ZF_EDITOR

} //namespace DebugUi
} //namespace GameLib

#endif
