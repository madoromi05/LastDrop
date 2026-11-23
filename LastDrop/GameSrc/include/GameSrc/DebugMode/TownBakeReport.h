#ifndef INCLUDED_TOWN_BAKE_REPORT_H
#define INCLUDED_TOWN_BAKE_REPORT_H

#include <string>
#include <vector>

// F6 の Town タブの Bake・Bake all が、押してから終わるまでにしたこと（TownEdit.xml を保存したか・書いた床と表のファイル）を覚え、知らせの文にする
// Bake all は車の置き場所ごとに出撃を読み直してから書くので、終わったときの文に、途中で書いた物も全部残す
// Debug・Profile ビルドだけ（Release では中身をビルドしない）
class TownBakeReport{
public:
	TownBakeReport();
	void clear();								// Bake・Bake all を押したときに呼ぶ
	void recordSave();							// TownEdit.xml を保存した
	void recordSpot( int escapeCarSpot );		// その車の置き場所の床と、隠れの表を書いた
	std::string text( const char* head ) const;	// head の後に、したことを 1 行ずつ並べた文。保存したか・書いた床（書いた順）・表（何回書いても 1 行）

private:
	bool mSaved;				// TownEdit.xml を保存したか
	std::vector< int > mSpots;	// 床を書いた車の置き場所（書いた順）
};

#endif
