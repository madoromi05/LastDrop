#include "GameSrc/DebugMode/TuningCatalog.h"

#ifdef ZF_EDITOR

#include "GameSrc/DebugMode/TuningVisitors.h"
#include "GameSrc/Player/WeaponPoseParams.h"
#include "GameSrc/Player/LookParams.h"
#include "GameSrc/Player/AimSwayParams.h"
#include "GameSrc/Weapon/WeaponStanceParams.h"
#include "GameSrc/Zombie/ZombieCrowdParams.h"
#include "GameSrc/Zombie/ZombieAttackParams.h"
#include "GameSrc/Render/LodSelector.h"
#include "GameSrc/Zombie/ZombieUncertaintyParams.h"
#include "GameSrc/Zombie/AnimationThrottleParams.h"
#include "GameSrc/Render/PostProcessParams.h"
#include "GameSrc/Render/LightingParams.h"
#include "GameSrc/Render/ShadowParams.h"
#include "GameSrc/Render/SsaoParams.h"
#include "GameSrc/Render/PuddleParams.h"
#include "GameSrc/Render/SsrParams.h"
#include "GameSrc/Render/TaaParams.h"
#include "GameSrc/Raid/RaidParams.h"
#include "GameSrc/Interaction/LootParams.h"

#include "GameLib/PseudoXml/Document.h"
#include "GameLib/PseudoXml/Element.h"
#include "GameLib/PseudoXml/Attribute.h"
#include "ThirdParty/imgui/imgui.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>

using namespace GameLib;

namespace{
	const char* kRootName = "Tuning";
	const char* kVersionName = "version";
	const int kVersion = 1;
	const char* kTemporarySuffix = ".tmp";

	// 構造体 1 つ分。今の値（取り出す関数の先）と、既定値（作り直した構造体）を並べて渡す。
	class ITuningGroup{
	public:
		virtual ~ITuningGroup() {}
		virtual const char* elementName() const = 0; // ファイルの要素の名前
		virtual const char* structName() const = 0;  // 見出しと C++ のコピーに出す名前
		virtual const char* headerPath() const = 0;  // C++ のコピーに出す、構造体のヘッダの場所
		// 今の値と既定値の項目を、同じ順に集める。
		virtual void collect( TuningCollector* live, TuningCollector* defaults ) = 0;
	};

	template< class Params >
	class TuningGroup : public ITuningGroup{
	public:
		TuningGroup( const char* element, const char* structure, const char* header, Params& ( *access )() ) :
			mElement( element ), mStruct( structure ), mHeader( header ), mAccess( access ) {
		}
		const char* elementName() const override { return mElement; }
		const char* structName() const override { return mStruct; }
		const char* headerPath() const override { return mHeader; }
		void collect( TuningCollector* live, TuningCollector* defaults ) override {
			mAccess().visit( *live );
			mDefaults.visit( *defaults );
		}
	private:
		const char* mElement;
		const char* mStruct;
		const char* mHeader;
		Params& ( *mAccess )();
		Params mDefaults; // 作ったままの値＝コードの既定値（書き換えない）
	};

