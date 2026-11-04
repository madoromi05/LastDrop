#include "GameSrc/Raid/RunState.h"

#include "GameLib/GameLib.h"
#include "GameLib/PseudoXml/Document.h"
#include "GameLib/PseudoXml/Element.h"
#include "GameLib/PseudoXml/Attribute.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

using namespace GameLib;

namespace{
	// 置き場所は settings.xml と同じく、起動した場所。
	const char* kFilePath = "save.xml";
	const char* kTemporarySuffix = ".tmp";
	const int kVersion = 1; // 形を変えたら上げる（古いファイルは、足りない項目を既定値で読む）

	//   <Save version="1">
	//     <Run day="2" kills="31">
	//       <Grid name="stash" columns="6" rows="8"> <Item cell="0" item="ammo_9mm" amount="48"/> … </Grid>
	//       <Grid name="bag" columns="2" rows="4"> … </Grid>
	//     </Run>
	//     <Record bestScore="3960"/>
	//   </Save>
	const char* kSaveName = "Save";
	const char* kRunName = "Run";
	const char* kGridName = "Grid";
	const char* kItemName = "Item";
	const char* kRecordName = "Record";
	const char* kAttrVersion = "version";
	const char* kAttrDay = "day";
	const char* kAttrKills = "kills";
	const char* kAttrName = "name";
	const char* kAttrColumns = "columns";
	const char* kAttrRows = "rows";
	const char* kAttrCell = "cell";
	const char* kAttrItem = "item";
	const char* kAttrAmount = "amount";
	const char* kAttrLoaded = "loaded";
	const char* kAttrBestScore = "bestScore";
	const char* kStashGridName = "stash";
	const char* kBagGridName = "bag";

	const int kFirstDay = 1;
	const int kRunAttributeNumber = 2;   // day・kills
	const int kGridAttributeNumber = 3;  // name・columns・rows
	const int kItemAttributeNumber = 4;  // cell・item・amount・loaded
	const int kRunChildNumber = 2;       // 倉庫・カバン
	const int kSaveChildNumber = 2;      // Run・Record

	bool isNamed( const PseudoXml::ConstElement& e, const char* name ) {
		return e.name() && std::strcmp( e.name(), name ) == 0;
	}

	// e の属性 name の数（無ければ fallback）。
	int intAttribute( const PseudoXml::ConstElement& e, const char* name, int fallback ) {
		for ( int i = 0; i < e.attributeNumber(); ++i ) {
			PseudoXml::ConstAttribute a = e.attribute( i );
			if ( a.name() && std::strcmp( a.name(), name ) == 0 ) {
				return a.getIntValue();
			}
		}
		return fallback;
	}

	// e の属性 name の文字列（無ければ nullptr）。
	const char* textAttribute( const PseudoXml::ConstElement& e, const char* name ) {
		for ( int i = 0; i < e.attributeNumber(); ++i ) {
			PseudoXml::ConstAttribute a = e.attribute( i );
			if ( a.name() && std::strcmp( a.name(), name ) == 0 ) {
				return a.value();
			}
		}
		return nullptr;
	}

	void writeGrid( PseudoXml::Element e, const char* name, const ItemGrid& grid ) {
		e.setName( kGridName );
		e.setAttributeNumber( kGridAttributeNumber );
		e.attribute( 0 ).set( kAttrName, name );
		e.attribute( 1 ).set( kAttrColumns, grid.columns() );
		e.attribute( 2 ).set( kAttrRows, grid.rows() );
		int owners = 0;
		for ( int i = 0; i < grid.cellCount(); ++i ) {
			if ( grid.ownerCell( i ) == i ) {
				++owners;
			}
		}
		e.setChildNumber( owners );
		int index = 0;
		for ( int i = 0; i < grid.cellCount(); ++i ) {
			if ( grid.ownerCell( i ) != i ) {
				continue;
			}
			const ItemStack& stack = grid.stackAt( i );
			PseudoXml::Element item = e.child( index++ );
			item.setName( kItemName );
			item.setAttributeNumber( kItemAttributeNumber );
			item.attribute( 0 ).set( kAttrCell, i );
			item.attribute( 1 ).set( kAttrItem, itemSpec( stack.item ).saveName );
			item.attribute( 2 ).set( kAttrAmount, stack.amount );
			item.attribute( 3 ).set( kAttrLoaded, stack.loadedRounds );
		}
	}

