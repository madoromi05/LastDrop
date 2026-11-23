#include "GameSrc/DebugMode/TownEditPanel.h"

#ifdef ZF_EDITOR

#include "GameSrc/DebugMode/TownBakePanel.h"
#include "GameSrc/DebugMode/EditorColors.h"
#include "GameSrc/DebugMode/TownEditFrame.h"
#include "GameSrc/DebugMode/TownEditOriginals.h"
#include "GameSrc/DebugMode/TownEditReload.h"
#include "GameSrc/Map/TownEditFile.h"
#include "GameSrc/Map/TownEditFileState.h"
#include "GameSrc/Scene/Game/World/TownEnvironment.h"
#include "GameSrc/Scene/SceneManager.h"
#include "GameSrc/Player/Camera/FirstPersonCamera.h"
#include "GameSrc/Math/AngleMath.h"

#include "GameLib/DebugUi/DebugUi.h"
#include "GameLib/Math/Vector3.h"
#include "ThirdParty/imgui/imgui.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace GameLib::Math;

namespace{
	const int kKindNumber = static_cast< int >( TownEditKind::Count );
	const ImVec4 kEditedColor( 1.f, 0.85f, 0.2f, 1.f );		// 一覧の、手直しした物・足した物（枠の今の位置と同じ黄色）
	const float kPositionDragSpeed = 0.05f;					// 位置の欄をドラッグしたときの 1 画素ぶん（m）
	const float kYawDragSpeed = 0.5f;						// 向きの欄の 1 画素ぶん（度）
	const float kNoDragLimit = 0.f;							// ドラッグの下限・上限を付けない（ImGui は下限と上限が同じなら付けない）
	const char* const kPositionFormat = "%.2f";				// cm まで
	const char* const kYawFormat = "%.1f";
	const int kPositionAxes = 3;							// x・y・z
	const float kAddedYaw = 0.f;							// 足した物の向き（置いた後に欄で回す）
	const int kTextSize = 128;
	const int kNoIndex = -1;

	// 選んでいる物。元の物は種類と元の並びの番号、足した物は TownEdits::additions の添字で見分ける
	struct Selection{
		bool active;
		TownEditKind kind;
		bool added;
		int index;
	};

	Selection gSelection = { false, TownEditKind::Building, false, kNoIndex };
	TownEditPanel::Highlight gHighlight = {};		// 最後に欄を描いたときの、選んでいる物の位置
	bool gHasHighlight = false;
	std::string gStatus;							// 最後の操作の結果（灰色の字）
	bool gStatusIsError = false;					// gStatus が、できなかった理由か（黄色にする）
	TownEditReload gReload;							// Apply・Bake の出撃の読み直しを待つ係（1 度に 1 つだけ読み直す）

	bool isEdited( const TownEdits& edits, const TownEditKey& key ) {
		return edits.movedTo( key ) || edits.isRemoved( key );
	}

	const TownPlacement* currentOf( const TownEdits& edits, const Selection& selection ) {	// 選んでいる物の今の位置。消した物は nullptr
		if ( selection.added ) {
			return &edits.additions()[ selection.index ].at;
		}
		return TownEditOriginals::placedAt( edits, selection.kind, selection.index );
	}

	void dropMissingSelection( const TownEdits& edits ) {	// 足した物を選んでいて、Revert などでその物が無くなったら選ぶのをやめる
		if ( !gSelection.active || !gSelection.added ) {
			return;
		}
		const std::vector< TownEdits::Addition >& additions = edits.additions();
		if ( gSelection.index >= static_cast< int >( additions.size() ) || additions[ gSelection.index ].kind != gSelection.kind ) {
			gSelection.active = false;
		}
	}

	std::string selectionName( const Selection& selection ) {
		char text[ kTextSize ];
		if ( selection.added ) {
			std::snprintf( text, sizeof( text ), "%s +%d (added)", townEditKindName( selection.kind ), selection.index );
		} else {
			std::snprintf( text, sizeof( text ), "%s #%d", townEditKindName( selection.kind ), selection.index );
		}
		return text;
	}

	void setStatus( const std::string& text, bool isError ) {
		gStatus = text;
		gStatusIsError = isError;
	}

	// ── 上の段（Save・Revert・Apply）──
	void save( const TownEdits& edits, TownEditFileState* file ) {
		std::string error;
		if ( TownEditFile::save( TownEditFile::kPath, edits, &error ) ) {
			file->saved( edits );	// ファイルが今の手直しになったので、読めなかった行の警告も消える
			setStatus( std::string( "Saved " ) + TownEditFile::kPath, false );
		} else {
			setStatus( error, true );
		}
	}

