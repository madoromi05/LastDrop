#include "GameSrc/DebugMode/TownBakeReport.h"

#ifdef ZF_EDITOR

#include "GameSrc/Map/TownEditFile.h"
#include "GameSrc/Map/TownBakeFile.h"

namespace{
	const char* const kLineHead = "\n  ";	// 1 行ごとの始め（F6 の窓は細いので、ファイル 1 つを 1 行にする）
}

TownBakeReport::TownBakeReport() :
	mSaved( false ) {
}

void TownBakeReport::clear() {
	mSaved = false;
	mSpots.clear();
}

void TownBakeReport::recordSave() {
	mSaved = true;
}

void TownBakeReport::recordSpot( int escapeCarSpot ) {
	mSpots.push_back( escapeCarSpot );
}

std::string TownBakeReport::text( const char* head ) const {
	std::string text = head;
	text += kLineHead;
	text += mSaved ? "saved " : "did not save ";
	text += TownEditFile::kPath;
	if ( !mSaved ) {
		text += " (no changes)";
	}
	for ( int spot : mSpots ) {
		text += kLineHead;
		text += "wrote " + TownBakeFile::navMeshPath( spot );
	}
	if ( !mSpots.empty() ) {	// 表は車の置き場所ごとに書き直すが、どれも同じ町の表なので 1 行
		text += kLineHead;
		text += "wrote ";
		text += TownBakeFile::kVisibilityPath;
	}
	return text;
}

#endif // ZF_EDITOR
