#include "GameSrc/Render/PostProcess/ColorLut.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Graphics/DepthTarget.h"

#include <fstream>

using namespace GameLib;

namespace{
	const char* kLutFile = "Resource/UI/lut.tga";
	const char* kExportFile = "Resource/UI/lut_export.tga";
	const char* kBakeShaderFile = "Resource/Shader/LutBake.hlsl";
	// 帯の大きさ：32×32 の表を横に 32 枚
	const int kStripWidth = ColorLut::kSize * ColorLut::kSize;
	const int kStripHeight = ColorLut::kSize;
	// LutBake.hlsl は Present.hlsl と同じ番号で定数を読む（b1。PostProcess.cpp の kPresentConstantSlot）
	const int kPresentConstantSlot = Graphics::Manager::FIRST_USER_CONSTANT_SLOT;

	// FileIO は無いファイルを読もうとすると止まる（halt）ので、先にあるかを見る（lut.tga は無いのが普通）
	bool fileExists( const char* path ) {
		std::ifstream ifs( path, std::ios::binary );
		return ifs.good();
	}
}

ColorLut::ColorLut() :
	mLoaded( false ) {
	mBaked = Graphics::RenderTarget::create( kStripWidth, kStripHeight, Graphics::RENDER_TARGET_RGBA8 );
	mBakeShader = Graphics::Shader::create( kBakeShaderFile );
	reload();
}

void ColorLut::reload() {
	mLoaded = false;
	mTexture.release();
	if ( fileExists( kLutFile ) ) {
		// 表の数は画面の色の数のまま引く（_SRGB として読むとリニアに直されてしまう）。TGA のまま（BC7 に縮めると表に誤差が入る）
		mTexture = Graphics::Texture::create( kLutFile, Graphics::TEXTURE_COLOR_SPACE_LINEAR );
	}
}

void ColorLut::update() {
	if ( mLoaded || !mTexture || !mTexture.isReady() ) {
		return;
	}
	// 大きさが違う画像（帯でない・段の数が違う）は引かない。引くと色がでたらめになるため
	if ( mTexture.isError() || mTexture.originalWidth() != kStripWidth || mTexture.originalHeight() != kStripHeight ) {
		mTexture.release();
		return;
	}
	mLoaded = true;
}

bool ColorLut::isLoaded() const {
	return mLoaded;
}

const Graphics::Texture& ColorLut::texture() const {
	return mTexture;
}

void ColorLut::exportBaked( const Graphics::ConstantBuffer& presentConstants ) {
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setRenderTargets( &mBaked, 1, Graphics::DepthTarget() );
	gm.setBlendMode( Graphics::BLEND_OPAQUE );
	gm.setShader( mBakeShader );
	gm.setConstantBuffer( kPresentConstantSlot, presentConstants );
	gm.drawFullScreen();
	// 後から描く物（固定パイプライン）に残さない
	gm.setShader( Graphics::Shader() );
	gm.setConstantBuffer( kPresentConstantSlot, Graphics::ConstantBuffer() );
	gm.resetRenderTargets(); // 読み戻す前に描画先から外す
	mBaked.writeTga( kExportFile );
}

const char* ColorLut::errorMessage() const {
	return mBakeShader.errorMessage();
}
