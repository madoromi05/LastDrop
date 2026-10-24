#include "GameSrc/Core/GameSettings.h"
#include "GameSrc/Audio/AudioEngine.h"

#include "GameLib/PseudoXml/Document.h"
#include "GameLib/PseudoXml/Element.h"
#include "GameLib/PseudoXml/Attribute.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

using namespace GameLib;

namespace{
	// 置き場所はキーコンフィグ（input_p0.xml）と同じく、起動した場所。
	const char* kFilePath = "settings.xml";
	const char* kElementName = "Settings";
	const char* kAttrMasterVolume = "masterVolume";
	const char* kAttrEffectVolume = "effectVolume";
	const char* kAttrMouseSensitivity = "mouseSensitivity";
	const int kAttributeNumber = 3;
	const float kPercent = 100.f;

	int clampValue( int value, int minValue, int maxValue ) {
		return std::min( std::max( value, minValue ), maxValue );
	}

	// 音量の数（0〜100）を掛ける倍率にする。そのままではなく 2 乗する（50 で 0.25 倍）。
	// 耳は音の大きさを比で感じるので、真ん中あたりで動かしたときに変わったのが分かりやすい。
	float volumeGain( int volume ) {
		float ratio = static_cast< float >( volume ) / static_cast< float >( GameSettings::kVolumeMax );
		return ratio * ratio;
	}
}

GameSettings::GameSettings() :
	masterVolume( kVolumeMax ),
	effectVolume( kVolumeMax ),
	mouseSensitivity( static_cast< int >( kPercent ) ) {
}

//   <Settings masterVolume="80" effectVolume="100" mouseSensitivity="125"/>
bool GameSettings::save() const {
	PseudoXml::Document doc = PseudoXml::Document::create();
	PseudoXml::Element root = doc.root();
	root.setChildNumber( 1 );
	PseudoXml::Element e = root.child( 0 );
	e.setName( kElementName );
	e.setAttributeNumber( kAttributeNumber );
	e.attribute( 0 ).set( kAttrMasterVolume, masterVolume );
	e.attribute( 1 ).set( kAttrEffectVolume, effectVolume );
	e.attribute( 2 ).set( kAttrMouseSensitivity, mouseSensitivity );

	std::string text;
	doc.convertToString( &text );
	std::ofstream ofs( kFilePath, std::ios::binary );
	if ( !ofs ) {
		return false;
	}
	ofs.write( text.data(), static_cast< std::streamsize >( text.size() ) );
	return static_cast< bool >( ofs );
}

bool GameSettings::load() {
	std::ifstream ifs( kFilePath, std::ios::binary );
	if ( !ifs ) {
		return false;
	}
	std::ostringstream buffer;
	buffer << ifs.rdbuf();
	std::string text = buffer.str();
	if ( text.empty() ) {
		return false;
	}
	PseudoXml::Document doc = PseudoXml::Document::create( text.data(), static_cast< int >( text.size() ) );
	if ( !doc || doc.isError() ) {
		return false;
	}
	// 書いた要素はルートの子になる（ルートそのものは名前を持たない入れ物）。
	PseudoXml::ConstElement root = doc.root();
	for ( int i = 0; i < root.childNumber(); ++i ) {
		PseudoXml::ConstElement e = root.child( i );
		if ( !e.name() || std::strcmp( e.name(), kElementName ) != 0 ) {
			continue;
		}
		// ファイルに無い項目は既定値のまま（後で項目を足しても、古いファイルを読める）。
		for ( int j = 0; j < e.attributeNumber(); ++j ) {
			PseudoXml::ConstAttribute a = e.attribute( j );
			const char* name = a.name();
			if ( std::strcmp( name, kAttrMasterVolume ) == 0 ) {
				masterVolume = clampValue( a.getIntValue(), kVolumeMin, kVolumeMax );
			} else if ( std::strcmp( name, kAttrEffectVolume ) == 0 ) {
				effectVolume = clampValue( a.getIntValue(), kVolumeMin, kVolumeMax );
			} else if ( std::strcmp( name, kAttrMouseSensitivity ) == 0 ) {
				mouseSensitivity = clampValue( a.getIntValue(), kSensitivityMin, kSensitivityMax );
			}
		}
		return true;
	}
	return false;
}

void GameSettings::applyAudio( AudioEngine& audio ) const {
	audio.setMasterVolume( volumeGain( masterVolume ) );
	audio.setBusVolume( AudioEngine::BUS_EFFECT, volumeGain( effectVolume ) );
}

float GameSettings::lookSensitivityScale() const {
	return static_cast< float >( mouseSensitivity ) / kPercent;
}
