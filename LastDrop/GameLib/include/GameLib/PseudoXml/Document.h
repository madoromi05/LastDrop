#ifndef INCLUDED_GAMELIB_PSEUDOXML_DOCUMENT_H
#define INCLUDED_GAMELIB_PSEUDOXML_DOCUMENT_H

#include <string>

namespace GameLib{
template< class T > class Array;
namespace PseudoXml{
using namespace std;

class Element;
class ConstElement;

///ファイルアクセス機能を持つニセxml根元クラス
class Document{
public:
	static Document create();											///書き込み用に空で生成
	static Document create( const char* filename );						///ファイルからコンストラクト(要FileIO初期化)
	static Document create( const char* data, int size );				///ロード済み文字列からコンストラクト
	Element root();														///根Elementゲット
	ConstElement root() const;											///根Elementゲット
	bool isReady();														///ロード済みか否か
	bool isError() const;												///読み込み中エラーが発生したか
	void convertToString( string* out ) const;							///ニセxmlテキストを生成
	void convertToBinary( Array< char >* out ) const;					///ニセxmlバイナリを生成
	void write( const char* filename, bool isBinary = false ) const;	///ニセxmlファイルを生成(要FileIO初期化)
	int childNumber() const;											///子の数ゲット
	Element child( int i );												///子ゲット
	ConstElement child( int i ) const;									///子ゲット

	//以下意識しなくていい
	Document();
	Document( const Document& );
	~Document();
	operator void*() const;
	Document& operator=( const Document& );
	void release();
	bool operator==( const Document& ) const;
	bool operator!=( const Document& ) const;

	class Impl;
private:
	Impl* mImpl;
};

//const版
class ConstDocument{
public:
	ConstElement root() const;											///根Elementゲット
	bool isError() const;												///読み込み中エラーが発生したか
	void convertToString( string* out ) const;							///ニセxmlテキストを生成
	void convertToBinary( Array< char >* out ) const;					///ニセxmlバイナリを生成
	void write( const char* filename, bool isBinary = false ) const;	///ニセxmlファイルを生成(要FileIO初期化)
	int childNumber() const;											///子の数ゲット
	ConstElement child( int i ) const;									///子ゲット

	//以下知らなくていい
	ConstDocument();
	ConstDocument( const Document& );
	~ConstDocument();
	ConstDocument& operator=( const Document& );
	void release();
	operator void*() const;
	bool operator==( const Document& ) const;
	bool operator!=( const Document& ) const;
private:
	Document mBase;
};

} //namespace PseudoXml
} //namespace GameLib

#endif
