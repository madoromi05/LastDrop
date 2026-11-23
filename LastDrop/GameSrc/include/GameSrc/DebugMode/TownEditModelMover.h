#ifndef INCLUDED_TOWN_EDIT_MODEL_MOVER_H
#define INCLUDED_TOWN_EDIT_MODEL_MOVER_H

#include "GameSrc/Map/TownEdits.h"
#include "GameSrc/World/GameObjectHandle.h"
#include <vector>

class GameWorld;

// F6 の Town タブの手直し（動かす・消す・足す）を、読み込んだ出撃の町の見た目へその場で写す係（確かめ用）
// 動かす：建物・町を囲む建物・ゴミ箱・木箱・消火栓・街灯（光は柱の置き場所から出るので一緒に動く）・脱出の車
// 消す：物は残して見えなくする（描かない・弾と視線が通る・見て使えない・街灯の光も消える）。Reset で戻すと中身もそのまま
// 足す：建物・ゴミ箱・木箱・消火栓・街灯を、手直しの位置に見せる物として置く（GameWorld::spawnTownEditPreview。箱は漁れない）
//   建物も建物の並びには入れず、置物と同じ物として置く（並びを増やすと、弾の名簿と隠れの表の番号が古い場所を指すため）
// 写さない物：移動の当たり判定・ゾンビの歩ける床・隠れの表。どれも Apply で出撃を読み直したときに合う
//   消した物の場所には見えない壁が残り、動かした物・足した物はすり抜ける
// 町の物と手直しの対応は、最初の follow で 1 回だけ決める。その時の手直しは、町を組み立てた手直しと同じため
//   （手直しを変えられるのは F6 の Town タブだけで、出撃を読み終えるまでは欄を出さない）
// GameEditorBridge が持ち、出撃の世界と同じだけ生きる（読み直した別の世界の物を指さないため）。手直しは変えない
class TownEditModelMover{
public:
	TownEditModelMover();

	// 手直しが前の呼び出しから変わっていれば、町の物の見た目を今の手直しに合わせる（変わっていなければ何もしない）
	// world: 読み終えた出撃の世界。いつも同じ世界であること。借りるだけ。update の外で呼ぶ（見せる物を置く・消すため）
	void follow( const TownEdits& edits, GameWorld* world );

private:
	// 読み込んだ町の物 1 つと、それを指す手直し
	struct Binding{
		TownEditKind kind;
		int originalIndex;			// 元の並びの番号（TownEditOriginals）。足した物は kNoIndex
		unsigned additionId;		// 足した物の番号（TownEdits::Addition::id）。元の物では読まない
		int buildingIndex;			// 建物なら TownEnvironment::buildings の添字。置物なら kNoIndex
		GameObjectHandle prop;		// 置物の物。建物なら何も指さない
		TownPlacement loaded;		// 町を組み立てたときの位置
		TownPlacement shown;		// 今見えている位置
		bool hidden;				// 消したので見えなくしているか
		bool lightWasOn;			// 見えなくする前に光が付いていたか（光を持つ物だけ。hidden の間だけ読む）
	};
	// 読み込んだ後に足した物を見せるために置いた物 1 つ
	struct Preview{
		TownEditKind kind;
		unsigned additionId;		// 見せている足した物の番号
		GameObjectHandle object;	// 置いた物。形の構築に失敗して置けなかったなら何も指さない
		TownPlacement shown;
	};

	void bind( const TownEdits& edits, GameWorld* world );												// 町の物と手直しの対応を決める
	void adoptReloadedAdditions( const TownEdits& edits );												// Revert で番号の振り直された足した物を、同じ場所の町の物へ結び直す
	const TownPlacement* wantedAt( const TownEdits& edits, const Binding& binding ) const;				// 今の手直しで binding の物を見せる位置。消した物は nullptr
	void showBinding( Binding* binding, const TownPlacement* to, GameWorld* world ) const;				// binding の物を to へ動かす。to が nullptr なら見えなくする
	void setHidden( Binding* binding, bool hidden, GameWorld* world ) const;
	void syncPreviews( const TownEdits& edits, GameWorld* world );										// 見せる物を、今の足した物に合わせて置く・動かす・消す
	bool isBoundAddition( unsigned additionId ) const;													// 読み込んだ町の物に結んだ足した物か
	bool isPreviewed( unsigned additionId ) const;														// 見せる物を置いた足した物か

	std::vector< Binding > mBindings;
	std::vector< Preview > mPreviews;
	bool mBound;			// bind を済ませたか
	unsigned mRevision;		// 最後に写した手直しの TownEdits::revision（mBound の後だけ読む）
};

#endif
