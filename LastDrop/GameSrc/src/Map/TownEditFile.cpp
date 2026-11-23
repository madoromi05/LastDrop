#include "GameSrc/Map/TownEditFile.h"

#include "GameLib/FileIO/Manager.h"
#include "GameLib/FileIO/ReadWhole.h"
#include "GameLib/PseudoXml/Document.h"
#include "GameLib/PseudoXml/Element.h"
#include "GameLib/PseudoXml/Attribute.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifdef ZF_EDITOR
#include <filesystem>
#include <fstream>
#endif

using namespace GameLib;

namespace{
	const char* kRootName = "TownEdit";
	const char* kMoveName = "Move";
	const char* kDeleteName = "Delete";
	const char* kAddName = "Add";
	const char* kVersionName = "version";
	const int kVersion = 1;
#ifdef ZF_EDITOR
	const char* kTemporarySuffix = ".tmp";
#endif

	bool findValue( PseudoXml::ConstElement element, const char* name, const char** out ) {	// 属性 name の値を探す。無ければ false
		for ( int i = 0; i < element.attributeNumber(); ++i ) {
			PseudoXml::ConstAttribute attribute = element.attribute( i );
			if ( std::strcmp( attribute.name(), name ) == 0 ) {
				*out = attribute.value();
				return true;
			}
		}
		return false;
	}

	bool readKind( PseudoXml::ConstElement element, TownEditKind* out, std::string* problem ) {
		const char* text = nullptr;
		if ( !findValue( element, "kind", &text ) ) {
			*problem = "missing kind";
			return false;
		}
		if ( !townEditKindFromName( text, out ) ) {
			*problem = std::string( "unknown kind (" ) + text + ")";
			return false;
		}
		return true;
	}

	bool readNumber( PseudoXml::ConstElement element, const char* name, float* out, std::string* problem ) {	// 数として丸ごと読めるときだけ入れる。getFloatValue は失敗しても 0 を返し、0 と見分けがつかないので使わない
		const char* text = nullptr;
		if ( !findValue( element, name, &text ) ) {
			*problem = std::string( "missing " ) + name;
			return false;
		}
		char* end = nullptr;
		const float value = std::strtof( text, &end );
		while ( end && *end == ' ' ) {
			++end;
		}
		if ( end == text || *end != '\0' || !std::isfinite( value ) ) {
			*problem = std::string( name ) + " is not a number (" + text + ")";
			return false;
		}
		*out = value;
		return true;
	}

	bool readKey( PseudoXml::ConstElement element, TownEditKey* out, std::string* problem ) {
		return readKind( element, &out->kind, problem ) &&
			readNumber( element, "fromX", &out->fromX, problem ) &&
			readNumber( element, "fromZ", &out->fromZ, problem );
	}

	bool readPlacement( PseudoXml::ConstElement element, TownPlacement* out, std::string* problem ) {
		return readNumber( element, "x", &out->x, problem ) &&
			readNumber( element, "y", &out->y, problem ) &&
			readNumber( element, "z", &out->z, problem ) &&
			readNumber( element, "yaw", &out->yawDegree, problem );
	}

	bool readRow( PseudoXml::ConstElement row, TownEdits* edits, std::string* problem ) {	// 1 行を読んで edits へ足す。足せなければ false と、その理由を problem へ入れる
		const char* name = row.name() ? row.name() : "";
		if ( std::strcmp( name, kMoveName ) == 0 ) {
			TownEditKey key;
			TownPlacement to;
			if ( !readKey( row, &key, problem ) || !readPlacement( row, &to, problem ) ) {
				return false;
			}
			edits->move( key, to );
		} else if ( std::strcmp( name, kDeleteName ) == 0 ) {
			TownEditKey key;
			if ( !readKey( row, &key, problem ) ) {
				return false;
			}
			edits->remove( key );
		} else if ( std::strcmp( name, kAddName ) == 0 ) {
			TownEditKind kind;
			TownPlacement at;
			if ( !readKind( row, &kind, problem ) || !readPlacement( row, &at, problem ) ) {
				return false;
			}
			edits->add( kind, at );
		} else {
			*problem = "unknown element";
			return false;
		}
		return true;
	}

#ifdef ZF_EDITOR
	std::string numberText( float value ) {	// 小数を、元の値へ戻る一番短い桁で文字にする（-25.03 は -25.03 のまま。float は有効数字 9 桁で必ず戻る）
		const int kMinDigits = 6;
		const int kMaxDigits = 9;
		const int kBufferSize = 32;
		char buffer[ kBufferSize ];
		for ( int digits = kMinDigits; digits <= kMaxDigits; ++digits ) {
			std::snprintf( buffer, kBufferSize, "%.*g", digits, value );
			if ( std::strtof( buffer, nullptr ) == value ) {
				break;
			}
		}
		return buffer;
	}

	const int kKeyAttributes = 3;
	const int kPlacementAttributes = 4;

	void setKey( PseudoXml::Element row, const TownEditKey& key, int firstAttribute ) {	// row の firstAttribute から kind・fromX・fromZ の 3 つを書く
		row.attribute( firstAttribute ).set( "kind", townEditKindName( key.kind ) );
		row.attribute( firstAttribute + 1 ).set( "fromX", numberText( key.fromX ).c_str() );
		row.attribute( firstAttribute + 2 ).set( "fromZ", numberText( key.fromZ ).c_str() );
	}

