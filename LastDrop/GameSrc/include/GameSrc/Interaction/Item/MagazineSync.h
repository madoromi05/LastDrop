#ifndef INCLUDED_MAGAZINE_SYNC_H
#define INCLUDED_MAGAZINE_SYNC_H

class Inventory;
class GameWorld;

// カバンのマスに出す銃の弾倉数と、世界の銃の物が持つ本当の弾倉数を同期する。
// 町の中では銃の物が本当の数を持ち、カバンのマスはその写し（InventoryPickup_Plan.txt Q2・R2_Plan.txt Q13）。
// カバン/漁りの画面を開くとき store、閉じるとき・出撃のロード直後に restore を呼ぶ。

// 世界の銃の弾倉数を、カバンの対応するマスへ書き写す（画面を開く直前に）。
void storeMagazineRounds( Inventory& inventory, const GameWorld& world );
// カバンのマスの弾倉数を、世界の銃の物へ入れる（画面を閉じたとき・銃の物を作った直後に）。
void restoreMagazineRounds( Inventory& inventory, GameWorld& world );

#endif
