#ifndef INCLUDED_GAME_CONFIG_H
#define INCLUDED_GAME_CONFIG_H

// ────────────────────────────────────────────────
// GameConfig：ゲームプレイ設定（入力バインドとは別系統）の「唯一の正」。
//
// ・現在の項目：landOnContact（スイング中に接地したら自動離脱するか）
// ・save/load でディスクへ永続化する。人が読めるテキストで書き、
//   ファイル欠如や失敗ではゲームを止めない（既定値で続行する）。
//
// 入力設定（InputConfig）とはファイルも責務も分ける。設定ファイル名は .cpp 側で
// 一元管理し、ゲームロジックはこのクラスのフィールドだけを触る。
// ────────────────────────────────────────────────
class GameConfig
{
public:
	GameConfig();

	bool landOnContact() const;        // true=着地（接地で離脱）／false=継続。
	void setLandOnContact( bool land );

	void save() const; // 既定パスへ保存する。
	bool load();       // 既定パスから読み込む。無ければ false（既定値のまま）。

private:
	bool mLandOnContact;
};

#endif
