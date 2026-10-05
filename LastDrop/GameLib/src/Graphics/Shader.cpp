#include <d3d11.h>
#undef max
#undef min

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Shader.h"
#include "Graphics/ShaderImpl.h"
#include "Graphics/ManagerImpl.h"

namespace GameLib{
namespace Graphics{

Shader Shader::create( const char* filename, const VertexFormat& format ){
	ASSERT( filename && "Shader : filename is null." );
	Shader r;
	r.mImpl = NEW Impl( filename, format, gManagerImpl->dxDevice() );
	return r;
}

bool Shader::isError() const {
	ASSERT( mImpl && "Graphics::Shader : This is empty object." );
	return !mImpl->isValid();
}

const char* Shader::errorMessage() const {
	ASSERT( mImpl && "Graphics::Shader : This is empty object." );
	return mImpl->mError.c_str();
}

bool Shader::reload(){
	ASSERT( mImpl && "Graphics::Shader : This is empty object." );
	bool ok = mImpl->compile();
	gManagerImpl->onShaderRebuilt( mImpl ); // 使用中なら作り直したシェーダを付け直す
	return ok;
}

const char* Shader::filename() const {
	ASSERT( mImpl && "Graphics::Shader : This is empty object." );
	return mImpl->mFilename.c_str();
}

//NULLポインタからの変換
Shader::Shader( const Impl* ) : mImpl( 0 ){
}

#define TYPE Shader
#include "GameLib/Base/Impl/ReferenceTypeTemplate.h"

} //namespace Graphics
} //namespace GameLib
