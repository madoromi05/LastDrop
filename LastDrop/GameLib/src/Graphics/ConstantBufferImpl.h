#ifndef INCLUDED_GAMELIB_GRAPHICS_CONSTANTBUFFERIMPL_H
#define INCLUDED_GAMELIB_GRAPHICS_CONSTANTBUFFERIMPL_H

#include "GameLib/Graphics/ConstantBuffer.h"
#include "GameLib/Base/Impl/ReferenceType.h"
#include <cstring>

namespace GameLib{
namespace Graphics{

class ConstantBuffer::Impl : public ReferenceType{
public:
	enum{ ALIGNMENT = 16 }; // 定数バッファの大きさは16バイトの倍数でなければ作れない

	Impl( int byteSize, ID3D11Device* device, ID3D11DeviceContext* context ) :
	mBuffer( 0 ),
	mSize( ( byteSize + ALIGNMENT - 1 ) / ALIGNMENT * ALIGNMENT ),
	mContext( context ){
		ASSERT( byteSize > 0 && "ConstantBuffer : size must be positive." );
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth      = static_cast< UINT >( mSize );
		desc.Usage          = D3D11_USAGE_DYNAMIC;
		desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		HRESULT hr = device->CreateBuffer( &desc, nullptr, &mBuffer );
		STRONG_ASSERT( SUCCEEDED( hr ) && "ConstantBuffer : CreateBuffer failed" );
	}
	~Impl(){
		if ( mBuffer ){ mBuffer->Release(); mBuffer = 0; }
	}
	void set( const void* data, int byteSize ){
		ASSERT( byteSize <= mSize && "ConstantBuffer : data is larger than the buffer." );
		D3D11_MAPPED_SUBRESOURCE ms = {};
		HRESULT hr = mContext->Map( mBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms );
		ASSERT( SUCCEEDED( hr ) && "ConstantBuffer : Map failed" );
		std::memcpy( ms.pData, data, static_cast< size_t >( byteSize ) );
		mContext->Unmap( mBuffer, 0 );
	}

	ID3D11Buffer* mBuffer;
	int mSize;
private:
	ID3D11DeviceContext* mContext;
};

} //namespace Graphics
} //namespace GameLib

#endif
