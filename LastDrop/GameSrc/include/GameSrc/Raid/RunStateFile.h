#ifndef INCLUDED_RUN_STATE_FILE_H
#define INCLUDED_RUN_STATE_FILE_H

class RunState;

// 5 日間の様子（RunState）を save.xml へ書く・読む係（SummaryPlan/個別仕様/MidPrioritySplit_Plan.txt 2 章。2026-11-09 RunState から分けた）。
// 保存の書き方は GameSettings と同じ（PseudoXml を文字列にして fstream）。形は R2_Plan.txt 7 章（形を変えたら RunStateFile.cpp の版を上げる）。
// RunState の公開の口だけを読み書きする（中身の並べ方は知らない）。置き場所は起動した場所（settings.xml と同じ）。
class RunStateFile{
public:
	// save.xml から state へ読む。ファイルが無い・壊れているときは false を返し、中身は変えない（呼ぶ側が 1 日目で始める）。
	// 読めない物の名前・マスからはみ出す物は読み飛ばす（ファイルを手で直しても止まらない）。state: nullptr 不可。
	static bool load( RunState* state );
	// state を save.xml へ書く（save.xml.tmp に書いてから入れ替える。書く途中で落ちても前のファイルが残る）。
	// 書けなければ false（遊ぶのは続けられる）。
	static bool save( const RunState& state );
	// save.xml を消す（F6 の Raid 欄。次の load は false になり、1 日目から）。
	static void erase();
};

#endif
