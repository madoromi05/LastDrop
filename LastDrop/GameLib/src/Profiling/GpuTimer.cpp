#ifdef ZF_EDITOR

#include <d3d11.h>
#undef max
#undef min

#include "GameLib/GameLib.h"
#include "Profiling/GpuTimer.h"
#include "Graphics/ManagerImpl.h"

namespace GameLib{
namespace Profiling{

namespace{
	const char* const kRestName = "Rest"; // 最後の印から Present まで
	const int kBeginStamp = 0;            // stamps[ 0 ] がフレームの始め

	template< class T > void safeRelease( T*& p ){
		if ( p ){
			p->Release();
			p = nullptr;
		}
	}

	ID3D11DeviceContext* context(){
		return Graphics::gManagerImpl ? Graphics::gManagerImpl->dxContext() : nullptr;
	}
}

GpuTimer::GpuTimer() :
mCreated( false ),
mFailed( false ),
mInFrame( false ),
mCurrent( 0 ){
	for ( Slot& slot : mSlots ){
		slot.disjoint = nullptr;
		for ( ID3D11Query*& stamp : slot.stamps ){
			stamp = nullptr;
		}
		slot.markCount = 0;
		slot.frame = -1;
		slot.issued = false;
	}
}

GpuTimer::~GpuTimer(){
	release();
}

bool GpuTimer::create(){
	Graphics::ManagerImpl* graphics = Graphics::gManagerImpl;
	if ( !graphics || !graphics->dxDevice() ){
		return false;
	}
	ID3D11Device* device = graphics->dxDevice();
	D3D11_QUERY_DESC disjointDesc = { D3D11_QUERY_TIMESTAMP_DISJOINT, 0 };
	D3D11_QUERY_DESC stampDesc = { D3D11_QUERY_TIMESTAMP, 0 };
	for ( Slot& slot : mSlots ){
		if ( FAILED( device->CreateQuery( &disjointDesc, &slot.disjoint ) ) ){
			release();
			return false;
		}
		for ( ID3D11Query*& stamp : slot.stamps ){
			if ( FAILED( device->CreateQuery( &stampDesc, &stamp ) ) ){
				release();
				return false;
			}
		}
	}
	mCreated = true;
	return true;
}

void GpuTimer::release(){
	for ( Slot& slot : mSlots ){
		safeRelease( slot.disjoint );
		for ( ID3D11Query*& stamp : slot.stamps ){
			safeRelease( stamp );
		}
		slot.issued = false;
	}
	mCreated = false;
	mInFrame = false;
}

void GpuTimer::beginFrame( long long frameIndex ){
	if ( mFailed ){
		return;
	}
	if ( !mCreated && !create() ){
		mFailed = true;
		return;
	}
	ID3D11DeviceContext* ctx = context();
	if ( !ctx ){
		return;
	}
	mCurrent = static_cast< int >( frameIndex % kLatency );
	Slot& slot = mSlots[ mCurrent ];
	ctx->Begin( slot.disjoint );
	ctx->End( slot.stamps[ kBeginStamp ] );
	slot.markCount = 0;
	slot.frame = frameIndex;
	slot.issued = false;
	mInFrame = true;
}

void GpuTimer::mark( const char* name ){
	if ( !mInFrame ){
		return;
	}
	Slot& slot = mSlots[ mCurrent ];
	if ( slot.markCount >= kMaxMarks ){
		return;
	}
	context()->End( slot.stamps[ slot.markCount + 1 ] );
	slot.names[ slot.markCount ] = name;
	++slot.markCount;
}

bool GpuTimer::endFrame( Result* result ){
	if ( !mInFrame ){
		return false;
	}
	ID3D11DeviceContext* ctx = context();
	Slot& slot = mSlots[ mCurrent ];
	ctx->End( slot.stamps[ slot.markCount + 1 ] );
	ctx->End( slot.disjoint );
	slot.issued = true;
	mInFrame = false;
	// 一番古いフレーム（次のフレームで使い直す Slot）を読む
	Slot& oldest = mSlots[ ( mCurrent + 1 ) % kLatency ];
	if ( !oldest.issued ){
		return false;
	}
	oldest.issued = false; // 読めても読めなくても、この Slot は次のフレームで使い直す
	return read( oldest, result );
}

bool GpuTimer::read( Slot& slot, Result* result ){
	ID3D11DeviceContext* ctx = context();
	// 待たずに聞く（まだなら捨てる）。DONOTFLUSH は、聞くために GPU へ命令を押し出さないため
	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint;
	if ( ctx->GetData( slot.disjoint, &disjoint, sizeof( disjoint ), D3D11_ASYNC_GETDATA_DONOTFLUSH ) != S_OK ){
		return false;
	}
	if ( disjoint.Disjoint || disjoint.Frequency == 0 ){
		return false; // クロックが変わった（値が信用できない）
	}
	int stampCount = slot.markCount + 2;
	UINT64 stamps[ kMaxMarks + 2 ];
	for ( int i = 0; i < stampCount; ++i ){
		if ( ctx->GetData( slot.stamps[ i ], &stamps[ i ], sizeof( UINT64 ), D3D11_ASYNC_GETDATA_DONOTFLUSH ) != S_OK ){
			return false;
		}
	}
	double frequency = static_cast< double >( disjoint.Frequency );
	result->frame = slot.frame;
	result->totalSeconds = static_cast< double >( stamps[ stampCount - 1 ] - stamps[ kBeginStamp ] ) / frequency;
	result->segmentCount = slot.markCount + 1;
	for ( int i = 0; i < result->segmentCount; ++i ){
		result->names[ i ] = ( i < slot.markCount ) ? slot.names[ i ] : kRestName;
		result->segmentSeconds[ i ] = static_cast< double >( stamps[ i + 1 ] - stamps[ i ] ) / frequency;
	}
	return true;
}

} //namespace Profiling
} //namespace GameLib

#endif // ZF_EDITOR
