#include "GameSrc/DebugMode/TownBakePanel.h"

#ifdef ZF_EDITOR

#include "GameSrc/DebugMode/TownBakeChain.h"
#include "GameSrc/DebugMode/TownBakeStatus.h"
#include "GameSrc/DebugMode/TownBakeReport.h"
#include "GameSrc/DebugMode/EditorColors.h"
#include "GameSrc/Map/TownMap.h"
#include "GameSrc/Map/TownEditFile.h"
#include "GameSrc/Map/TownEditFileState.h"
#include "GameSrc/Map/TownCache.h"
#include "GameSrc/Map/NavMesh.h"
#include "GameSrc/Scene/Game/World/TownEnvironment.h"
#include "GameSrc/Scene/Game/World/ZombieDirector.h"
#include "GameSrc/Scene/SceneManager.h"

#include "GameLib/FileIO/Manager.h"
#include "GameLib/Math/Functions.h"
#include "ThirdParty/imgui/imgui.h"

#include <cstdio>
#include <string>
#include <vector>

namespace{
	const float kUnknownRadius = -1.f;	// Release の既定の長さを、まだ求めていない
	const float kTownCenter = 0.f;		// 町の真ん中（world の原点。-Z が北・+X が東）
	const int kTextSize = 128;

	// 欄の状態。F6 の欄は関数だけの namespace なので、描画をまたいで覚える物はここに置く
	TownBakeChain gChain;					// Bake・Bake all の、まだ書いていない車の置き場所
	TownBakeStatus gStatus;					// 床と表のファイルの状態（最後に求め直したときの物）
	bool gStatusDirty = true;				// 書き出した・読み直したので、状態を求め直すか
	unsigned gStatusRevision = 0;			// 状態を求めたときの手直しの revision（変わったら求め直す）
	float gDefaultRadius = kUnknownRadius;	// Release が床を作る長さ。ゾンビの体の頂点を全部見るので、起動中 1 回だけ求める
	TownBakeReport gReport;					// 今の Bake・Bake all で保存・書いた物（終わったときの知らせに全部出す）
	std::string gMessage;					// 最後の Bake の様子・結果
	bool gMessageIsError = false;			// gMessage が止まった理由か（黄色にする）

	void setMessage( const std::string& text, bool isError ) {
		gMessage = text;
		gMessageIsError = isError;
	}

	std::string spotLabel( const TownMap& map, int spot ) {	// 車の置き場所の、F6 に出す名前。例は spot 0 (north)。TownMap の名前は日本語で、F6 の字形に無いため
		const TownPlacement& at = map.escapeCarSpots()[ spot ].placement;
		const char* direction = nullptr;
		if ( GameLib::Math::abs( at.z - kTownCenter ) >= GameLib::Math::abs( at.x - kTownCenter ) ) {	// 町の真ん中から見て、離れている向きの方角
			direction = ( at.z < kTownCenter ) ? "north" : "south";
		} else {
			direction = ( at.x > kTownCenter ) ? "east" : "west";
		}
		char text[ kTextSize ];
		std::snprintf( text, sizeof( text ), "spot %d (%s)", spot, direction );
		return text;
	}

	float defaultRadius( const TownEditPanel::Context& context ) {	// Release が床を作る長さ。まだ求めていなければ、今の出撃のゾンビの係から求める
		if ( gDefaultRadius == kUnknownRadius && context.zombies ) {
			gDefaultRadius = context.zombies->defaultNavMeshRadius();
		}
		return gDefaultRadius;
	}

	bool canBake( const TownEditPanel::Context& context ) {	// 読み終えた出撃と、書き出す材料がそろっているか
		return context.edits && context.file && context.town && context.record && context.cache && context.collisionWorld && context.zombies;
	}

	bool loadedIsCurrent( const TownEditPanel::Context& context ) {	// 今の出撃の町が、今の手直しを当てた町と同じ床・表になるか
		const TownMap edited( context.edits, townEditLimits() );
		return TownBakeStatus::bakesSame( context.town->map(), edited, context.escapeCarSpot, defaultRadius( context ) );
	}