	void revert( TownEdits* edits, TownEditFileState* file ) {
		std::vector< std::string > warnings;
		const std::string path = TownEditFile::kPath;
		const bool loaded = TownEditFile::load( TownEditFile::kPath, edits, &warnings );
		file->loaded( *edits, warnings );
		if ( loaded ) {
			setStatus( "Reverted to " + path + " (Apply rebuilds the town)", false );
		} else if ( warnings.empty() ) {
			setStatus( "No " + path + ": edits cleared", false );
		} else {
			setStatus( "Cannot read " + path + ": edits cleared", true );
		}
	}

	void apply( const TownEditPanel::Context& context ) {	// 今の手直しで、今と同じ種類の出撃（ふつう・ゾンビの確認用）を作り直す。日の条件と車は、次の 1 回だけ今のままにする
		gReload.begin( context.record, context.today, context.escapeCarSpot );
		SceneManager::instance().requestRetryRaid();
		setStatus( "Reloading the raid with the edits...", false );
	}

	void finishApply( TownEditReload::Result reloaded ) {	// Apply の読み直しが済んだら、結果を知らせる
		if ( reloaded == TownEditReload::Result::Loaded ) {
			setStatus( "Reloaded the raid (same car spot and condition).", false );
		} else if ( reloaded == TownEditReload::Result::Replaced ) {
			setStatus( "The reload was replaced by another scene change.", true );
		}
	}

