#ifndef INCLUDED_TOWN_EDIT_FILE_STATE_H
#define INCLUDED_TOWN_EDIT_FILE_STATE_H

#include <string>
#include <vector>

class TownEdits;

// 起動中の町の手直し（TownEdits）と、TownEdit.xml の中身の間の様子
// 最後にファイルを読んだ・書いたときの手直しの revision と、そのとき読めなかった行の理由を覚える
//   読めなかった行は手直しに入っていない。今の手直しをそのまま書くと、その行がファイルから消える
//   revision が同じなら、手直しはファイルの中身のまま（書き直す物が無い）
// F6 の Town タブの Bake は、これで先に TownEdit.xml を保存するかを決める（手で書いたファイルを黙って書き換えないため）
// 起動から終了まで 1 つ（GameResources が TownEdits の隣に持つ）。起動時の読み込みと、F6 の Save・Revert・Bake が更新する
class TownEditFileState{
public:
	// Bake が床と表を書く前に、TownEdit.xml をどうするか
	enum class BakeSave{
		Keep,	// 書き直さない（ファイルが手直しのまま・手直しもファイルも無い）
		Save,	// 先に保存する（次の起動が同じ町を組み立てて、指紋が合うように）
		Refuse,	// 読めなかった行があるので保存せず、Bake を止める。ファイルを直すか、Save で今の手直しを書くかは使う人が決める
	};

	TownEditFileState();

	void loaded( const TownEdits& edits, const std::vector< std::string >& warnings );	// ファイルを読んだ後（起動時・Revert）に呼ぶ。warnings: TownEditFile::load が足した理由。空なら全部読めた
	void saved( const TownEdits& edits );												// ファイルへ書いた後に呼ぶ。ファイルが手直しと同じになるので、読めなかった行の理由も消す
	const std::vector< std::string >& warnings() const;									// 最後に読んだときの、読めなかった行の理由（書いた後は空）。F6 の Town タブに黄色で出す
	BakeSave bakeSave( const TownEdits& edits, bool fileExists ) const;					// Bake が先に保存するか。fileExists: TownEdit.xml があるか

private:
	std::vector< std::string > mWarnings;
	unsigned mRevision;	// 最後に読んだ・書いたときの TownEdits::revision
};

#endif
