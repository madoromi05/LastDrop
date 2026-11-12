#include "GameSrc/Render/ShaderConstants.h"

#include "GameLib/Math/Matrix44.h"
#include "GameLib/Math/Matrix34.h"

using namespace GameLib::Math;

namespace{
	const int kDepthRowStart = ShaderConstants::kMatrixFloats - ShaderConstants::kRowFloats;	// 4 行目（m30）の、m00 から数えた番号
}

// Matrix44 / Matrix34 のメンバーは m00 から行ごとに並ぶので、m00 の場所から順に読む
void ShaderConstants::copyMatrix44( const Matrix44& matrix, float* destination ) {
	for ( int i = 0; i < kMatrixFloats; ++i ) {
		destination[ i ] = ( &matrix.m00 )[ i ];
	}
}

void ShaderConstants::copyDepthRow( const Matrix44& projectionView, float* destination ) {
	for ( int i = 0; i < kRowFloats; ++i ) {
		destination[ i ] = ( &projectionView.m00 )[ kDepthRowStart + i ];
	}
}

void ShaderConstants::copyMatrix34( const Matrix34& matrix, float* destination ) {
	for ( int i = 0; i < kMatrix34Floats; ++i ) {
		destination[ i ] = ( &matrix.m00 )[ i ];
	}
}