	void drawFileRow( const TownEditPanel::Context& context ) {
		if ( ImGui::Button( "Save" ) ) {
			save( *context.edits, context.file );
		}
		ImGui::SameLine();
		if ( ImGui::Button( "Revert" ) ) {
			revert( context.edits, context.file );
		}
		ImGui::SameLine();
		ImGui::BeginDisabled( gReload.isPending() || TownBakePanel::isRunning() || !context.record );
		if ( ImGui::Button( "Apply" ) ) {
			apply( context );
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::TextDisabled( "(%s)", TownEditFile::kPath );
	}

	void drawMessages( const TownEditPanel::Context& context ) {
		if ( !gStatus.empty() ) {
			if ( gStatusIsError ) {
				ImGui::TextColored( EditorColors::kWarning, "%s", gStatus.c_str() );
			} else {
				ImGui::TextDisabled( "%s", gStatus.c_str() );
			}
		}
		for ( const std::string& warning : context.town->map().editWarnings() ) {
			ImGui::TextColored( EditorColors::kWarning, "%s", warning.c_str() );
		}
		for ( const std::string& warning : context.file->warnings() ) {	// 起動時か Revert で、ファイルの読めなかった行
			ImGui::TextColored( EditorColors::kWarning, "%s", warning.c_str() );
		}
	}

	// ── Pick ──
	void pick( const TownEdits& edits, const FirstPersonCamera& camera ) {	// 飛んでいる自キャラの視点の正面の線の先の物を選ぶ。消した物は町に無いので選ばない
		std::vector< TownEditFrame::Target > targets;
		std::vector< Selection > owners;	// targets と同じ並びの、選んだときの Selection
		for ( int k = 0; k < kKindNumber; ++k ) {
			const TownEditKind kind = static_cast< TownEditKind >( k );
			for ( int i = 0; i < TownEditOriginals::count( kind ); ++i ) {
				const Selection selection = { true, kind, false, i };
				if ( const TownPlacement* at = currentOf( edits, selection ) ) {
					targets.push_back( { kind, *at } );
					owners.push_back( selection );
				}
			}
		}
		const std::vector< TownEdits::Addition >& additions = edits.additions();
		for ( int a = 0; a < static_cast< int >( additions.size() ); ++a ) {
			targets.push_back( { additions[ a ].kind, additions[ a ].at } );
			owners.push_back( { true, additions[ a ].kind, true, a } );
		}
		const int chosen = TownEditFrame::pick( camera.eyePosition(), camera.viewDirection(), targets );
		if ( chosen == TownEditFrame::kNoTarget ) {
			setStatus( "Nothing to pick.", false );
			return;
		}
		gSelection = owners[ chosen ];
		setStatus( "Picked " + selectionName( gSelection ), false );
	}

	void drawPickRow( const TownEditPanel::Context& context ) {
		ImGui::BeginDisabled( !context.flightCamera );
		if ( ImGui::Button( "Pick" ) ) {
			pick( *context.edits, *context.flightCamera );
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::TextDisabled( context.flightCamera ? "(object in front of you)" : "(Pick / Add need Debug > Fly)" );
	}

	// ── 選んだ物の欄 ──
	void drawPlacementFields( TownEdits* edits, const Selection& selection, const TownPlacement& current ) {	// 位置と向きの欄。動かしたら手直しへ書く
		TownPlacement to = current;
		float position[ kPositionAxes ] = { to.x, to.y, to.z };
		const bool moved = ImGui::DragFloat3( "Position", position, kPositionDragSpeed, kNoDragLimit, kNoDragLimit, kPositionFormat );
		const bool usesYaw = ( selection.kind != TownEditKind::ZombieSpawn );	// 湧く候補は向きを使わない
		ImGui::BeginDisabled( !usesYaw );
		const bool turned = ImGui::DragFloat( "Yaw", &to.yawDegree, kYawDragSpeed, kNoDragLimit, kNoDragLimit, kYawFormat );
		ImGui::EndDisabled();
		if ( !usesYaw ) {
			ImGui::SameLine();
			ImGui::TextDisabled( "(not used)" );
		}
		if ( !moved && !turned ) {
			return;
		}
		to.x = position[ 0 ];
		to.y = position[ 1 ];
		to.z = position[ 2 ];
		to.yawDegree = AngleMath::wrapSigned( to.yawDegree );
		if ( selection.added ) {
			edits->moveAdded( selection.index, to );
			return;
		}
		const TownEditKey key = TownEditOriginals::keyOf( selection.kind, selection.index );
		edits->revert( key );	// 同じ名札の行を 2 つ持たない（重なった行は、ファイルを読み直したときに警告になる）
		edits->move( key, to );
	}

	std::string limitReason( TownEditKind kind ) {	// Delete を押せない理由（消すと割る下限）
		const TownEditLimits limits = townEditLimits();
		char text[ kTextSize ];
		text[ 0 ] = '\0';
		switch ( kind ) {
			case TownEditKind::Dumpster:
				std::snprintf( text, sizeof( text ), "(keeps %d+ dumpsters and crates for fuel)", limits.minLootSpots );
				break;
			case TownEditKind::Crate:
				std::snprintf( text, sizeof( text ), "(keeps %d+ dumpsters and crates, %d+ crates for rifles)", limits.minLootSpots, limits.minCrates );
				break;
			case TownEditKind::ZombieSpawn:
				std::snprintf( text, sizeof( text ), "(keeps %d+ spawn points)", limits.minZombieSpawns );
				break;
			default:
				break;
		}
		return text;
	}

	void drawSelectedButtons( TownEdits* edits, const Selection& selection, bool exists ) {	// Reset・Delete。足した物は Delete だけ。取り消すと後ろの番号が 1 つ詰まるので、選ぶのもやめる
		if ( selection.added ) {
			if ( ImGui::Button( "Delete" ) ) {
				edits->removeAdded( selection.index );
				setStatus( "Removed " + selectionName( selection ), false );
				gSelection.active = false;
			}
			return;
		}
		const TownEditKey key = TownEditOriginals::keyOf( selection.kind, selection.index );
		ImGui::BeginDisabled( !isEdited( *edits, key ) );
		if ( ImGui::Button( "Reset" ) ) {
			edits->revert( key );
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if ( !townEditKindCanAddOrDelete( selection.kind ) ) {
			ImGui::TextDisabled( "(no Delete: the car spots keep their number)" );
			return;
		}
		const bool allowed = edits->canRemoveOneMore( selection.kind, TownEditOriginals::counts(), townEditLimits() );
		ImGui::BeginDisabled( !exists || !allowed );
		if ( ImGui::Button( "Delete" ) ) {
			edits->revert( key );	// 動かした行は消した物には当てられないので、一緒に消す
			edits->remove( key );
		}
		ImGui::EndDisabled();
		if ( exists && !allowed ) {
			ImGui::SameLine();
			ImGui::TextDisabled( "%s", limitReason( selection.kind ).c_str() );
		}
	}

	void drawSelected( TownEdits* edits ) {
		if ( !gSelection.active ) {
			ImGui::TextDisabled( "None (choose from the list below, or Pick)." );
			return;
		}
		const Selection selection = gSelection;
		ImGui::TextUnformatted( selectionName( selection ).c_str() );
		if ( !selection.added ) {
			const TownPlacement& original = TownEditOriginals::at( selection.kind, selection.index );
			ImGui::SameLine();
			ImGui::TextDisabled( "from %.2f, %.2f, %.2f  yaw %.1f", original.x, original.y, original.z, original.yawDegree );
		}
		const TownPlacement* current = currentOf( *edits, selection );
		if ( current ) {
			drawPlacementFields( edits, selection, *current );
		} else {
			ImGui::TextColored( kEditedColor, "Deleted (hidden now, Apply removes its collision. Reset brings it back)" );
		}
		drawSelectedButtons( edits, selection, current != nullptr );
	}

	// ── 種類ごとの一覧 ──
	void addAhead( TownEdits* edits, TownEditKind kind, const FirstPersonCamera& camera ) {
		const Vector3 at = TownEditFrame::addPoint( camera.eyePosition(), camera.viewDirection() );
		const int index = edits->add( kind, { at.x, at.y, at.z, kAddedYaw } );
		gSelection = { true, kind, true, index };
		setStatus( "Added " + selectionName( gSelection ) + " (Apply adds its collision and loot)", false );
	}

	void drawRow( const char* label, bool edited, const Selection& row ) {	// 一覧の 1 行。手直しした物は黄色。押したら選ぶ
		const bool selected = gSelection.active && gSelection.kind == row.kind && gSelection.added == row.added && gSelection.index == row.index;
		if ( edited ) {
			ImGui::PushStyleColor( ImGuiCol_Text, kEditedColor );
		}
		if ( ImGui::Selectable( label, selected ) ) {
			gSelection = row;
		}
		if ( edited ) {
			ImGui::PopStyleColor();
		}
	}

	void drawKindList( TownEdits* edits, TownEditKind kind, const FirstPersonCamera* flightCamera ) {
		ImGui::PushID( static_cast< int >( kind ) );
		const int count = TownEditOriginals::count( kind );
		const bool open = ImGui::TreeNode( "kind", "%s (%d)", townEditKindName( kind ), count );
		if ( townEditKindCanAddOrDelete( kind ) ) {
			ImGui::SameLine();
			ImGui::BeginDisabled( !flightCamera );
			if ( ImGui::SmallButton( "Add" ) ) {
				addAhead( edits, kind, *flightCamera );
			}
			ImGui::EndDisabled();
		}
		if ( open ) {
			char label[ kTextSize ];
			for ( int i = 0; i < count; ++i ) {
				const TownPlacement& original = TownEditOriginals::at( kind, i );
				const TownEditKey key = { kind, original.x, original.z };
				const bool removed = edits->isRemoved( key );
				std::snprintf( label, sizeof( label ), "#%d  %.2f, %.2f%s", i, original.x, original.z, removed ? "  (deleted)" : "" );
				drawRow( label, isEdited( *edits, key ), { true, kind, false, i } );
			}
			const std::vector< TownEdits::Addition >& additions = edits->additions();
			for ( int a = 0; a < static_cast< int >( additions.size() ); ++a ) {
				if ( additions[ a ].kind != kind ) {
					continue;
				}
				std::snprintf( label, sizeof( label ), "+%d  %.2f, %.2f", a, additions[ a ].at.x, additions[ a ].at.z );
				drawRow( label, true, { true, kind, true, a } );
			}
			ImGui::TreePop();
		}
		ImGui::PopID();
	}

	void updateHighlight( const TownEdits& edits ) {	// 選んでいる物の元の位置と今の位置を、枠を描く係のために写しておく
		dropMissingSelection( edits );
		gHasHighlight = gSelection.active;
		if ( !gHasHighlight ) {
			return;
		}
		gHighlight.kind = gSelection.kind;
		gHighlight.hasOriginal = !gSelection.added;
		if ( gHighlight.hasOriginal ) {
			gHighlight.original = TownEditOriginals::at( gSelection.kind, gSelection.index );
		}
		const TownPlacement* current = currentOf( edits, gSelection );
		gHighlight.hasCurrent = ( current != nullptr );
		if ( current ) {
			gHighlight.current = *current;
		}
	}
}

namespace TownEditPanel{

void update( const Context& context ) {
	if ( !context.record ) {
		return;
	}
	const bool baking = TownBakePanel::isRunning();	// 読み直しを頼んだのが Bake か Apply か（Bake の途中は Apply を押せない）
	const TownEditReload::Result reloaded = gReload.poll( *context.record, context.town != nullptr, context.escapeCarSpot );
	TownBakePanel::update( context, &gReload, reloaded );
	if ( !baking ) {
		finishApply( reloaded );
	}
}

void draw( const Context& context ) {
	if ( !context.edits || !context.file || !context.town ) {
		ImGui::TextDisabled( gReload.isPending() ? "Reloading the raid..." : "No raid: start a raid to edit the town." );
		return;
	}
	TownEdits* edits = context.edits;
	drawFileRow( context );
	drawMessages( context );
	TownBakePanel::draw( context, gReload );
	ImGui::SeparatorText( "Selected" );
	drawPickRow( context );
	dropMissingSelection( *edits );	// Revert で足した物が減ったときに、無い番号を読まないように
	drawSelected( edits );
	ImGui::SeparatorText( "Objects" );
	for ( int k = 0; k < kKindNumber; ++k ) {
		drawKindList( edits, static_cast< TownEditKind >( k ), context.flightCamera );
	}
	updateHighlight( *edits );
}

const Highlight* highlight() {
	return ( gHasHighlight && GameLib::DebugUi::isOpen() ) ? &gHighlight : nullptr;
}

} //namespace TownEditPanel

#else // ZF_EDITOR

namespace TownEditPanel{

void update( const Context& ) {
}

void draw( const Context& ) {
}

const Highlight* highlight() {
	return nullptr;
}

} //namespace TownEditPanel

#endif // ZF_EDITOR
