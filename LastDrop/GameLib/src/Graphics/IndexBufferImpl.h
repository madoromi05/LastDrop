#ifndef INCLUDED_GAMELIB_GRAPHICS_INDEXBUFFERIMPL_H
#define INCLUDED_GAMELIB_GRAPHICS_INDEXBUFFERIMPL_H

#include "GameLib/Base/Impl/ReferenceType.h"
#include "GameLib/PseudoXml/Element.h"
#include "GameLib/PseudoXml/Attribute.h"
#include "GameLib/Base/RefString.h"

namespace GameLib{
using namespace GameLib::PseudoXml;
namespace Graphics{

class IndexBuffer::Impl : public ReferenceType{
public:
	Impl( ConstElement e, ID3D11Device* device, ID3D11DeviceContext* context ) :
	mDxObject( 0 ),
	mIndexNumber( 0 ),
	mName( 0 ),
	mDevice( device ),
	mContext( context ){
		int n = e.childNumber();
		mIndexNumber = n * 3;
		if ( mIndexNumber > 0 ){
			createDxObject();
			unsigned short* indices = lock();
			for ( int i = 0; i < n; ++i ){
				ConstElement triangle = e.child( i );
				ASSERT( triangle.attributeNumber() > 0 );
				ConstAttribute a = triangle.attribute( 0 );
				int tmp[ 3 ];
				int count = a.getIntValues( tmp, 3 );
				ASSERT( count == 3 );
				for ( int j = 0; j < 3; ++j ){
					ASSERT( tmp[ j ] >= 0 && tmp[ j ] <= 0xffff );
					indices[ i * 3 + j ] = static_cast< unsigned short >( tmp[ j ] );
				}
			}
			unlock();
			indices = 0;
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
	Impl( int indexNumber, ID3D11Device* device, ID3D11DeviceContext* context ) :
	mDxObject( 0 ),
	mIndexNumber( indexNumber ),
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
		desc.ByteWidth      = mIndexNumber * sizeof( unsigned short );
		desc.Usage          = D3D11_USAGE_DYNAMIC;
		desc.BindFlags      = D3D11_BIND_INDEX_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		HRESULT hr = mDevice->CreateBuffer( &desc, nullptr, &mDxObject );
		ASSERT( SUCCEEDED( hr ) && "IndexBuffer CreateBuffer failed" );
	}
	unsigned short* lock(){
		D3D11_MAPPED_SUBRESOURCE ms = {};
		HRESULT hr = mContext->Map( mDxObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms );
		ASSERT( SUCCEEDED( hr ) && "IndexBuffer Map failed" );
		return static_cast< unsigned short* >( ms.pData );
	}
	void unlock(){
		mContext->Unmap( mDxObject, 0 );
	}
	const char* name() const {
		return mName;
	}

	ID3D11Buffer*        mDxObject;
	int                  mIndexNumber;
	char*                mName;
	ID3D11Device*        mDevice;
	ID3D11DeviceContext* mContext;
private:
	void operator=( const Impl& );
};

} //namespace Graphics
} //namespace GameLib

#endif