	// ── 1 段ずつ進める ──
	bool saveEdits( const TownEditPanel::Context& context, std::string* error ) {	// 手直しを TownEdit.xml へ保存する。書けなければ false と、その理由を error へ入れる
		if ( !TownEditFile::save( TownEditFile::kPath, *context.edits, error ) ) {
			return false;
		}
		context.file->saved( *context.edits );
		gReport.recordSave();
		return true;
	}

	// 今の出撃の車の置き場所の床と、表を書く。手直しをファイルから変えていれば、先に TownEdit.xml へ保存する（次の起動が同じ町を組み立てて、指紋が合うように）
	// 書くのは手直しと同じ町だと確かめた出撃だけなので、ここで保存すれば、ファイルの手直しと書いた床が必ずそろう
	// ファイルに読めなかった行があれば、保存するとその行が消えるので、保存も書き出しもせずに止まる（直すか Save を押すかは使う人が決める）
	void bakeLoadedSpot( const TownEditPanel::Context& context ) {
		const TownEditFileState::BakeSave save = context.file->bakeSave( *context.edits, GameLib::FileIO::Manager().exists( TownEditFile::kPath ) );
		if ( save == TownEditFileState::BakeSave::Refuse ) {
			gChain.finishBake( false );
			setMessage( "Bake stopped: TownEdit.xml has unread rows. Fix it or press Save.", true );
			return;
		}
		std::string error;
		const bool written = ( save != TownEditFileState::BakeSave::Save || saveEdits( context, &error ) ) &&
			context.cache->bake( context.escapeCarSpot, context.town->map(), *context.collisionWorld, context.town->buildings(),
				defaultRadius( context ), &error );
		gChain.finishBake( written );
		gStatusDirty = true;
		if ( !written ) {
			setMessage( "Bake stopped: " + error, true );
			return;
		}
		gReport.recordSpot( context.escapeCarSpot );
		setMessage( gReport.text( "Baking:" ), false );
	}

	void requestReload( const TownEditPanel::Context& context, TownEditReload* reload ) {	// 続きの車の置き場所で出撃を読み直す。日の条件は今のまま
		const int spot = gChain.reloadSpot();
		reload->begin( context.record, context.today, spot );
		SceneManager::instance().requestRetryRaid();
		setMessage( "Reloading the raid with " + spotLabel( context.town->map(), spot ) + "...", false );
	}

	void finishChain() {	// 途中の知らせは読み直しの知らせで消えるので、終わったときに、保存したか・書いたファイルを全部出し直す
		setMessage( gReport.text( "Bake finished:" ), false );
	}

	void step( const TownEditPanel::Context& context, TownEditReload* reload ) {	// 読み終えた出撃で、Bake・Bake all を 1 段進める
		switch ( gChain.next( context.escapeCarSpot, loadedIsCurrent( context ) ) ) {
			case TownBakeChain::Step::Bake:
				bakeLoadedSpot( context );
				break;
			case TownBakeChain::Step::Reload:
				requestReload( context, reload );
				break;
			case TownBakeChain::Step::Done:
				finishChain();
				break;
			case TownBakeChain::Step::OutOfDate:
				setMessage( "Bake stopped: the edits changed while reloading. Press Bake again.", true );
				break;
			case TownBakeChain::Step::Idle:
				break;
		}
	}

	void startBake( const TownEditPanel::Context& context, const std::vector< int >& spots ) {	// spots の床を書き始める。書くのは次の update から
		gReport.clear();
		gChain.start( spots, context.escapeCarSpot );
		setMessage( "Baking...", false );
	}

	// ── 欄 ──
	void refreshStatus( const TownEditPanel::Context& context ) {	// 手直しが変わったか、書き出した・読み直した後だけ、ファイルの状態を求め直す
		const bool editsChanged = ( context.edits->revision() != gStatusRevision );
		if ( !gStatusDirty && !editsChanged ) {
			return;
		}
		if ( ImGui::IsAnyItemActive() ) {	// 数字の欄をドラッグしている間は手直しが毎フレーム変わるので、離してから読む（ファイルを全部読むため）
			return;
		}
		const TownMap edited( context.edits, townEditLimits() );
		gStatus.update( edited, defaultRadius( context ) );
		gStatusRevision = context.edits->revision();
		gStatusDirty = false;
	}

