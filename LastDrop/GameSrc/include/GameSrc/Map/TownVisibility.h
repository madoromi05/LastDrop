#ifndef INCLUDED_TOWN_VISIBILITY_H
#define INCLUDED_TOWN_VISIBILITY_H

#include "GameSrc/Hit/GroundGrid.h"
#include "GameLib/Math/Vector3.h"
#include <cstdint>
#include <vector>

class Building;

// 簡易 PVS：町を上から見た 4m のますごとに「そこから見えうる建物」と「見えうるます」を、読み込みの時に前もって作って持つ
// 実行中は表を引くだけ
// 作った表はバイト列にして書き出せ、次からはそれを読める（saveTables・loadTables。どのファイルを読むかは TownCache）
// 建物が動かず、町が碁盤の目で平らなので、上から見た 2D の線（建物の足元の長方形が遮るか）で決める
// 足元の長方形は建物の向きに回した物。建物は好きな角度に回せるので、東西南北にそろった箱で囲むと斜めの建物では壁からはみ出し、
// 見えるはずの物まで遮ってしまうため
//   見る点  ：ますの四隅（目がますのどこにいても、どれかの点から見えた物は描く。見えた物を消さない側に倒す）
//   見られる：建物の足元の長方形の、見る点を向いた辺の上に 1m ごとの点（ゾンビ用のますどうしは、ますの四隅どうし）
//   遮る物  ：ほかの建物の、体の高さの壁（MeshWallBuilder）を囲む長方形を少し縮めた物。見た目を囲む箱は壁より外まで囲んでいるので、
//            それで遮ると壁の外を通る線まで消してしまう。遮る建物が見られる物より低ければ遮らない
//            （低い建物の上から奥の高い建物が見えるため）
// 描くかを決めるだけに使い、当たり判定やゲームの進行には使わない（ViewFrustum・DrawCulling と同じ約束）。影のパスにも使わない
// （見えない建物の影が、見える所に落ちるため）
class TownVisibility{
public:
	static const int kNoCell = -1;	// 目がどのますにも当たらない（町の外・建物の足元・建物より高い所。F6 の Fly で飛んでいる自キャラなど）とき viewCell が返す値

	TownVisibility();
	TownVisibility( const TownVisibility& ) = delete;
	TownVisibility& operator=( const TownVisibility& ) = delete;

	// 建物の並び（GameWorld::buildings。番号は並びの順）から表を作る（前の表は捨てる）。読み込みの時に 1 回。建物が 0 軒なら作らない
	void build( const std::vector< Building >& buildings );
	// 表（ますごとの見えうる建物・見えうるますのビット）をバイト列にして out に書く（前の中身は捨てる）。表が無ければ空
	// ファイルへ書くのは TownBakeFile。バイト列は、この PC と同じ数の並び（x64）でだけ読める
	void saveTables( std::vector< unsigned char >* out ) const;
	// saveTables で書いたバイト列から表を作る（前の表は捨てる）。建物の足元・遮る物のます・目が来うるますは buildings から作り直す（軽い）
	// buildings は表を作ったときと同じ並びを渡す。ますの数・建物の数・長さがバイト列と合わなければ false で、表は無いまま
	bool loadTables( const std::vector< Building >& buildings, const std::vector< unsigned char >& data );
	bool isReady() const;

	int cellAt( const GameLib::Math::Vector3& eye ) const;														// 目の位置 eye が入っているます（kNoCell なら PVS を使わない＝全部描く）
	bool showsBuilding( int viewCell, int building ) const;														// viewCell から building 番の建物が見えうるか。viewCell が kNoCell なら true
	// viewCell から、足元 position にいる物（ゾンビ）が見えうるか。position のますと、その周り 8 ますのどれかが見えうれば true
	// （体がますの境をまたいで見える分）。viewCell が kNoCell・position が町の外なら true
	bool showsPoint( int viewCell, const GameLib::Math::Vector3& position ) const;
	void cellBounds( int cell, GameLib::Math::Vector3* boundsMin, GameLib::Math::Vector3* boundsMax ) const;	// ますの範囲（上から見た四角。y は 0）。確認用の表示（F3）に使う
	// ますの数（町を覆う 4m のます。表が無ければ 0）・cell が目の来うるます（建物の足元でない）か。確かめ（TownVisibilityCheck）が使う
	int cellCount() const;
	bool isOpenCell( int cell ) const;

	// 3D の線での確かめは DebugMode/TownVisibilityCheck

private:
	// 上から見た建物 1 軒。長方形は建物の向きに回っていて、建物の置き場所からの U・V で持つ
	// U はモデルの +X、V はモデルの +Z が、置いた建物で向く向き。回り方は見た目（StaticMeshInstance・PlacementMatrix）と同じ
	struct Footprint{
		float centerX, centerZ;								// 建物の置き場所
		float cosYaw, sinYaw;								// 建物の向きの cos・sin
		float minU, minV, maxU, maxV;						// 見られる長方形（建物の見た目を囲む箱。壁より少し大きい）
		float blockMinU, blockMinV, blockMaxU, blockMaxV;	// 遮る長方形（体の高さの壁を囲む箱を少し縮めた物）
		float height;										// 一番高い所（y）

		void toLocal( float x, float z, float* u, float* v ) const;	// world の点 (x,z) を、建物の向きの (u,v) へ
		void toWorld( float u, float v, float* x, float* z ) const;	// 建物の向きの (u,v) を、world の点 (x,z) へ
		void blockBounds( GameLib::Math::Vector3* boundsMin, GameLib::Math::Vector3* boundsMax ) const;	// 遮る長方形を囲む、東西南北にそろった箱（y は 0）
	};

	static Footprint footprintOf( const Building& building );	// 建物 1 軒の足元。同じ形を向き 0 で置いたときの箱を、建物の向きに回した長方形にする
	void reset();	// 表を捨てて、無い形（isReady が false）にする

	// ── 表を作る段（build から、この順に呼ぶ。後の段は前の段の結果を読む。loadTables は始めの 2 段だけ呼ぶ）──
	// 建物の足元と遮る長方形を作り、町を囲む範囲を townMin / townMax へ書く。目を使ってよい高さもここで決める
	void buildFootprints( const std::vector< Building >& buildings, GameLib::Math::Vector3* townMin, GameLib::Math::Vector3* townMax );
	void markOpenCells( const GameLib::Math::Vector3& townMin, const GameLib::Math::Vector3& townMax );	// 町をますに分け、目が来うるます（真ん中が建物の壁の長方形に入らない）に印を付ける
	void bakeBuildingVisibility();																		// ますごとの見えうる建物（ますの見る点のどれかから見えうる建物）
	void bakeCellVisibility();																			// ますごとの見えうるます（ゾンビ用。四隅のどれかどうしが見通せれば見えうる）

	bool isBlocked( float ax, float az, float bx, float bz, float minHeight, int ignore ) const;	// 2D の線分 (ax,az)→(bx,bz) を、高さ minHeight 以上の建物（ignore 番を除く）が遮るか
	bool seesBuilding( float x, float z, int building ) const;										// 点 (x,z) から building 番の建物が見えうるか（建物の見る点を向いた辺の上の点のどれかへ、線が遮られずに届くか）
	int cellIndex( int cellX, int cellZ ) const;													// ますの番号（範囲の外なら kNoCell）
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

bool& pvsEnabled();	// 目から見る絵の建物・ゾンビを PVS で間引くか（F6 → Render の PVS。見比べと計測用）。起動中ずっと 1 つ（初期値 true）

#endif
