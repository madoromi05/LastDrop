#ifndef INCLUDED_GAMELIB_GRAPHICS_VERTEXBUFFERIMPL_H
#define INCLUDED_GAMELIB_GRAPHICS_VERTEXBUFFERIMPL_H

#include "GameLib/Graphics/Vertex.h"
#include "GameLib/Base/Impl/ReferenceType.h"
#include "GameLib/PseudoXml/Element.h"
#include "GameLib/PseudoXml/Attribute.h"
#include "GameLib/Base/RefString.h"
#include <vector>

namespace GameLib{
using namespace GameLib::PseudoXml;
namespace Graphics{

class VertexBuffer::Impl : public ReferenceType{
public:
	Impl( ConstElement e, ID3D11Device* device, ID3D11DeviceContext* context ) :
	mDxObject( 0 ),
	mVertexNumber( 0 ),
	mStride( sizeof( Vertex ) ),
	mName( 0 ),
	mDevice( device ),
	mContext( context ){
		mVertexNumber = e.childNumber();
		if ( mVertexNumber > 0 ){
			createDxObject();
			Vertex* vertices = lock();
			for ( int i = 0; i < mVertexNumber; ++i ){
				ConstElement vertex = e.child( i );
				int an = vertex.attributeNumber();
				Vertex& v = vertices[ i ];
				v.mPosition.w = 1.f;
				v.mNormal.set( 0.f, 0.f, 0.f );
				v.mColor = 0xffffffff;
				v.mUv.set( 0.f, 0.f );
				for ( int j = 0; j < an; ++j ){
					ConstAttribute a = vertex.attribute( j );
					if ( RefString( "position" ) == a.name() ){
						a.getFloatValues( &v.mPosition.x, 3 );
					}else if ( RefString( "normal" ) == a.name() ){
						a.getFloatValues( &v.mNormal.x, 3 );
					}else if ( RefString( "uv" ) == a.name() ){
						a.getFloatValues( &v.mUv.x, 2 );
					}else if ( RefString( "color" ) == a.name() ){
						float color[ 4 ];
						a.getFloatValues( color, 4 );
						unsigned c[ 4 ];
						for ( int k = 0; k < 4; ++k ){
							c[ k ] = static_cast< unsigned >( color[ k ] * 255.f + 0.5f );
							c[ k ] = ( c[ k ] > 255 ) ? 255 : c[ k ];
						}
						v.mColor = ( c[ 3 ] << 24 ) | ( c[ 0 ] << 16 ) | ( c[ 1 ] << 8 ) | c[ 2 ];	// ARGB packing -> stored as BGRA bytes (B=byte0,G=byte1,R=byte2,A=byte3)
					}
				}
			}
			unlock();
			vertices = 0;
		}
		int an = e.attributeNumber();
		for ( int i = 0; i < an; ++i ){
			ConstAttribute a = e.attribute( i );
			if ( RefString( "name" ) == a.name() ){
				RefString name( a.value() );
				mName = name.copyOriginal();
			}
		}
	}
	Impl( int vertexNumber, ID3D11Device* device, ID3D11DeviceContext* context ) :
	mDxObject( 0 ),
	mVertexNumber( vertexNumber ),
	mStride( sizeof( Vertex ) ),
	mName( 0 ),
	mDevice( device ),
	mContext( context ){
		createDxObject();
	}
	// 標準の Vertex 以外の形（VertexFormat）の頂点バッファ。stride: 1頂点のバイト数
	Impl( int vertexNumber, int stride, ID3D11Device* device, ID3D11DeviceContext* context ) :
	mDxObject( 0 ),
	mVertexNumber( vertexNumber ),
	mStride( stride ),
	mName( 0 ),
	mDevice( device ),
	mContext( context ){
		createDxObject();
	}
	~Impl(){
		if ( mDxObject ){
			mDxObject->Release();
			mDxObject = 0;
		}
		SAFE_DELETE_ARRAY( mName );
	}
	void createDxObject(){
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth      = static_cast< UINT >( mVertexNumber * mStride );
		desc.Usage          = D3D11_USAGE_DYNAMIC;
		desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		HRESULT hr = mDevice->CreateBuffer( &desc, nullptr, &mDxObject );
		ASSERT( SUCCEEDED( hr ) && "VertexBuffer CreateBuffer failed" );
	}
	// 頂点の形を問わずに書く（中身の並びは呼び出し側が VertexFormat どおりに詰める）
	void* lockData(){
		D3D11_MAPPED_SUBRESOURCE ms = {};
		HRESULT hr = mContext->Map( mDxObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms );
		ASSERT( SUCCEEDED( hr ) && "VertexBuffer Map failed" );
		return ms.pData;
	}
	Vertex* lock(){
		ASSERT( mStride == sizeof( Vertex ) && "VertexBuffer : not a standard Vertex buffer. use lockData()." );
		D3D11_MAPPED_SUBRESOURCE ms = {};
		HRESULT hr = mContext->Map( mDxObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms );
		ASSERT( SUCCEEDED( hr ) && "VertexBuffer Map failed" );
		return static_cast< Vertex* >( ms.pData );
	}
	void unlock(){
		mContext->Unmap( mDxObject, 0 );
	}

	ID3D11Buffer*        mDxObject;
	int                  mVertexNumber;
	int                  mStride; // 1頂点のバイト数（標準の Vertex なら sizeof( Vertex )）
	char*                mName;
	ID3D11Device*        mDevice;
	ID3D11DeviceContext* mContext;
private:
	void operator=( const Impl& );
};

} //namespace Graphics
} //namespace GameLib

#endif
