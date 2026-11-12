#ifndef INCLUDED_GAMELIB_BASE_UTF8_H
#define INCLUDED_GAMELIB_BASE_UTF8_H

// UTF-8 の文字列から 1 字ずつ取り出す
// プロジェクトは /utf-8 でビルドするので、ソースに書いた "拾う" のような文字列は UTF-8 のバイトで入る
// 文字を描く所（StringRenderer）と幅を測る所（UiRenderer::textWidth）で読み方が食い違わないよう、ここだけに書く
// 参考：RFC 3629 https://www.rfc-editor.org/rfc/rfc3629

namespace GameLib{

namespace Utf8{
	const int kReplacementChar = 0xFFFD;	///読めないバイト（UTF-8 の決まりに合わない並び）のときに返す字（U+FFFD）。豆腐として描かれる

	///text の先頭の 1 字を Unicode のコードポイントで返し、その字のバイト数を *byteCount に入れる
	///text は '\0' で終わること。*text が '\0' なら 0 を返し、*byteCount は 0
	///読めない並びは kReplacementChar を返す。進める量は、途中で切れた字ならそこまで読めたバイト数、先頭が字の始まりでなければ 1
	inline int decode( const char* text, int* byteCount ){
		const unsigned kContinuationMask = 0xC0;	// 先頭のバイトの形で、あと何バイト続くかが決まる（0xxxxxxx / 110xxxxx / 1110xxxx / 11110xxx）
		const unsigned kContinuationMark = 0x80;	// 続きのバイトは 10xxxxxx
		const unsigned kContinuationBits = 0x3F;
		const int kBitsPerContinuation = 6;
		struct Lead{ unsigned mask; unsigned mark; int length; };
		const Lead kLeads[] = {
			{ 0x80, 0x00, 1 },
			{ 0xE0, 0xC0, 2 },
			{ 0xF0, 0xE0, 3 },
			{ 0xF8, 0xF0, 4 },
		};
		const unsigned char* s = reinterpret_cast< const unsigned char* >( text );
		if ( s[ 0 ] == '\0' ){
			*byteCount = 0;
			return 0;
		}
		*byteCount = 1;
		for ( const Lead& lead : kLeads ){
			if ( ( s[ 0 ] & lead.mask ) != lead.mark ){
				continue;
			}
			int code = static_cast< int >( s[ 0 ] & ~lead.mask );
			for ( int i = 1; i < lead.length; ++i ){
				if ( ( s[ i ] & kContinuationMask ) != kContinuationMark ){
					// 途中で切れた字は、読めた所までを 1 つの豆腐にする（Unicode の勧める置き換え方）。'\0' もここで止まるので、文字列の外は読まない
					*byteCount = i;
					return kReplacementChar;
				}
				code = ( code << kBitsPerContinuation ) | static_cast< int >( s[ i ] & kContinuationBits );
			}
			*byteCount = lead.length;
			return code;
		}
		return kReplacementChar;
	}
} //namespace Utf8

} //namespace GameLib

#endif