	// 調整値の一覧。構造体を足すときはここに 1 行足す。並びはエディタの見出しの順。
	std::vector< std::unique_ptr< ITuningGroup > > makeGroups() {
		std::vector< std::unique_ptr< ITuningGroup > > groups;
		groups.push_back( std::make_unique< TuningGroup< WeaponPoseParams > >(
			"WeaponPose", "WeaponPoseParams", "GameSrc/Player/WeaponPoseParams.h", &weaponPoseParams ) );
		groups.push_back( std::make_unique< TuningGroup< WeaponStanceParams > >(
			"WeaponStance", "WeaponStanceParams", "GameSrc/Weapon/WeaponStanceParams.h", &weaponStanceParams ) );
		groups.push_back( std::make_unique< TuningGroup< LookParams > >(
			"Look", "LookParams", "GameSrc/Player/LookParams.h", &lookParams ) );
		groups.push_back( std::make_unique< TuningGroup< AimSwayParams > >(
			"AimSway", "AimSwayParams", "GameSrc/Player/AimSwayParams.h", &aimSwayParams ) );
		// 1 日の時間・脱出点・スコアの重み（R2_Plan.txt。遊んで決める値）。
		groups.push_back( std::make_unique< TuningGroup< RaidParams > >(
			"Raid", "RaidParams", "GameSrc/Raid/RaidParams.h", &raidParams ) );
		// 漁れる場所の開ける秒・空になる確率・包帯の効き目（R3_Plan.txt Q13。遊んで決める値）。
		groups.push_back( std::make_unique< TuningGroup< LootParams > >(
			"Loot", "LootParams", "GameSrc/Interaction/LootParams.h", &lootParams ) );
		groups.push_back( std::make_unique< TuningGroup< ZombieCrowdParams > >(
			"ZombieCrowd", "ZombieCrowdParams", "GameSrc/Zombie/ZombieCrowdParams.h", &zombieCrowdParams ) );
		// つかんでいる間の距離・向きを合わせる値（ZombieAttack_Plan.txt 10 章）。
		groups.push_back( std::make_unique< TuningGroup< ZombieAttackParams > >(
			"ZombieAttack", "ZombieAttackParams", "GameSrc/Zombie/ZombieAttackParams.h", &zombieAttackParams ) );
		groups.push_back( std::make_unique< TuningGroup< ZombieUncertaintyParams > >(
			"ZombieUncertainty", "ZombieUncertaintyParams", "GameSrc/Zombie/ZombieUncertaintyParams.h", &zombieUncertaintyParams ) );
		// ゾンビのアニメを間引く段（OptimizationB_Plan.txt 9 章）。
		groups.push_back( std::make_unique< TuningGroup< AnimationThrottleParams > >(
			"AnimationThrottle", "AnimationThrottleParams", "GameSrc/Zombie/AnimationThrottleParams.h", &animationThrottleParams ) );
		// 置物の細かさの段（LOD）の境目（OptimizationC_Plan.txt 8 章）。
		groups.push_back( std::make_unique< TuningGroup< LodParams > >(
			"Lod", "LodParams", "GameSrc/Render/LodSelector.h", &lodParams ) );
		// 露出・ブルーム（リニア化の後に合わせ直す値。LinearToneMap_Plan.txt 7 章）。
		groups.push_back( std::make_unique< TuningGroup< PostProcessParams > >(
			"PostProcess", "PostProcessParams", "GameSrc/Render/PostProcessParams.h", &postProcessParams ) );
		// 月・空・環境光（夜の暗さ。PbrNightLighting_Plan.txt 7・11 章）。昼との切り替えは F6 → Render の Day / Night。
		groups.push_back( std::make_unique< TuningGroup< LightingParams > >(
			"Lighting", "LightingParams", "GameSrc/Render/LightingParams.h", &lightingParams ) );
		// 影を読む位置のずらし・影の濃さ（Shadow_Plan.txt 7・9 章）。
		groups.push_back( std::make_unique< TuningGroup< ShadowParams > >(
			"Shadow", "ShadowParams", "GameSrc/Render/ShadowParams.h", &shadowParams ) );
		// 物の隙間の暗がり（SsaoTranslucent_Plan.txt 5-4）。絵は F6 → Render の G-Buffer の SSAO で見る。
		groups.push_back( std::make_unique< TuningGroup< SsaoParams > >(
			"SSAO", "SsaoParams", "GameSrc/Render/SsaoParams.h", &ssaoParams ) );
		// 雨上がりの水たまり（SsrDecalMaterial_Plan.txt 6-2）。道路の材質だけに効く。
		groups.push_back( std::make_unique< TuningGroup< PuddleParams > >(
			"Puddle", "PuddleParams", "GameSrc/Render/PuddleParams.h", &puddleParams ) );
		// 映り込み（SsrDecalMaterial_Plan.txt 7-5）。絵は F6 → Render の G-Buffer の SSR で見る。
		groups.push_back( std::make_unique< TuningGroup< SsrParams > >(
			"SSR", "SsrParams", "GameSrc/Render/SsrParams.h", &ssrParams ) );
		// TAA（Taa_Plan.txt 7-5）。F6 → Render のアンチエイリアスが TAA のときだけ効く。
		groups.push_back( std::make_unique< TuningGroup< TaaParams > >(
			"TAA", "TaaParams", "GameSrc/Render/TaaParams.h", &taaParams ) );
		return groups;
	}

