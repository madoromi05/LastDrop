#include "GameSrc/Map/TownEditFileState.h"
#include "GameSrc/Map/TownEdits.h"

TownEditFileState::TownEditFileState() :
	mRevision( 0 ) {
}

void TownEditFileState::loaded( const TownEdits& edits, const std::vector< std::string >& warnings ) {
	mWarnings = warnings;
	mRevision = edits.revision();
}

void TownEditFileState::saved( const TownEdits& edits ) {
	mWarnings.clear();
	mRevision = edits.revision();
}

const std::vector< std::string >& TownEditFileState::warnings() const {
	return mWarnings;
}

TownEditFileState::BakeSave TownEditFileState::bakeSave( const TownEdits& edits, bool fileExists ) const {
	if ( !mWarnings.empty() ) {	// 読めなかった行は手直しに無いので、書くとファイルから消える。手直しを変えていなくても、黙って書かずに止める
		return BakeSave::Refuse;
	}
	if ( edits.revision() == mRevision ) {	// 最後に読んだ・書いたときから変えていないので、ファイルは手直しのまま
		return BakeSave::Keep;
	}
	if ( edits.isEmpty() && !fileExists ) {	// 手直しもファイルも無ければ、空のファイルを作らない
		return BakeSave::Keep;
	}
	return BakeSave::Save;
}