	// e（Grid）の物を grid へ置く。列の数が違うファイル（形を変えた後）は、行と列で同じ位置へ置き直す。
	// 広さ（行の数）はファイルの値に合わせない（広さを決めるのは強化の段。段階R5 で RunState に持つ）。
	void readGrid( const PseudoXml::ConstElement& e, ItemGrid* grid ) {
		int savedColumns = intAttribute( e, kAttrColumns, grid->columns() );
		if ( savedColumns <= 0 ) {
			return;
		}
		for ( int i = 0; i < e.childNumber(); ++i ) {
			PseudoXml::ConstElement item = e.child( i );
			if ( !isNamed( item, kItemName ) ) {
				continue;
			}
			ItemId id = itemFromSaveName( textAttribute( item, kAttrItem ) );
			int amount = intAttribute( item, kAttrAmount, 0 );
			int cell = intAttribute( item, kAttrCell, -1 );
			if ( id == ITEM_NONE || amount <= 0 || amount > itemSpec( id ).stackSize || cell < 0 ) {
				continue; // 知らない物・壊れた数は読み飛ばす
			}
			int row = cell / savedColumns;
			int column = cell % savedColumns;
			if ( column >= grid->columns() ) {
				continue;
			}
			ItemStack stack = { id, amount };
			stack.loadedRounds = intAttribute( item, kAttrLoaded, 0 );
			grid->placeAt( row * grid->columns() + column, stack ); // はみ出す・重なるなら置かない
		}
	}
}

RunState::RunState() :
	mDay( kFirstDay ),
	mStash( kStashColumns, kStashRows ),
	mBag( kBagColumns, kBagRows ),
	mKills( 0 ),
	mBestScore( 0 ),
	mRunEndPending( false ) {
}

int RunState::day() const {
	return mDay;
}

ItemGrid& RunState::stash() {
	return mStash;
}

const ItemGrid& RunState::stash() const {
	return mStash;
}

ItemGrid& RunState::bag() {
	return mBag;
}

const ItemGrid& RunState::bag() const {
	return mBag;
}

int RunState::kills() const {
	return mKills;
}

void RunState::setDay( int day ) {
	ASSERT( day >= kFirstDay && "RunState::setDay : bad day." );
	mDay = day;
}

void RunState::setKills( int kills ) {
	mKills = kills;
}

int RunState::bestScore() const {
	return mBestScore;
}

void RunState::setBestScore( int score ) {
	mBestScore = score;
}

const DayResult& RunState::lastDay() const {
	return mLastDay;
}

void RunState::setLastDay( const DayResult& result ) {
	mLastDay = result;
}

const RunEnd& RunState::lastRunEnd() const {
	return mLastRunEnd;
}

void RunState::setLastRunEnd( const RunEnd& end ) {
	mLastRunEnd = end;
}

bool RunState::isRunEndPending() const {
	return mRunEndPending;
}

void RunState::setRunEndPending( bool pending ) {
	mRunEndPending = pending;
}

