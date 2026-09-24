#include "SkinnedMeshInstance.h"
#include "SkinnedMeshData.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"
#include "GameLib/Scene/Animation.h"

using namespace GameLib;
using namespace GameLib::Math;

SkinnedMeshInstance::SkinnedMeshInstance( const SkinnedMeshData* data ) :
	mData( data ),
	mPosition( 0.f, 0.f, 0.f ),
	mAngleY( 0.f ) {
	ASSERT( mData && mData->isReady() && "SkinnedMeshInstance : data is null or not ready." );
	mSkeleton = mData->createSkeleton();
	mSkinMatrices.resize( mData->boneNumber() );
	mVertexBuffer = Graphics::VertexBuffer::create( mData->vertexNumber() );
}

void SkinnedMeshInstance::setPosition( const Vector3& position ) {
	mPosition = position;
}

void SkinnedMeshInstance::setAngleY( float degree ) {
	mAngleY = degree;
}

void SkinnedMeshInstance::setAnimation( const char* name, float startTime ) {
	Scene::Animation animation = mData->animation( name );
	if ( !animation ) {
		// 名前違いでもエラーにならず静止したままになるので、ここで知らせる。
		GameLib::cout << "SkinnedMeshInstance: animation not found: " << name << GameLib::endl;
	}
	mSkeleton.setAnimation( animation );
	// 開始時刻を設定する（以降の update は、ここから1フレームずつ進む）。
	mSkeleton.updateAnimation( startTime );
}

void SkinnedMeshInstance::update() {
	// 骨格の根に置き場所と向きを入れると、deform の出力がそのままワールド座標になる。
	mSkeleton.setPosition( mPosition );
	mSkeleton.setAngle( Vector3( 0.f, mAngleY, 0.f ) );
	mSkeleton.updateAnimation();
	mData->deform( mSkeleton, &mSkinMatrices, mVertexBuffer );
}

void SkinnedMeshInstance::draw() const {
	mData->draw( mVertexBuffer );
}
