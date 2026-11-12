#include "GameSrc/DebugMode/TuningVisitors.h"

#ifdef ZF_EDITOR

#include "ThirdParty/imgui/imgui.h"

#include <cstdio>
#include <cstring>

namespace{
	const int kVectorCount = 3;
	static_assert( sizeof( GameLib::Math::Vector3 ) == sizeof( float ) * kVectorCount, "Vector3 must be three floats in a row." );

	const float kDragSpeedRatio = 0.001f;				// エディタで 1 画素ドラッグしたときに動く量を、範囲のこの割合にする（範囲の端から端まで 1000 画素）
	const float kItemWidthRatio = 0.55f;				// 値の入力欄の幅（欄の幅に対して。名前と「既定値に戻す」ボタンを同じ行に残す）
	const ImVec4 kChangedColor( 1.f, 0.8f, 0.3f, 1.f );	// 既定値と違う項目の字の色（ImGui の既定の暗い配色で読める黄色）

	// 小数を元の値へ戻る桁（float は有効数字 9 桁）で文字にする
	std::string numberText( float value ) {
		const int kBufferSize = 32;
		char buffer[ kBufferSize ];
		std::snprintf( buffer, kBufferSize, "%.9g", value );
		return buffer;
	}

	// C++ の float の書き方にする。小数点も指数も無ければ「.f」（30 → 30.f）、あれば「f」を付ける
	std::string floatLiteral( float value ) {
		std::string text = numberText( value );
		bool hasPointOrExponent = text.find_first_of( ".eE" ) != std::string::npos;
		return text + ( hasPointOrExponent ? "f" : ".f" );
	}
}

void TuningCollector::operator()( const char* name, float& value, float minValue, float maxValue, const char* unit ) {
	mItems.push_back( TuningItem{ name, &value, 1, minValue, maxValue, unit } );
}

void TuningCollector::operator()( const char* name, GameLib::Math::Vector3& value, float minValue, float maxValue, const char* unit ) {
	mItems.push_back( TuningItem{ name, &value.x, kVectorCount, minValue, maxValue, unit } );
}

const std::vector< TuningItem >& TuningCollector::items() const {
	return mItems;
}

namespace TuningVisitors{

bool differs( const TuningItem& live, const TuningItem& defaults ) {
	for ( int i = 0; i < live.count; ++i ) {
		if ( live.values[ i ] != defaults.values[ i ] ) {
			return true;
		}
	}
	return false;
}

void copyValues( const TuningItem& from, const TuningItem& to ) {
	for ( int i = 0; i < from.count; ++i ) {
		to.values[ i ] = from.values[ i ];
	}
}

bool draw( const TuningItem& live, const TuningItem& defaults ) {
	ImGui::PushID( live.name );
	bool changed = differs( live, defaults );
	if ( changed ) {
		ImGui::PushStyleColor( ImGuiCol_Text, kChangedColor );
	}
	// 数字の後ろに単位を出す（"%.4g deg"）。Ctrl＋クリックで数字を直接打てる（ImGui の DragFloat の決まり）
	std::string format = std::string( "%.4g " ) + live.unit;
	float speed = ( live.maxValue - live.minValue ) * kDragSpeedRatio;
	ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x * kItemWidthRatio );
	bool edited = ( live.count == kVectorCount )
		? ImGui::DragFloat3( live.name, live.values, speed, live.minValue, live.maxValue, format.c_str(), ImGuiSliderFlags_AlwaysClamp )
		: ImGui::DragFloat( live.name, live.values, speed, live.minValue, live.maxValue, format.c_str(), ImGuiSliderFlags_AlwaysClamp );
	if ( changed ) {
		ImGui::PopStyleColor();
		ImGui::SameLine();
		if ( ImGui::SmallButton( "Reset" ) ) {
			copyValues( defaults, live );
			edited = true;
		}
	}
	ImGui::PopID();
	return edited;
}

std::string fileText( const TuningItem& item ) {
	std::string text;
	for ( int i = 0; i < item.count; ++i ) {
		if ( i > 0 ) {
			text += ", ";
		}
		text += numberText( item.values[ i ] );
	}
	return text;
}

std::string cppLine( const TuningItem& item ) {
	std::string line;
	if ( item.count == kVectorCount ) {
		line = std::string( "GameLib::Math::Vector3 " ) + item.name + " = GameLib::Math::Vector3( " +
			floatLiteral( item.values[ 0 ] ) + ", " + floatLiteral( item.values[ 1 ] ) + ", " + floatLiteral( item.values[ 2 ] ) + " );";
	} else {
		line = std::string( "float " ) + item.name + " = " + floatLiteral( item.values[ 0 ] ) + ";";
	}
	return line + "  // " + item.unit;
}

} //namespace TuningVisitors

#endif // ZF_EDITOR