bool RunState::save() const {
	PseudoXml::Document doc = PseudoXml::Document::create();
	PseudoXml::Element root = doc.root();
	root.setChildNumber( 1 );
	PseudoXml::Element save = root.child( 0 );
	save.setName( kSaveName );
	save.setAttributeNumber( 1 );
	save.attribute( 0 ).set( kAttrVersion, kVersion );
	save.setChildNumber( kSaveChildNumber );

	PseudoXml::Element run = save.child( 0 );
	run.setName( kRunName );
	run.setAttributeNumber( kRunAttributeNumber );
	run.attribute( 0 ).set( kAttrDay, mDay );
	run.attribute( 1 ).set( kAttrKills, mKills );
	run.setChildNumber( kRunChildNumber );
	writeGrid( run.child( 0 ), kStashGridName, mStash );
	writeGrid( run.child( 1 ), kBagGridName, mBag );

	PseudoXml::Element record = save.child( 1 );
	record.setName( kRecordName );
	record.setAttributeNumber( 1 );
	record.attribute( 0 ).set( kAttrBestScore, mBestScore );

	std::string text;
	doc.convertToString( &text );
	std::string temporary = std::string( kFilePath ) + kTemporarySuffix;
	{
		std::ofstream ofs( temporary, std::ios::binary );
		if ( !ofs ) {
			return false;
		}
		ofs.write( text.data(), static_cast< std::streamsize >( text.size() ) );
		if ( !ofs ) {
			return false;
		}
	}
	// MSVC の rename は、置き換え先があっても上書きする（TuningCatalog と同じ）。
	std::error_code code;
	std::filesystem::rename( temporary, kFilePath, code );
	return !code;
}

bool RunState::load() {
	std::ifstream ifs( kFilePath, std::ios::binary );
	if ( !ifs ) {
		return false;
	}
	std::ostringstream buffer;
	buffer << ifs.rdbuf();
	std::string text = buffer.str();
	if ( text.empty() ) {
		return false;
	}
	PseudoXml::Document doc = PseudoXml::Document::create( text.data(), static_cast< int >( text.size() ) );
	if ( !doc || doc.isError() ) {
		return false;
	}
	PseudoXml::ConstElement root = doc.root();
	for ( int i = 0; i < root.childNumber(); ++i ) {
		PseudoXml::ConstElement save = root.child( i );
		if ( !isNamed( save, kSaveName ) ) {
			continue;
		}
		// 読めると分かってから中身を入れ替える（途中で読めなくなっても、半端な中身にしない）。
		ItemGrid stash( kStashColumns, kStashRows );
		ItemGrid bag( kBagColumns, kBagRows );
		int day = kFirstDay;
		int kills = 0;
		int bestScore = 0;
		for ( int j = 0; j < save.childNumber(); ++j ) {
			PseudoXml::ConstElement e = save.child( j );
			if ( isNamed( e, kRunName ) ) {
				day = intAttribute( e, kAttrDay, kFirstDay );
				kills = intAttribute( e, kAttrKills, 0 );
				for ( int k = 0; k < e.childNumber(); ++k ) {
					PseudoXml::ConstElement grid = e.child( k );
					const char* name = textAttribute( grid, kAttrName );
					if ( !isNamed( grid, kGridName ) || !name ) {
						continue;
					}
					if ( std::strcmp( name, kStashGridName ) == 0 ) {
						readGrid( grid, &stash );
					} else if ( std::strcmp( name, kBagGridName ) == 0 ) {
						readGrid( grid, &bag );
					}
				}
			} else if ( isNamed( e, kRecordName ) ) {
				bestScore = intAttribute( e, kAttrBestScore, 0 );
			}
		}
		// 日はクリアの手前（kDayCount）までに収める（手で直したファイルでも、5 日目より先から始めない）。
		mDay = ( day < kFirstDay ) ? kFirstDay : ( ( day > kDayCount ) ? kDayCount : day );
		mKills = ( kills < 0 ) ? 0 : kills;
		mBestScore = ( bestScore < 0 ) ? 0 : bestScore;
		mStash = stash;
		mBag = bag;
		return true;
	}
	return false;
}

void RunState::erase() {
	std::error_code code;
	std::filesystem::remove( kFilePath, code ); // 無くても構わない
}
