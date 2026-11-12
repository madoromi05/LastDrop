#ifndef INCLUDED_TOWN_VISIBILITY_H
#define INCLUDED_TOWN_VISIBILITY_H

#include "GameSrc/Hit/GroundGrid.h"
#include "GameLib/Math/Vector3.h"
#include <cstdint>
#include <vector>

class Building;

// 簡易 PVS：町を上から見た 4m のますごとに「そこから見えうる建物」と「見えうるます」を、読み込みの時に前もって作って持つ
// （SummaryPlan/個別仕様/最適化と計測/OptimizationC_Plan.txt 7 章）。実行中は表を引くだけ
// 建物が動かず、町が碁盤の目で平らなので、上から見た 2D の線（建物の足元の長方形が遮るか）で決める
//   見る点  ：ますの四隅（目がますのどこにいても、どれかの点から見えた物は描く。見えた物を消さない側に倒す）
//   見られる：建物の足元の長方形の、見る点を向いた辺の上に 1m ごとの点（ゾンビ用のますどうしは、ますの四隅どうし）
//   遮る物  ：ほかの建物の、体の高さの壁（MeshWallBuilder）を囲む長方形を少し縮めた物。見た目を囲む箱は足元の板（11.16m 四方）まで
//            囲んでいて壁より何 m も大きく、それで遮ると壁の外を通る線まで消した（Debug の確かめ）。遮る建物が見られる物より低ければ遮らない
//            （低い建物の上から奥の高い建物が見えるため）
// 描くかを決めるだけに使い、当たり判定やゲームの進行には使わない（ViewFrustum・DrawCulling と同じ約束）。影のパスにも使わない
// （見えない建物の影が、見える所に落ちるため）
class TownVisibility{
public:
	// 目がどのますにも当たらない（町の外・建物の足元・建物より高い所。F6 の観察用カメラなど）とき viewCell が返す値
	static const int kNoCell = -1;

	TownVisibility();
	TownVisibility( const TownVisibility& ) = delete;
	TownVisibility& operator=( const TownVisibility& ) = delete;

	// 建物の並び（GameWorld::buildings。番号は並びの順）から表を作る（前の表は捨てる）。読み込みの時に 1 回。建物が 0 軒なら作らない
	void build( const std::vector< Building >& buildings );
	bool isReady() const;

	// 目の位置 eye が入っているます（kNoCell なら PVS を使わない＝全部描く）
	int cellAt( const GameLib::Math::Vector3& eye ) const;
	// viewCell から building 番の建物が見えうるか。viewCell が kNoCell なら true
	bool showsBuilding( int viewCell, int building ) const;
	// viewCell から、足元 position にいる物（ゾンビ）が見えうるか。position のますと、その周り 8 ますのどれかが見えうれば true
	// （体がますの境をまたいで見える分）。viewCell が kNoCell・position が町の外なら true
	bool showsPoint( int viewCell, const GameLib::Math::Vector3& position ) const;
	// ますの範囲（上から見た四角。y は 0）。確認用の表示（F3）に使う
	void cellBounds( int cell, GameLib::Math::Vector3* boundsMin, GameLib::Math::Vector3* boundsMax ) const;
	// ますの数（町を覆う 4m のます。表が無ければ 0）・cell が目の来うるます（建物の足元でない）か。確かめ（TownVisibilityCheck）が使う
	int cellCount() const;
	bool isOpenCell( int cell ) const;

	// 3D の線での確かめは DebugMode/TownVisibilityCheck

private:
	// 上から見た建物 1 軒
	struct Footprint{
		float minX, minZ, maxX, maxZ; // 見られる長方形（建物の見た目を囲む箱。足元の板まで含むので壁より大きい）
		float height;                 // 一番高い所（y）
		float blockMinX, blockMinZ, blockMaxX, blockMaxZ; // 遮る長方形（体の高さの壁を囲む箱を少し縮めた物）
	};

	// ── 表を作る段（build から、この順に呼ぶ。後の段は前の段の結果を読む）──
	// 建物の足元と遮る長方形を作り、町を囲む範囲を townMin / townMax へ書く。目を使ってよい高さもここで決める
	void buildFootprints( const std::vector< Building >& buildings, GameLib::Math::Vector3* townMin, GameLib::Math::Vector3* townMax );
	// 町をますに分け、目が来うるます（真ん中が建物の壁の長方形に入らない）に印を付ける
	void markOpenCells( const GameLib::Math::Vector3& townMin, const GameLib::Math::Vector3& townMax );
	// ますごとの見えうる建物（ますの見る点のどれかから見えうる建物）
	void bakeBuildingVisibility();
	// ますごとの見えうるます（ゾンビ用。四隅のどれかどうしが見通せれば見えうる）
	void bakeCellVisibility();

	// 2D の線分 (ax,az)→(bx,bz) を、高さ minHeight 以上の建物（ignore 番を除く）が遮るか
	bool isBlocked( float ax, float az, float bx, float bz, float minHeight, int ignore ) const;
	// 点 (x,z) から building 番の建物が見えうるか（建物の見る点を向いた辺の上の点のどれかへ、線が遮られずに届くか）
	bool seesBuilding( float x, float z, int building ) const;
	int cellIndex( int cellX, int cellZ ) const; // ますの番号（範囲の外なら kNoCell）
	bool hasBit( const std::vector< std::uint32_t >& bits, int row, int rowWords, int column ) const;
	void setBit( std::vector< std::uint32_t >* bits, int row, int rowWords, int column );

	std::vector< Footprint > mFootprints;
	GroundGrid mOccluders;       // 遮る長方形を入れたます（線分が通る近くの建物だけ調べる）
	mutable std::vector< int > mCandidates; // isBlocked の作業領域
	float mMinX;
	float mMinZ;
	int mCountX;
	int mCountZ;
	float mMaxEyeHeight;         // これより高い目では使わない（一番低い建物の高さ）
	std::vector< bool > mCellOpen;               // ますごと：目が来うるか（真ん中が建物の壁の長方形に入らない）
	int mBuildingWords;                          // 1 ますぶんの建物のビットの数（32 ビット単位）
	std::vector< std::uint32_t > mBuildingBits;  // ますごとの見えうる建物
	int mCellWords;                              // 1 ますぶんのますのビットの数
	std::vector< std::uint32_t > mCellBits;      // ますごとの見えうるます
};

// 目から見る絵の建物・ゾンビを PVS で間引くか（F6 → Render の PVS。見比べと計測用）。起動中ずっと 1 つ（初期値 true）
bool& pvsEnabled();

#endif
