#ifndef INCLUDED_MAGAZINE_SYNC_H
#define INCLUDED_MAGAZINE_SYNC_H

#include "GameSrc/Interaction/Item/ItemCatalog.h"

class Inventory;
class GameWorld;
class GameObjectWorld;

// カバンのマスに出す銃の弾倉数と、世界の銃の物が持つ本当の弾倉数を同期する
// 町の中では銃の物が本当の数を持ち、カバンのマスはその写し
// カバン/漁りの画面を開くとき store、閉じるとき・出撃のロード直後に restore を呼ぶ
// 銃の物は種類で引く（GunComponent::find。銃は 1 種類 1 丁なので、種類で引ける）

int magazineRoundsOf( const GameObjectWorld& objects, ItemId item );		// 世界の銃の物 item（ITEM_KIND_WEAPON）の弾倉に入っている弾の数（その銃が無ければ 0）
void storeMagazineRounds( Inventory& inventory, const GameWorld& world );	// 世界の銃の物をカバンの対応するマスへ書き写す（画面を開く直前に）
void restoreMagazineRounds( Inventory& inventory, GameWorld& world );		// カバンのマスの弾倉数を、世界の銃の物へ入れる（画面を閉じたとき・銃の物を作った直後に。弾倉の大きさまでに収める）

#endif
