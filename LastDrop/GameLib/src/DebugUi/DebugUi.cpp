#ifdef ZF_EDITOR

#include <d3d11.h>
#undef max
#undef min

#include "GameLib/GameLib.h"
#include "GameLib/DebugUi/DebugUi.h"
#include "GameLib/Input/Manager.h"
#include "GameLib/Profiling/Profiler.h"
#include "Graphics/ManagerImpl.h"
#include "ThirdParty/imgui/imgui.h"
#include "ThirdParty/imgui/imgui_impl_win32.h"
#include "ThirdParty/imgui/imgui_impl_dx11.h"

// imgui_impl_win32.h では、windows.h を持ち込まないよう宣言が #if 0 で外してあるので、使う側で宣言する（ImGui の決まり）。
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

namespace GameLib{
namespace DebugUi{

namespace{
	// エディタを開け閉めするキー（2026-10-23 決定。F1～F5・F9 は確認用の機能で使用中。F10 は Windows のメニューに入るキー）。
	const ImGuiKey kToggleKey = ImGuiKey_F6;

	bool gCreated = false;
	bool gOpen = false;

	// 閉じている間は、ImGui にマウスを使わせず、カーソルの形も変えさせない（カーソルロックとの取り合いを起こさない）。
	void applyMouseFlags( ImGuiIO& io ){
		const ImGuiConfigFlags kMouseFlags = ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoMouseCursorChange;
		if ( gOpen ){
			io.ConfigFlags &= ~kMouseFlags;
		}else{
			io.ConfigFlags |= kMouseFlags;
		}
	}
}

void create( void* windowHandle ){
	ASSERT( !gCreated && "DebugUi::create : already created." );
	Graphics::ManagerImpl* graphics = Graphics::gManagerImpl;
	ASSERT( graphics && "DebugUi::create : create Graphics::Manager first." );
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	// 窓の位置は Editor の窓が毎フレーム決めるので、imgui.ini は書かない（ファイルが増えず git にも混ざらない）。
	io.IniFilename = nullptr;
	io.LogFilename = nullptr;
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init( windowHandle );
	ImGui_ImplDX11_Init( graphics->dxDevice(), graphics->dxContext() );
	applyMouseFlags( io );
	gCreated = true;
}

void destroy(){
	if ( !gCreated ){
		return;
	}
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	gCreated = false;
}

bool handleWindowMessage( void* window, unsigned message, std::uintptr_t wParam, std::intptr_t lParam ){
	if ( !gCreated ){
		return false; // 窓を作る途中にも呼ばれる（まだ ImGui が無い）
	}
	return ImGui_ImplWin32_WndProcHandler( static_cast< HWND >( window ), message,
		static_cast< WPARAM >( wParam ), static_cast< LPARAM >( lParam ) ) != 0;
}

void beginFrame(){
	if ( !gCreated ){
		return;
	}
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGuiIO& io = ImGui::GetIO();
	if ( ImGui::IsKeyPressed( kToggleKey, false ) ){
		gOpen = !gOpen;
		applyMouseFlags( io );
	}
	// どちらが入力を使うかは、ImGui の前のフレームの判定で決める（ImGui のふつうの使い方）。
	// 反映されるのは次に入力を読むとき（1 フレーム遅れ）。
	// キーボードは「文字を打っている間」だけ止める（WantTextInput）。WantCaptureKeyboard はボタンを押している間なども
	// 真になり、キーを押している途中で止まったり戻ったりして、1 回の F7 が 2 回に数えられた（2026-10-24 確認）。
	Input::Manager().setUiCapture( gOpen, gOpen && io.WantTextInput );
}

void render(){
	if ( !gCreated ){
		return;
	}
	PROFILE_SCOPE( "ImGui" );
	Profiling::gpuMark( "UI" ); // ここまでが HUD・デバッグ文字の GPU の時間
	ImGui::Render();
	// ImGui はバックバッファへ描く（ステートは ImGui が保存して元に戻す）。
	Graphics::gManagerImpl->bindBackBuffer();
	ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData() );
	Profiling::gpuMark( "ImGui" );
}

bool isOpen(){
	return gOpen;
}

} //namespace DebugUi
} //namespace GameLib

#endif // ZF_EDITOR
