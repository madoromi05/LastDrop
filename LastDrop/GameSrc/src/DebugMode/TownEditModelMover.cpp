#include "GameSrc/DebugMode/TownEditModelMover.h"
#include "GameSrc/DebugMode/TownEditOriginals.h"
#include "GameSrc/Scene/Game/World/GameWorld.h"
#include "GameSrc/Scene/Game/World/TownEnvironment.h"
#include "GameSrc/Map/Building.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/World/GameObjectWorld.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/Component/StaticModelComponent.h"
#include "GameSrc/Component/LightComponent.h"
#include "GameSrc/Math/AngleMath.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Vector3.h"

using namespace GameLib::Math;

namespace{
	const int kNoIndex = -1;
	const unsigned kNotAdded = 0;		// 元の物の Binding::additionId（読まない）
	const float kSameSpot = 0.05f;		// 手直しの位置と町の物を同じ物とみなす距離（m。手直しの名札と同じ）
	const int kKindNumber = static_cast< int >( TownEditKind::Count );

	// 読み込んだ町の、見た目を動かせる物 1 つ
	struct Candidate{
		TownEditKind group;		// groupOf でまとめた種類
		int buildingIndex;		// 建物なら TownEnvironment::buildings の添字。置物なら kNoIndex
		GameObject* prop;		// 置物なら物。建物なら nullptr
		Vector3 position;
		bool taken;				// もう手直しと結んだか
	};

	TownEditKind groupOf( TownEditKind kind ) {	// 町の物から見分けられる種類。建物と囲いの建物、ゴミ箱と木箱は、物の種類（タグ）では分けられないので同じ組にする
		switch ( kind ) {
			case TownEditKind::PerimeterBuilding:	return TownEditKind::Building;
			case TownEditKind::Crate:				return TownEditKind::Dumpster;
			default:								return kind;
		}
	}

	bool groupOfTag( GameObjectTag tag, TownEditKind* group ) {	// 置物の組を group へ書く。手直しできない物（自キャラ・ゾンビ・落ちている物など）なら false
		switch ( tag ) {
			case GameObjectTag::LootContainer:	*group = TownEditKind::Dumpster;	return true;
			case GameObjectTag::FireHydrant:	*group = TownEditKind::FireHydrant;	return true;
			case GameObjectTag::StreetLamp:		*group = TownEditKind::StreetLight;	return true;
			case GameObjectTag::EscapeCar:		*group = TownEditKind::EscapeCar;	return true;
			default:							return false;
		}
	}

	bool hasModel( TownEditKind kind ) {	// 町に見た目がある種類か（湧く候補と自キャラの始めの位置は、置き場所だけで見た目が無い）
		return kind != TownEditKind::ZombieSpawn && kind != TownEditKind::PlayerStart;
	}

	std::vector< Candidate > collectCandidates( GameWorld* world ) {
		std::vector< Candidate > candidates;
		const std::vector< Building >& buildings = world->town().buildings();
		for ( int i = 0; i < static_cast< int >( buildings.size() ); ++i ) {
			candidates.push_back( { TownEditKind::Building, i, nullptr, buildings[ i ].position(), false } );
		}
		world->objects().forEach< StaticModelComponent >( [ &candidates ]( StaticModelComponent& model ) {
			GameObject* object = model.gameObject();
			TownEditKind group;
			if ( groupOfTag( object->tag(), &group ) ) {
				candidates.push_back( { group, kNoIndex, object, model.mesh().position(), false } );
			}
		} );
		return candidates;
	}

	bool isNear( float x, float z, const TownPlacement& at ) {
		const float dx = x - at.x;
		const float dz = z - at.z;
		return dx * dx + dz * dz <= kSameSpot * kSameSpot;
	}

	const Candidate* takeNear( std::vector< Candidate >* candidates, TownEditKind kind, const TownPlacement& at ) {	// at に置かれた kind の物のうち、まだ結んでいない物を結んだ印にして返す。無ければ nullptr
		const TownEditKind group = groupOf( kind );
		for ( Candidate& candidate : *candidates ) {
			if ( !candidate.taken && candidate.group == group && isNear( candidate.position.x, candidate.position.z, at ) ) {
				candidate.taken = true;
				return &candidate;
			}
		}
		return nullptr;
	}

