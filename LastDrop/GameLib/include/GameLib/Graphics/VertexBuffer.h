#ifndef INCLUDED_GAMELIB_GRAPHICS_VERTEXBUFFER_H
#define INCLUDED_GAMELIB_GRAPHICS_VERTEXBUFFER_H

namespace GameLib{
namespace PseudoXml{
	class ConstElement;
}
namespace Graphics{

class Vertex;
class VertexFormat;

class VertexBuffer{
public:
	static VertexBuffer create( PseudoXml::ConstElement );
	static VertexBuffer create( int vertexNumber );
	///標準の Vertex 以外の形の頂点バッファ。書き込みは lockData で行う（lock は使えない）
	static VertexBuffer create( int vertexNumber, const VertexFormat& );
	Vertex* lock();
	void unlock( Vertex** );
	///形を問わずに書き込む。中身は VertexFormat の並び（offset / stride）どおりに詰める
	void* lockData();
	void unlockData( void** );
	///1頂点のバイト数
	int stride() const;
	const char* name() const;
	int vertexNumber() const;

	//以下ユーザは意識しない関数群
	VertexBuffer();
	VertexBuffer( const VertexBuffer& );
	~VertexBuffer();
	operator void*() const;
	VertexBuffer& operator=( const VertexBuffer& );
	void release();
	bool operator==( const VertexBuffer& ) const;
	bool operator!=( const VertexBuffer& ) const;

	class Impl;
	VertexBuffer( const Impl* );
private:
	friend class Manager;
	Impl* mImpl;
};

} //namespace Graphics
} //namespace GameLib

#endif
