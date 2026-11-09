#ifndef INCLUDED_TUNING_VISITORS_H
#define INCLUDED_TUNING_VISITORS_H

// 調整値の構造体（WeaponPoseParams など）の visit を受けて、エディタで扱う項目の並びにする（Debug・Profile ビルドだけ）。
// 計画書は SummaryPlan/個別仕様/最適化と計測/DebugTools_Profiling_Plan.txt の 5-5。
//
// 構造体ごとの visit は v( 名前, 値, 下限, 上限, 単位 ) を項目の数だけ呼ぶ。ここではそれを 1 つの係（TuningCollector）で受けて
// 「値の場所と範囲」の並びにし、描く・ファイルへ書く・読む・既定値と比べる・C++ にする は、その並びに対する関数で行う。
// 5 つの係を別々に作らないのは、どれも「項目を順に見る」だけで、並びにしておけば visit を 1 回呼ぶだけで済むため。
// 項目を足すときに直すのは構造体の visit だけ（ここは触らない）。
#ifdef ZF_EDITOR

#include "GameLib/Math/Vector3.h"

#include <string>
#include <vector>

// 項目 1 つ。値は構造体のメンバを直接指す（指す先の構造体より長く持たない。使うたびに集め直す）。
struct TuningItem{
	const char* name;   // メンバの名前（ファイルのキー・C++ のコピーにも使う）
	float* values;      // 値の先頭（float は 1 つ、Vector3 は x, y, z の 3 つ）
	int count;          // 値の数（1 か 3）
	float minValue;     // エディタで動かせる範囲
	float maxValue;
	const char* unit;   // 単位（表示と C++ のコメントに使う）
};

// visit の v として渡す係。呼ばれた順に項目をためる。
class TuningCollector{
public:
	void operator()( const char* name, float& value, float minValue, float maxValue, const char* unit );
	void operator()( const char* name, GameLib::Math::Vector3& value, float minValue, float maxValue, const char* unit );
	const std::vector< TuningItem >& items() const;
private:
	std::vector< TuningItem > mItems;
};

namespace TuningVisitors{

// 今の値と既定値が違うか（同じ構造体の同じ項目どうしで呼ぶ）。
bool differs( const TuningItem& live, const TuningItem& defaults );
// from の値を to へ写す（既定値へ戻すときに使う）。
void copyValues( const TuningItem& from, const TuningItem& to );
// 1 項目を ImGui で描く。既定値と違えば字の色を変え、横に既定値へ戻すボタンを出す。値を変えたら true。
bool draw( const TuningItem& live, const TuningItem& defaults );
// ファイルに書く値の文字列（"1.5" や "-0.12, -0.1, 0.28"）。元の値へ戻る桁（%.9g）で書く。
std::string fileText( const TuningItem& item );
// 構造体の初期値の形の 1 行（"float carryCant = 15.f;  // deg"）。
std::string cppLine( const TuningItem& item );

} //namespace TuningVisitors

#endif // ZF_EDITOR

#endif