	std::vector< std::unique_ptr< ITuningGroup > >& groups() {
		static std::vector< std::unique_ptr< ITuningGroup > > sGroups = makeGroups();
		return sGroups;
	}

	ITuningGroup* findGroup( const char* elementName ) {
		for ( auto& group : groups() ) {
			if ( std::strcmp( group->elementName(), elementName ) == 0 ) {
				return group.get();
			}
		}
		return nullptr;
	}

	const TuningItem* findItem( const std::vector< TuningItem >& items, const char* name ) {
		for ( const TuningItem& item : items ) {
			if ( std::strcmp( item.name, name ) == 0 ) {
				return &item;
			}
		}
		return nullptr;
	}

	bool readText( const std::string& path, std::string* text ) {
		std::ifstream ifs( path, std::ios::binary );
		if ( !ifs ) {
			return false;
		}
		std::ostringstream buffer;
		buffer << ifs.rdbuf();
		*text = buffer.str();
		return true;
	}

	// 1 つの要素（構造体 1 つ分）の属性を、その構造体の項目へ入れる。
	void loadGroup( PseudoXml::ConstElement element, ITuningGroup* group, std::vector< std::string >* warnings ) {
		TuningCollector live;
		TuningCollector defaults;
		group->collect( &live, &defaults );
		for ( int i = 0; i < element.attributeNumber(); ++i ) {
			PseudoXml::ConstAttribute attribute = element.attribute( i );
			const TuningItem* item = findItem( live.items(), attribute.name() );
			std::string where = std::string( group->elementName() ) + "." + attribute.name();
			if ( !item ) {
				warnings->push_back( "Unknown item: " + where );
				continue;
			}
			// 読めた数が項目の値の数に届かなければ、その項目は既定値のまま（途中まで入れない）。
			const int kMaxCount = 3;
			float values[ kMaxCount ];
			if ( attribute.getFloatValues( values, item->count ) != item->count ) {
				warnings->push_back( "Unreadable value: " + where );
				continue;
			}
			for ( int c = 0; c < item->count; ++c ) {
				item->values[ c ] = values[ c ];
			}
		}
	}
}

