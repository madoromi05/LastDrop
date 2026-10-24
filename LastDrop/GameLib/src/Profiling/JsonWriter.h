#ifndef INCLUDED_GAMELIB_PROFILING_JSON_WRITER_H
#define INCLUDED_GAMELIB_PROFILING_JSON_WRITER_H

#ifdef ZF_EDITOR

#include <string>
#include <vector>

namespace GameLib{
namespace Profiling{

// 計測の結果を JSON の文字列にする小さな書き手（読むことはしない）。
// ライブラリを足さずに済ませるため自前にしてある。値の関数は、オブジェクトの中では key を、配列の中では nullptr を渡す。
// 例：
//   JsonWriter json;
//   json.beginObject( nullptr );
//   json.number( "totalMs", 12.5 );
//   json.beginArray( "items" ); json.string( nullptr, "a.txt" ); json.endArray();
//   json.endObject();
class JsonWriter{
public:
	JsonWriter();
	void beginObject( const char* key );
	void endObject();
	void beginArray( const char* key );
	void endArray();
	void string( const char* key, const char* text );
	void string( const char* key, const std::string& text );
	void number( const char* key, double value );  // 小数第 3 位まで（ms の 1 マイクロ秒まで）
	void integer( const char* key, long long value );
	void boolean( const char* key, bool value );
	const std::string& text() const;
private:
	// 前の要素とのカンマ・改行・字下げ・キーを書く。
	void beginValue( const char* key );
	void writeQuoted( const char* text );

	std::string mText;
	std::vector< bool > mHasItem; // 開いている入れ子ごとに、もう要素を書いたか
};

} //namespace Profiling
} //namespace GameLib

#endif // ZF_EDITOR

#endif