	bool isSame( const TownPlacement& a, const TownPlacement& b ) {
		return a.x == b.x && a.y == b.y && a.z == b.z && a.yawDegree == b.yawDegree;
	}

	const TownEdits::Addition* findAddition( const TownEdits& edits, unsigned id ) {	// 番号 id の足した物。取り消した・Revert で振り直したなら nullptr
		for ( const TownEdits::Addition& addition : edits.additions() ) {
			if ( addition.id == id ) {
				return &addition;
			}
		}
		return nullptr;
	}

	void moveProp( GameObject* prop, const TownPlacement& to ) {	// 置物の置き場所を書き換えて、見た目を写し直す
		TransformComponent* transform = prop->getComponent< TransformComponent >();
		StaticModelComponent* model = prop->getComponent< StaticModelComponent >();
		if ( !transform || !model ) {
			return;
		}
		transform->position() = to.position();	// 街灯の光（LightComponent）も、この置き場所から出る
		transform->angleY() = AngleMath::wrapSigned( to.yawDegree );
		model->followTransform();
	}
}

TownEditModelMover::TownEditModelMover() :
	mBound( false ),
	mRevision( 0 ) {
}

void TownEditModelMover::follow( const TownEdits& edits, GameWorld* world ) {
	ASSERT( world && "TownEditModelMover::follow : world is null." );
	if ( !mBound ) {
		bind( edits, world );
		mBound = true;
		mRevision = edits.revision();
		return;
	}
	if ( edits.revision() == mRevision ) {
		return;
	}
	mRevision = edits.revision();
	adoptReloadedAdditions( edits );
	for ( Binding& binding : mBindings ) {
		showBinding( &binding, wantedAt( edits, binding ), world );
	}
	syncPreviews( edits, world );
}

void TownEditModelMover::bind( const TownEdits& edits, GameWorld* world ) {
	std::vector< Candidate > candidates = collectCandidates( world );
	auto bindOne = [ this, &candidates ]( TownEditKind kind, int originalIndex, unsigned additionId, const TownPlacement& at ) {
		const Candidate* candidate = takeNear( &candidates, kind, at );
		if ( !candidate ) {
			return;	// 読み込んだ町に無い（手直しの行が使われなかった・形を作れず置けなかった）。その物は枠だけ動く
		}
		const bool kNotHidden = false;
		const bool kLightUnread = false;	// lightWasOn は見えなくするときに読む
		mBindings.push_back( { kind, originalIndex, additionId, candidate->buildingIndex, GameObjectHandle( candidate->prop ), at, at, kNotHidden, kLightUnread } );
	};
	for ( int k = 0; k < kKindNumber; ++k ) {
		const TownEditKind kind = static_cast< TownEditKind >( k );
		if ( !hasModel( kind ) ) {
			continue;
		}
		for ( int i = 0; i < TownEditOriginals::count( kind ); ++i ) {
			if ( const TownPlacement* placed = TownEditOriginals::placedAt( edits, kind, i ) ) {	// 消した物は町に無い
				bindOne( kind, i, kNotAdded, *placed );
			}
		}
	}
	for ( const TownEdits::Addition& addition : edits.additions() ) {
		if ( hasModel( addition.kind ) ) {
			bindOne( addition.kind, kNoIndex, addition.id, addition.at );
		}
	}
}

void TownEditModelMover::adoptReloadedAdditions( const TownEdits& edits ) {
	// Revert はファイルを読み直して足した物に新しい番号を振るので、読み込んだ町の足した物と番号が合わなくなる
	// 番号を失った町の物に、同じ種類で読み込んだときの位置にある足した物が来たら、その番号へ結び直す（見せる物を二重に置かないため）
	for ( const TownEdits::Addition& addition : edits.additions() ) {
		if ( isBoundAddition( addition.id ) || isPreviewed( addition.id ) ) {
			continue;
		}
		for ( Binding& binding : mBindings ) {
			const bool lostId = ( binding.originalIndex == kNoIndex && !findAddition( edits, binding.additionId ) );
			if ( lostId && binding.kind == addition.kind && isNear( binding.loaded.x, binding.loaded.z, addition.at ) ) {
				binding.additionId = addition.id;
				break;
			}
		}
	}
}

