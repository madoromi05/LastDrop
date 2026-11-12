#ifdef ZF_EDITOR

#include "Profiling/SpikeWriter.h"
#include "Profiling/FrameTimeline.h"
#include "Profiling/JsonWriter.h"
#include "Profiling/LoadTracker.h"
#include "Profiling/OutputFiles.h"
#include "Profiling/SystemInfo.h"
#include "GameLib/Profiling/Profiler.h"

#include <string>

namespace GameLib{
namespace Profiling{

namespace{
	const double kSpikeBudgetRatio = 1.5;	// 予算の何倍を超えたらスパイクとするか。VSync 中は 1 回描き損ねると 2 倍（33ms）になるので、その間に置く
	const double kCooldownSeconds = 1.0;	// 続けて書かない間隔（秒）。書き出しで次のフレームが遅れ、それがまたスパイクになるのを防ぐ
	// 1 回の起動で書く数の上限（ずっと重い場面でファイルが増え続けないように）
	const int kMaxFilesPerLaunch = 50;
	const double kMillisecondsPerSecond = 1000.0;
	const int kFormatVersion = 1;			// JSON の形を変えたら上げる
}

SpikeWriter::SpikeWriter() :
mLastWrittenFrame( -1 ),
mSecondsSinceWritten( 0.0 ),
mWrittenCount( 0 ){
}

bool SpikeWriter::writeIfSpike( const FrameRecord& frame, const std::deque< LoadItem >& recentLoadItems ){
	FrameTimes times = timesOf( frame );
	mSecondsSinceWritten += times.frame;
	// ロード中は長いのが当たり前（ロードの JSON の方で見る）。止まっていた・自分で書いたフレームは原因がはっきりしている
	if ( frame.loading || frame.suspended || frame.wroteFile ){
		return false;
	}
	if ( times.frame <= kFrameBudgetSeconds * kSpikeBudgetRatio ){
		return false;
	}
	if ( mWrittenCount >= kMaxFilesPerLaunch ){
		return false;
	}
	if ( mLastWrittenFrame >= 0 && mSecondsSinceWritten < kCooldownSeconds ){
		return false;
	}

	JsonWriter json;
	json.beginObject( nullptr );
	json.integer( "version", kFormatVersion );
	json.integer( "frame", frame.index );
	json.number( "frameMs", times.frame * kMillisecondsPerSecond );
	json.number( "budgetMs", kFrameBudgetSeconds * kMillisecondsPerSecond );
	json.number( "cpuMs", times.cpu * kMillisecondsPerSecond );
	json.number( "presentMs", times.present * kMillisecondsPerSecond );
	json.number( "sleepMs", times.sleep * kMillisecondsPerSecond );
	json.number( "otherMs", times.other * kMillisecondsPerSecond );
	json.integer( "ticks", frame.ticks );
	json.integer( "droppedScopes", frame.droppedScopes );
	// このフレームで何をしていたか（押したボタン・弾が出た など）
	json.beginArray( "events" );
	for ( int i = 0; i < frame.eventCount; ++i ){
		json.string( nullptr, frame.events[ i ] );
	}
	json.endArray();
	// 区間は始まった順。startMs はフレームの始めから。depth で入れ子が分かる
	json.beginArray( "scopes" );
	for ( int i = 0; i < frame.scopeCount; ++i ){
		const ScopeEvent& scope = frame.scopes[ i ];
		json.beginObject( nullptr );
		json.string( "name", scope.name );
		json.integer( "depth", scope.depth );
		json.number( "startMs", secondsBetween( frame.begin, scope.begin ) * kMillisecondsPerSecond );
		json.number( "ms", secondsOf( scope ) * kMillisecondsPerSecond );
		json.endObject();
	}
	json.endArray();
	json.beginArray( "loadItems" );
	for ( const LoadItem& item : recentLoadItems ){
		if ( item.frame != frame.index ){
			continue;
		}
		json.beginObject( nullptr );
		json.string( "category", item.category );
		json.string( "name", item.name );
		json.number( "ms", item.seconds * kMillisecondsPerSecond );
		json.endObject();
	}
	json.endArray();
	SystemInfo::write( "system", &json );
	json.endObject();

	// 同じ秒に 2 つ書いても重ならないよう、フレームの番号も名前に入れる
	std::string fileName = "spike_" + OutputFiles::timestamp() + "_f" + std::to_string( frame.index ) + ".json";
	OutputFiles::write( OutputFiles::pathOf( fileName ), json.text() );
	mLastWrittenFrame = frame.index;
	mSecondsSinceWritten = 0.0;
	++mWrittenCount;
	return true;
}

int SpikeWriter::writtenCount() const {
	return mWrittenCount;
}

} //namespace Profiling
} //namespace GameLib

#endif // ZF_EDITOR
