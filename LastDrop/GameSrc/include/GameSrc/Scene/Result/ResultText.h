#ifndef INCLUDED_RESULT_TEXT_H
#define INCLUDED_RESULT_TEXT_H

#include <string>

// 結果の画面に書く数字の書き方。脱出成功とゲームオーバーの両方が同じ書き方を使うので、1 か所に置く。ResultScreen_Plan.txt
namespace ResultText{
	std::string time( float seconds );	// 経った秒を「分:秒」（14:07）にする。秒の端数は切り捨て（0:59.9 は 0:59）。seconds: 0 以上
}

#endif