const TownPlacement* TownEditModelMover::wantedAt( const TownEdits& edits, const Binding& binding ) const {
	if ( binding.originalIndex != kNoIndex ) {
		return TownEditOriginals::placedAt( edits, binding.kind, binding.originalIndex );
	}
	const TownEdits::Addition* addition = findAddition( edits, binding.additionId );
	return addition ? &addition->at : nullptr;
}

void TownEditModelMover::showBinding( Binding* binding, const TownPlacement* to, GameWorld* world ) const {
	setHidden( binding, to == nullptr, world );
	if ( !to || isSame( *to, binding->shown ) ) {
		return;	// 消した物は、見えなくする前の位置のまま残す（Reset で戻したときに、そこから動かす）
	}
	if ( binding->buildingIndex != kNoIndex ) {
		world->town().moveBuilding( binding->buildingIndex, to->position(), AngleMath::wrapSigned( to->yawDegree ) );
	} else if ( GameObject* prop = binding->prop.get() ) {
		moveProp( prop, *to );
	}
	binding->shown = *to;
}

void TownEditModelMover::setHidden( Binding* binding, bool hidden, GameWorld* world ) const {
	if ( binding->hidden == hidden ) {
		return;
	}
	binding->hidden = hidden;
	if ( binding->buildingIndex != kNoIndex ) {
		world->town().setBuildingHidden( binding->buildingIndex, hidden );
		return;
	}
	GameObject* prop = binding->prop.get();
	if ( !prop ) {
		return;
	}
	if ( StaticModelComponent* model = prop->getComponent< StaticModelComponent >() ) {
		model->setHidden( hidden );
	}
	if ( LightComponent* light = prop->getComponent< LightComponent >() ) {	// 外せる部品（街灯だけが持つ）
		if ( hidden ) {
			binding->lightWasOn = light->isOn();
			light->setOn( false );
		} else {
			light->setOn( binding->lightWasOn );
		}
	}
}

void TownEditModelMover::syncPreviews( const TownEdits& edits, GameWorld* world ) {
	// 取り消した足した物の見せる物を消し、残っている物は今の位置へ動かす
	for ( size_t i = mPreviews.size(); i > 0; --i ) {
		Preview& preview = mPreviews[ i - 1 ];
		const TownEdits::Addition* addition = findAddition( edits, preview.additionId );
		GameObject* object = preview.object.get();
		if ( !addition ) {
			if ( object ) {
				world->removeTownEditPreview( object );
			}
			mPreviews.erase( mPreviews.begin() + ( i - 1 ) );
			continue;
		}
		if ( object && !isSame( addition->at, preview.shown ) ) {
			moveProp( object, addition->at );
		}
		preview.shown = addition->at;
	}
	// 新しく足した物を置く。置けなかった物も並びに残す（手直しが変わるたびに置き直そうとしないため）
	for ( const TownEdits::Addition& addition : edits.additions() ) {
		if ( !hasModel( addition.kind ) || isBoundAddition( addition.id ) || isPreviewed( addition.id ) ) {
			continue;
		}
		GameObject* object = world->spawnTownEditPreview( addition.kind, addition.at );
		mPreviews.push_back( { addition.kind, addition.id, GameObjectHandle( object ), addition.at } );
	}
}

bool TownEditModelMover::isBoundAddition( unsigned additionId ) const {
	for ( const Binding& binding : mBindings ) {
		if ( binding.originalIndex == kNoIndex && binding.additionId == additionId ) {
			return true;
		}
	}
	return false;
}

bool TownEditModelMover::isPreviewed( unsigned additionId ) const {
	for ( const Preview& preview : mPreviews ) {
		if ( preview.additionId == additionId ) {
			return true;
		}
	}
	return false;
}
