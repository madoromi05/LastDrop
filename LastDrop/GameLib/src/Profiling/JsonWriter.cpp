#ifdef ZF_EDITOR

#include "Profiling/JsonWriter.h"
#include "GameLib/GameLib.h"

#include <cstdio>

namespace GameLib{
namespace Profiling{

namespace{
	const int kIndentWidth = 2;
	const int kNumberBufferSize = 64;
	const unsigned char kFirstPrintable = 0x20; // これより小さい文字（改行・タブなど）は \u で書く
}

JsonWriter::JsonWriter(){
}

void JsonWriter::beginObject( const char* key ){
	beginValue( key );
	mText += '{';
	mHasItem.push_back( false );
}

void JsonWriter::endObject(){
	ASSERT( !mHasItem.empty() && "JsonWriter : endObject without beginObject." );
	bool hadItem = mHasItem.back();
	mHasItem.pop_back();
	if ( hadItem ){
		mText += '\n';
		mText.append( mHasItem.size() * kIndentWidth, ' ' );
	}
	mText += '}';
}

void JsonWriter::beginArray( const char* key ){
	beginValue( key );
	mText += '[';
	mHasItem.push_back( false );
}

void JsonWriter::endArray(){
	ASSERT( !mHasItem.empty() && "JsonWriter : endArray without beginArray." );
	bool hadItem = mHasItem.back();
	mHasItem.pop_back();
	if ( hadItem ){
		mText += '\n';
		mText.append( mHasItem.size() * kIndentWidth, ' ' );
	}
	mText += ']';
}

void JsonWriter::string( const char* key, const char* text ){
	beginValue( key );
	writeQuoted( text ? text : "" );
}

void JsonWriter::string( const char* key, const std::string& text ){
	string( key, text.c_str() );
}

void JsonWriter::number( const char* key, double value ){
	beginValue( key );
	char buffer[ kNumberBufferSize ];
	snprintf( buffer, sizeof( buffer ), "%.3f", value );
	mText += buffer;
}

void JsonWriter::integer( const char* key, long long value ){
	beginValue( key );
	char buffer[ kNumberBufferSize ];
	snprintf( buffer, sizeof( buffer ), "%lld", value );
	mText += buffer;
}

void JsonWriter::boolean( const char* key, bool value ){
	beginValue( key );
	mText += value ? "true" : "false";
}

const std::string& JsonWriter::text() const {
	return mText;
}

void JsonWriter::beginValue( const char* key ){
	if ( mHasItem.empty() ){
		return; // 一番外の値
	}
	if ( mHasItem.back() ){
		mText += ',';
	}
	mHasItem.back() = true;
	mText += '\n';
	mText.append( mHasItem.size() * kIndentWidth, ' ' );
	if ( key ){
		writeQuoted( key );
		mText += ": ";
	}
}

void JsonWriter::writeQuoted( const char* text ){
	mText += '"';
	for ( const char* p = text; *p; ++p ){
		unsigned char c = static_cast< unsigned char >( *p );
		if ( c == '"' || c == '\\' ){
			mText += '\\';
			mText += static_cast< char >( c );
		}else if ( c < kFirstPrintable ){
			char buffer[ kNumberBufferSize ];
			snprintf( buffer, sizeof( buffer ), "\\u%04x", c );
			mText += buffer;
		}else{
			mText += static_cast< char >( c );
		}
	}
	mText += '"';
}

} //namespace Profiling
} //namespace GameLib

#endif // ZF_EDITOR