namespace TuningCatalog{

void drawAll() {
	for ( auto& group : groups() ) {
		if ( !ImGui::TreeNodeEx( group->structName(), ImGuiTreeNodeFlags_DefaultOpen ) ) {
			continue;
		}
		TuningCollector live;
		TuningCollector defaults;
		group->collect( &live, &defaults );
		for ( size_t i = 0; i < live.items().size(); ++i ) {
			TuningVisitors::draw( live.items()[ i ], defaults.items()[ i ] );
		}
		ImGui::TreePop();
	}
}

void resetAll() {
	for ( auto& group : groups() ) {
		TuningCollector live;
		TuningCollector defaults;
		group->collect( &live, &defaults );
		for ( size_t i = 0; i < live.items().size(); ++i ) {
			TuningVisitors::copyValues( defaults.items()[ i ], live.items()[ i ] );
		}
	}
}

bool save( const std::string& path, std::string* error ) {
	PseudoXml::Document doc = PseudoXml::Document::create();
	PseudoXml::Element root = doc.root();
	root.setChildNumber( 1 );
	PseudoXml::Element tuning = root.child( 0 );
	tuning.setName( kRootName );
	tuning.setAttributeNumber( 1 );
	tuning.attribute( 0 ).set( kVersionName, kVersion );
	tuning.setChildNumber( static_cast< int >( groups().size() ) );
	for ( size_t g = 0; g < groups().size(); ++g ) {
		TuningCollector live;
		TuningCollector defaults;
		groups()[ g ]->collect( &live, &defaults );
		PseudoXml::Element element = tuning.child( static_cast< int >( g ) );
		element.setName( groups()[ g ]->elementName() );
		element.setAttributeNumber( static_cast< int >( live.items().size() ) );
		for ( size_t i = 0; i < live.items().size(); ++i ) {
			const TuningItem& item = live.items()[ i ];
			// 値は自分で文字にする（PseudoXml の float の配列は 4 桁で丸めるので、元の値へ戻らない）。
			element.attribute( static_cast< int >( i ) ).set( item.name, TuningVisitors::fileText( item ).c_str() );
		}
	}
	std::string text;
	doc.convertToString( &text );

	std::error_code code;
	std::filesystem::path target( path );
	if ( target.has_parent_path() ) {
		std::filesystem::create_directories( target.parent_path(), code );
	}
	std::string temporary = path + kTemporarySuffix;
	{
		std::ofstream ofs( temporary, std::ios::binary );
		if ( !ofs ) {
			*error = "Cannot open " + temporary;
			return false;
		}
		ofs.write( text.data(), static_cast< std::streamsize >( text.size() ) );
		if ( !ofs ) {
			*error = "Cannot write " + temporary;
			return false;
		}
	}
	// MSVC の rename は、置き換え先があっても上書きする（MoveFileEx の MOVEFILE_REPLACE_EXISTING と同じ）。
	std::filesystem::rename( temporary, target, code );
	if ( code ) {
		*error = "Cannot replace " + path + " (" + code.message() + ")";
		return false;
	}
	return true;
}

bool load( const std::string& path, std::vector< std::string >* warnings ) {
	std::string text;
	if ( !readText( path, &text ) || text.empty() ) {
		warnings->push_back( "Cannot read " + path );
		return false;
	}
	PseudoXml::Document doc = PseudoXml::Document::create( text.data(), static_cast< int >( text.size() ) );
	if ( !doc || doc.isError() ) {
		warnings->push_back( "Broken file: " + path );
		return false;
	}
	resetAll();
	// 書いた要素はルートの子になる（ルートそのものは名前を持たない入れ物。GameSettings と同じ）。
	PseudoXml::ConstElement root = doc.root();
	bool found = false;
	for ( int r = 0; r < root.childNumber(); ++r ) {
		PseudoXml::ConstElement tuning = root.child( r );
		if ( !tuning.name() || std::strcmp( tuning.name(), kRootName ) != 0 ) {
			continue;
		}
		found = true;
		int version = 0;
		for ( int a = 0; a < tuning.attributeNumber(); ++a ) {
			if ( std::strcmp( tuning.attribute( a ).name(), kVersionName ) == 0 ) {
				version = tuning.attribute( a ).getIntValue();
			}
		}
		if ( version != kVersion ) {
			warnings->push_back( "Version " + std::to_string( version ) + " (expected " + std::to_string( kVersion ) + ")" );
		}
		for ( int e = 0; e < tuning.childNumber(); ++e ) {
			PseudoXml::ConstElement element = tuning.child( e );
			const char* name = element.name() ? element.name() : "";
			ITuningGroup* group = findGroup( name );
			if ( !group ) {
				warnings->push_back( std::string( "Unknown group: " ) + name );
				continue;
			}
			loadGroup( element, group, warnings );
		}
	}
	if ( !found ) {
		warnings->push_back( std::string( "No <" ) + kRootName + "> in " + path );
	}
	return true;
}

std::string toCpp( int* changedNumber ) {
	std::string text;
	int changed = 0;
	for ( auto& group : groups() ) {
		TuningCollector live;
		TuningCollector defaults;
		group->collect( &live, &defaults );
		std::string lines;
		for ( size_t i = 0; i < live.items().size(); ++i ) {
			if ( TuningVisitors::differs( live.items()[ i ], defaults.items()[ i ] ) ) {
				lines += "\t" + TuningVisitors::cppLine( live.items()[ i ] ) + "\n";
				++changed;
			}
		}
		if ( !lines.empty() ) {
			text += std::string( "// " ) + group->structName() + "（" + group->headerPath() + "）\n" + lines;
		}
	}
	if ( changedNumber ) {
		*changedNumber = changed;
	}
	return text;
}

} //namespace TuningCatalog

#endif // ZF_EDITOR
