#include <d3d11.h>
#undef max
#undef min

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/ConstantBuffer.h"
#include "Graphics/ConstantBufferImpl.h"
#include "Graphics/ManagerImpl.h"

namespace GameLib{
namespace Graphics{

ConstantBuffer ConstantBuffer::create( int byteSize ){
	ConstantBuffer r;
	r.mImpl = NEW Impl( byteSize, gManagerImpl->dxDevice(), gManagerImpl->dxContext() );
	return r;
}

void ConstantBuffer::set( const void* data, int byteSize ){
	ASSERT( mImpl && "Graphics::ConstantBuffer : This is empty object." );
	ASSERT( data && "Graphics::ConstantBuffer : data is null." );
	mImpl->set( data, byteSize );
}

int ConstantBuffer::size() const {
	ASSERT( mImpl && "Graphics::ConstantBuffer : This is empty object." );
	return mImpl->mSize;
}

//NULLポインタからの変換
ConstantBuffer::ConstantBuffer( const Impl* ) : mImpl( 0 ){
}

#define TYPE ConstantBuffer
#include "GameLib/Base/Impl/ReferenceTypeTemplate.h"

} //namespace Graphics
} //namespace GameLib
