#include "GameSrc/Collision/MeshWallBuilder.h"
#include "GameSrc/Render/StaticMesh/StaticMeshInstance.h"
#include "GameSrc/Render/StaticMesh/StaticMeshData.h"
#include "GameSrc/Collision/MoveHeight.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"
#include "GameLib/Math/Matrix34.h"

#include <vector>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const int kVerticesPerTriangle = 3;
	// 残す側の向き（clipByHeight の keepSign）
	const float kKeepAbove = 1.f;
	const float kKeepBelow = -1.f;
	const float kMinEdgeLength = 0.0001f;		// 真上から見てこれより短い辺（m）からは法線を作らない（まっすぐ立った壁の縦の辺は長さ 0 になる）
	const float kSameDirectionDot = 0.9999f;	// 2本の法線の内積の絶対値がこれ以上なら、同じ向き（または逆向き）とみなして1本にまとめる（約 0.8 度）

	// 凸な多角形 in（inNumber 隅）のうち、高さ height より上（keepSign=kKeepAbove）／下（kKeepBelow）の部分だけを out に書き、隅の数を返す
	// 境目をまたぐ辺は、境目との交点で切る（Sutherland-Hodgman 法）。隅は最大で1つ増える。全部が外なら 0
	int clipByHeight( const Vector3* in, int inNumber, float height, float keepSign, Vector3* out ) {
		int outNumber = 0;
		for ( int i = 0; i < inNumber; ++i ) {
			const Vector3& current = in[ i ];
			const Vector3& next = in[ ( i + 1 ) % inNumber ];
			float currentSide = ( current.y - height ) * keepSign; // 0 以上なら残す側
			float nextSide = ( next.y - height ) * keepSign;
			if ( currentSide >= 0.f ) {
				out[ outNumber ] = current;
				++outNumber;
			}
			if ( ( currentSide >= 0.f ) != ( nextSide >= 0.f ) ) {
				// 境目をまたぐ。currentSide と nextSide は符号が違うので、分母は 0 にならない
				Vector3 edge;
				edge.setSub( next, current );
				out[ outNumber ].setMadd( current, edge, currentSide / ( currentSide - nextSide ) );
				++outNumber;
			}
		}
		return outNumber;
	}

	// かけらの隅から、真上から見た辺の法線を作る（同じ向き・逆向きは1本にまとめる）
	void setEdgeNormals( WallPiece* piece ) {
		piece->edgeNormalNumber = 0;
		for ( int i = 0; i < piece->cornerNumber; ++i ) {
			const Vector3& from = piece->corner[ i ];
			const Vector3& to = piece->corner[ ( i + 1 ) % piece->cornerNumber ];
			Vector3 normal( to.z - from.z, 0.f, from.x - to.x ); // 辺を真上から見て 90 度回した向き
			if ( normal.length() < kMinEdgeLength ) {
				continue;
			}
			normal.normalize();
			bool isNew = true;
			for ( int j = 0; j < piece->edgeNormalNumber; ++j ) {
				if ( Math::abs( normal.dot( piece->edgeNormal[ j ] ) ) >= kSameDirectionDot ) {
					isNew = false;
					break;
				}
			}
			if ( isNew ) {
				piece->edgeNormal[ piece->edgeNormalNumber ] = normal;
				++piece->edgeNormalNumber;
			}
		}
	}

	// ワールドに置いた三角形1枚を体の高さの範囲で切り取り、かけらにする。範囲に入らない・真上から見て点にしかならないなら false
	bool makePiece( const Vector3 triangle[ kVerticesPerTriangle ], WallPiece* piece ) {
		Vector3 aboveStep[ WallPiece::kMaxCorners ];
		int number = clipByHeight( triangle, kVerticesPerTriangle, MoveHeight::kStepOver, kKeepAbove, aboveStep );
		piece->cornerNumber = clipByHeight( aboveStep, number, MoveHeight::kBodyTop, kKeepBelow, piece->corner );
		if ( piece->cornerNumber == 0 ) {
			return false;
		}
		setEdgeNormals( piece );
		if ( piece->edgeNormalNumber == 0 ) {
			return false;
		}
		piece->boundsMin = piece->corner[ 0 ];
		piece->boundsMax = piece->corner[ 0 ];
		for ( int i = 1; i < piece->cornerNumber; ++i ) {
			piece->boundsMin.min( piece->corner[ i ] );
			piece->boundsMax.max( piece->corner[ i ] );
		}
		return true;
	}
}

WallMesh MeshWallBuilder::make( const StaticMeshInstance& mesh ) {
	const Matrix34& world = mesh.worldMatrix();

	std::vector< Vector3 > localCorners; // 三角形の隅を3つずつ並べたもの（モデルのローカル）
	mesh.data()->appendTriangles( &localCorners );

	WallMesh walls;
	for ( size_t i = 0; i + kVerticesPerTriangle <= localCorners.size(); i += kVerticesPerTriangle ) {
		Vector3 triangle[ kVerticesPerTriangle ];
		for ( int j = 0; j < kVerticesPerTriangle; ++j ) {
			world.mul( &triangle[ j ], localCorners[ i + j ] );
		}
		WallPiece piece;
		if ( makePiece( triangle, &piece ) ) {
			walls.add( piece );
		}
	}
	return walls;
}