	void setPlacement( PseudoXml::Element row, const TownPlacement& p, int firstAttribute ) {	// row の firstAttribute から x・y・z・yaw の 4 つを書く
		row.attribute( firstAttribute ).set( "x", numberText( p.x ).c_str() );
		row.attribute( firstAttribute + 1 ).set( "y", numberText( p.y ).c_str() );
		row.attribute( firstAttribute + 2 ).set( "z", numberText( p.z ).c_str() );
		row.attribute( firstAttribute + 3 ).set( "yaw", numberText( p.yawDegree ).c_str() );
	}

	bool writeText( const std::string& path, const std::string& text, std::string* error ) {	// path へ text をそのまま書く。開けない・書けなければ false と、その理由を error へ入れる
		std::ofstream ofs( path, std::ios::binary );
		if ( !ofs ) {
			*error = "Cannot open " + path;
			return false;
		}
		ofs.write( text.data(), static_cast< std::streamsize >( text.size() ) );
		if ( !ofs ) {
			*error = "Cannot write " + path;
			return false;
		}
		return true;
	}
#endif
}

namespace TownEditFile{

const char* const kPath = "Resource/Map/TownEdit.xml";

bool load( const char* path, TownEdits* out, std::vector< std::string >* warnings ) {
	out->clear();
	if ( !FileIO::Manager().exists( path ) ) {	// FileIO は無いファイルを読もうとすると止まるので、先にあるかを見る。無いのは普通（手直し無し）
		return false;
	}
	std::vector< char > data;
	if ( !FileIO::readWhole( path, &data ) || data.empty() ) {
		warnings->push_back( std::string( "Cannot read " ) + path );
		return false;
	}
	PseudoXml::Document doc = PseudoXml::Document::create( data.data(), static_cast< int >( data.size() ) );
	PseudoXml::ConstElement root = doc.root();	// 書いた要素はルートの子になる（ルートそのものは名前を持たない入れ物）
	bool found = false;
	for ( int r = 0; r < root.childNumber(); ++r ) {
		PseudoXml::ConstElement edit = root.child( r );
		if ( !edit.name() || std::strcmp( edit.name(), kRootName ) != 0 ) {
			continue;
		}
		found = true;
		const char* versionText = nullptr;
		const int version = findValue( edit, kVersionName, &versionText ) ? std::atoi( versionText ) : 0;
		if ( version != kVersion ) {
			warnings->push_back( "Version " + std::to_string( version ) + " (expected " + std::to_string( kVersion ) + ")" );
		}
		for ( int e = 0; e < edit.childNumber(); ++e ) {
			PseudoXml::ConstElement row = edit.child( e );
			std::string problem;
			if ( !readRow( row, out, &problem ) ) {
				warnings->push_back( std::string( "<" ) + ( row.name() ? row.name() : "" ) + "> row " + std::to_string( e + 1 ) + " skipped: " + problem );
			}
		}
	}
	if ( !found ) {	// 文字列から作る PseudoXml は壊れていても失敗を返さないので、壊れたファイルもここで分かる
		warnings->push_back( std::string( "No <" ) + kRootName + "> in " + path );
		return false;
	}
	return true;
}

#ifdef ZF_EDITOR
bool save( const char* path, const TownEdits& edits, std::string* error ) {
	PseudoXml::Document doc = PseudoXml::Document::create();
	PseudoXml::Element root = doc.root();
	root.setChildNumber( 1 );
	PseudoXml::Element edit = root.child( 0 );
	edit.setName( kRootName );
	edit.setAttributeNumber( 1 );
	edit.attribute( 0 ).set( kVersionName, kVersion );

	edit.setChildNumber( static_cast< int >( edits.moves().size() + edits.removals().size() + edits.additions().size() ) );
	int next = 0;
	for ( const TownEdits::Move& move : edits.moves() ) {
		PseudoXml::Element row = edit.child( next++ );
		row.setName( kMoveName );
		row.setAttributeNumber( kKeyAttributes + kPlacementAttributes );
		setKey( row, move.key, 0 );
		setPlacement( row, move.to, kKeyAttributes );
	}
	for ( const TownEditKey& key : edits.removals() ) {
		PseudoXml::Element row = edit.child( next++ );
		row.setName( kDeleteName );
		row.setAttributeNumber( kKeyAttributes );
		setKey( row, key, 0 );
	}
	for ( const TownEdits::Addition& addition : edits.additions() ) {
		PseudoXml::Element row = edit.child( next++ );
		row.setName( kAddName );
		row.setAttributeNumber( 1 + kPlacementAttributes );
		row.attribute( 0 ).set( "kind", townEditKindName( addition.kind ) );
		setPlacement( row, addition.at, 1 );
	}
	std::string text;
	doc.convertToString( &text );

	std::error_code code;
	std::filesystem::path target( path );
	if ( target.has_parent_path() ) {
		std::filesystem::create_directories( target.parent_path(), code );
	}
	const std::string temporary = std::string( path ) + kTemporarySuffix;
	if ( writeText( temporary, text, error ) ) {
		std::filesystem::rename( temporary, target, code );	// MSVC の rename は、置き換え先があっても上書きする
		if ( !code ) {
			return true;
		}
		*error = std::string( "Cannot replace " ) + path + " (" + code.message() + ")";
	}
	std::filesystem::remove( temporary, code );	// 書きかけの一時ファイルを残さない（make_pak が Resource/ の下を全部 .pak へ入れるため）。消せなくても、返すのは書けなかった理由
	return false;
}
#endif

} //namespace TownEditFile
