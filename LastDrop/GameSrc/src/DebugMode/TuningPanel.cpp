#include "GameSrc/DebugMode/TuningPanel.h"

#ifdef ZF_EDITOR

#include "GameSrc/DebugMode/TuningCatalog.h"

#include "ThirdParty/imgui/imgui.h"

#include <filesystem>
#include <string>
#include <vector>

namespace{
	// プリセットの置き場所（起動した場所から見た所。.gitignore に入れてある）と拡張子
	const char* kDirectory = "Tuning";
	const char* kExtension = ".xml";
	const int kNameCapacity = 64;
	// ファイル名に使えない文字（Windows）。打たれたら保存しない
	const char* kInvalidNameCharacters = "\\/:*?\"<>|";
	const float kNameWidthRatio = 0.5f; // 名前の欄の幅（欄の幅に対して。Save ボタンを同じ行に残す）
	const ImVec4 kWarningColor( 1.f, 0.85f, 0.2f, 1.f );

	char gName[ kNameCapacity ] = "tuning"; // 保存する名前（既定は tuning）
	std::vector< std::string > gFiles;   // Tuning/ にあるプリセットの名前（拡張子なし）
	int gSelectedFile = -1;
	bool gFilesListed = false;           // 一度でも一覧を作ったか（欄を初めて開いたときに作る）
	std::string gStatus;                 // 最後の操作の結果（灰色の字）
	std::vector< std::string > gWarnings; // 読み込みで見つかった問題（黄色い字）

	std::string presetPath( const std::string& name ) {
		return std::string( kDirectory ) + "/" + name + kExtension;
	}

	void listFiles() {
		gFiles.clear();
		gSelectedFile = -1;
		gFilesListed = true;
		std::error_code code;
		if ( !std::filesystem::is_directory( kDirectory, code ) ) {
			return;
		}
		for ( const auto& entry : std::filesystem::directory_iterator( kDirectory, code ) ) {
			if ( entry.is_regular_file() && entry.path().extension() == kExtension ) {
				gFiles.push_back( entry.path().stem().string() );
			}
		}
		if ( !gFiles.empty() ) {
			gSelectedFile = 0;
		}
	}

	void drawSave() {
		ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x * kNameWidthRatio );
		ImGui::InputText( "##name", gName, kNameCapacity );
		ImGui::SameLine();
		if ( ImGui::Button( "Save" ) ) {
			std::string name = gName;
			if ( name.empty() || name.find_first_of( kInvalidNameCharacters ) != std::string::npos ) {
				gStatus = "Invalid name.";
				return;
			}
			std::string error;
			if ( TuningCatalog::save( presetPath( name ), &error ) ) {
				gStatus = "Saved " + presetPath( name );
				listFiles();
			} else {
				gStatus = error;
			}
		}
	}

	void drawLoad() {
		const char* preview = ( gSelectedFile >= 0 ) ? gFiles[ gSelectedFile ].c_str() : "(no preset)";
		ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x * kNameWidthRatio );
		if ( ImGui::BeginCombo( "##files", preview ) ) {
			for ( int i = 0; i < static_cast< int >( gFiles.size() ); ++i ) {
				if ( ImGui::Selectable( gFiles[ i ].c_str(), i == gSelectedFile ) ) {
					gSelectedFile = i;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		ImGui::BeginDisabled( gSelectedFile < 0 );
		if ( ImGui::Button( "Load" ) ) {
			gWarnings.clear();
			const std::string& name = gFiles[ gSelectedFile ];
			if ( TuningCatalog::load( presetPath( name ), &gWarnings ) ) {
				gStatus = "Loaded " + presetPath( name );
			} else {
				gStatus = "Load failed.";
			}
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if ( ImGui::Button( "Refresh" ) ) {
			listFiles();
		}
	}

	void drawTools() {
		if ( ImGui::Button( "Copy as C++" ) ) {
			int changed = 0;
			std::string text = TuningCatalog::toCpp( &changed );
			if ( changed == 0 ) {
				gStatus = "No changed values.";
			} else {
				ImGui::SetClipboardText( text.c_str() );
				gStatus = "Copied " + std::to_string( changed ) + " changed values.";
			}
		}
		ImGui::SameLine();
		if ( ImGui::Button( "Reset all" ) ) {
			TuningCatalog::resetAll();
			gStatus = "Reset to code defaults.";
		}
	}
}

namespace TuningPanel{

void draw() {
	if ( !gFilesListed ) {
		listFiles();
	}
	ImGui::TextDisabled( "Preset (%s/*%s)", kDirectory, kExtension );
	drawSave();
	drawLoad();
	drawTools();
	if ( !gStatus.empty() ) {
		ImGui::TextDisabled( "%s", gStatus.c_str() );
	}
	for ( const std::string& warning : gWarnings ) {
		ImGui::TextColored( kWarningColor, "%s", warning.c_str() );
	}
	ImGui::Separator();
	TuningCatalog::drawAll();
}

} //namespace TuningPanel

#else // ZF_EDITOR

namespace TuningPanel{

void draw() {
}

} //namespace TuningPanel

#endif // ZF_EDITOR