	void drawStateRow( const char* label, TownBakeStatus::State state, bool thisRaid ) {	// 1 行。使えないファイルは黄色
		const char* suffix = thisRaid ? "  (this raid)" : "";
		if ( state == TownBakeStatus::State::File ) {
			ImGui::Text( "%s: %s%s", label, TownBakeStatus::stateName( state ), suffix );
		} else {
			ImGui::TextColored( EditorColors::kWarning, "%s: %s%s", label, TownBakeStatus::stateName( state ), suffix );
		}
	}

	void drawStatus( const TownEditPanel::Context& context ) {	// 車の置き場所ごとの床と、表の状態。今の出撃の床をどこから得たか
		const std::vector< TownBakeStatus::State >& navMeshes = gStatus.navMeshes();
		for ( int spot = 0; spot < static_cast< int >( navMeshes.size() ); ++spot ) {
			drawStateRow( spotLabel( context.town->map(), spot ).c_str(), navMeshes[ spot ], spot == context.escapeCarSpot );
		}
		drawStateRow( "visibility", gStatus.visibility(), false );
		const bool fromFile = context.cache->isNavMeshFromFile( context.escapeCarSpot );
		ImGui::TextDisabled( fromFile ? "This raid's floor was loaded from the file." : "This raid's floor was built at load." );
		if ( !gStatus.isUpToDate() ) {
			ImGui::TextColored( EditorColors::kWarning, "Bake files are out of date: the game builds them at load." );
		}
	}

	void drawButtons( const TownEditPanel::Context& context, const TownEditReload& reload ) {
		const bool busy = gChain.isRunning() || reload.isPending();
		ImGui::BeginDisabled( busy || !canBake( context ) );
		if ( ImGui::Button( "Bake" ) ) {
			startBake( context, { context.escapeCarSpot } );
		}
		ImGui::SameLine();
		const std::vector< int > staleSpots = gStatus.spotsToBake( context.escapeCarSpot );
		ImGui::BeginDisabled( staleSpots.empty() );
		if ( ImGui::Button( "Bake all" ) ) {
			startBake( context, staleSpots );
		}
		ImGui::EndDisabled();
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::TextDisabled( "(this car spot / every stale one)" );
	}

	void drawNotes( const TownEditPanel::Context& context ) {	// 書き出しの決まりの注意
		if ( context.town->navMesh().agentRadius() != defaultRadius( context ) ) {	// F6 で余白を変えた出撃の床は、Bake が書く床と違う
			ImGui::TextColored( EditorColors::kWarning, "Release builds the floor with the default radius (%.2f m, this raid %.2f m).",
				defaultRadius( context ), context.town->navMesh().agentRadius() );
		}
		ImGui::TextDisabled( "Bake again after replacing a building model (the files only check placements)." );
	}
}

namespace TownBakePanel{

bool isRunning() {
	return gChain.isRunning();
}

void update( const TownEditPanel::Context& context, TownEditReload* reload, TownEditReload::Result reloaded ) {
	if ( reloaded == TownEditReload::Result::Loaded || reloaded == TownEditReload::Result::Replaced ) {
		gStatusDirty = true;	// Apply・Bake の後は、状態を読み直して見せる
	}
	if ( !gChain.isRunning() ) {
		return;
	}
	if ( reloaded == TownEditReload::Result::Replaced ) {	// 別の出撃に替わったので、その出撃で続けずに止める
		gChain.stop();
		setMessage( "Bake stopped: another scene change replaced the reload.", true );
		return;
	}
	if ( reload->isPending() || !canBake( context ) ) {	// 読み直した出撃を読み終えるまでは書かない
		return;
	}
	step( context, reload );
}

void draw( const TownEditPanel::Context& context, const TownEditReload& reload ) {
	ImGui::SeparatorText( "Bake" );
	if ( !canBake( context ) ) {
		ImGui::TextDisabled( "Cannot bake in this raid." );
		return;
	}
	refreshStatus( context );
	drawStatus( context );
	drawButtons( context, reload );
	drawNotes( context );
	if ( !gMessage.empty() ) {
		if ( gMessageIsError ) {
			ImGui::TextColored( EditorColors::kWarning, "%s", gMessage.c_str() );
		} else {
			ImGui::TextDisabled( "%s", gMessage.c_str() );
		}
	}
}

} //namespace TownBakePanel

#endif // ZF_EDITOR
