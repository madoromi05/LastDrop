#include "GameSrc/Hit/HitboxLayout.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshData.h"
#include "GameSrc/Math/BoxFitter.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"
#include "GameLib/Math/Functions.h"
#include <string>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const int kMaxBonesPerPart = 6;
	// 箱の幅の半分の下限（骨の空間の単位）。頂点が平らに並んで幅が 0 になると逆行列が作れないため。
	const float kMinHalfExtent = 0.001f;

	// 部位ごとの「箱を付ける骨」「箱の長い辺の向き（関節 → 関節）」「頂点を集める骨」。名前は頭の共通部分を除いたもの。
	// 箱を付ける骨は、その部位の肉が一緒に動く骨（胴は腰〜胸が曲がるので、真ん中の Spine1 に付ける）。
	// 長い辺は、レスト姿勢で axisFrom の関節から axisTo の関節へ向かう向きに合わせる（骨の向きがモデルによって
	// 腕や脚に沿っていないため。ゾンビの骨はどれも回転 0 で、斜めの腕を斜めでない箱で囲むと立方体に近くなる）。
	// axisTo の骨が無いモデル（つま先の骨が無いゾンビの足など）では、頂点の広がりから向きを決める（BoxFitter）。
	// 1つの部位が箱を複数持ってもよい（同じ部位を表に2行書く）。
	// 腕と脚は関節ごとに1つの箱：肩〜肘・肘〜指先、腰〜膝・膝〜足首。
	// 前腕は手首・指の骨に付いた頂点も前腕の骨の空間で囲む。すねは足を含めない
	// （足は前へ突き出ているので、含めると箱がすねの前へ張り出して太くなる）。
	// 名前の末尾が kWildcard なら、その名前で始まる骨を全部含める（手首と指の骨 16 本を1つずつ書かないため）。
	// 顎・つま先の骨は片方のモデルにだけある。無い骨は飛ばすので、同じ表を主人公にも使える。
	const char kWildcard = '*';
	struct PartDefinition{
		BodyPart part;
		const char* anchorBone;
		const char* axisFrom;                  // 長い辺の始まりの関節
		const char* axisTo;                    // 長い辺の終わりの関節
		const char* bones[ kMaxBonesPerPart ]; // 余りは nullptr
	};
	const PartDefinition kPartDefinitions[] = {
		// 頭は首の骨の頂点を含めない（首は胴の箱の上端と重なるため）。向きだけ首→頭の関節で決める。
		{ BodyPart::Head,          "Head",         "Neck",         "Head",         { "Head", "low_jaw1", "low_jaw2" } },
		{ BodyPart::Torso,         "Spine1",       "Hips",         "Spine2",       { "Hips", "Spine", "Spine1", "Spine2", "LeftShoulder", "RightShoulder" } },
		{ BodyPart::LeftUpperArm,  "LeftArm",      "LeftArm",      "LeftForeArm",  { "LeftArm" } },
		{ BodyPart::RightUpperArm, "RightArm",     "RightArm",     "RightForeArm", { "RightArm" } },
		{ BodyPart::LeftForeArm,   "LeftForeArm",  "LeftForeArm",  "LeftHand",     { "LeftForeArm", "LeftHand*" } },
		{ BodyPart::RightForeArm,  "RightForeArm", "RightForeArm", "RightHand",    { "RightForeArm", "RightHand*" } },
		{ BodyPart::LeftThigh,     "LeftUpLeg",    "LeftUpLeg",    "LeftLeg",      { "LeftUpLeg" } },
		{ BodyPart::RightThigh,    "RightUpLeg",   "RightUpLeg",   "RightLeg",     { "RightUpLeg" } },
		{ BodyPart::LeftLowerLeg,  "LeftLeg",      "LeftLeg",      "LeftFoot",     { "LeftLeg" } },
		{ BodyPart::RightLowerLeg, "RightLeg",     "RightLeg",     "RightFoot",    { "RightLeg" } },
	};

	int findBone( const SkinnedMeshData* data, const char* prefix, const char* name ) {
		return data->findBone( ( std::string( prefix ) + name ).c_str() );
	}

	// 表の名前 name に当てはまる骨を mask で true にする（末尾が kWildcard なら、その前までで始まる骨を全部）。
	void markBones( const SkinnedMeshData* data, const char* prefix, const char* name, std::vector< bool >* mask ) {
		std::string pattern = std::string( prefix ) + name;
		if ( pattern.back() != kWildcard ) {
			int bone = data->findBone( pattern.c_str() );
			if ( bone >= 0 ) {
				( *mask )[ bone ] = true;
			}
			return;
		}
		pattern.pop_back();
		for ( int bone = 0; bone < data->boneNumber(); ++bone ) {
			if ( std::string( data->boneName( bone ) ).compare( 0, pattern.size(), pattern ) == 0 ) {
				( *mask )[ bone ] = true;
			}
		}
	}
}

HitboxLayout::HitboxLayout( const SkinnedMeshData* data, const char* boneNamePrefix ) {
	ASSERT( data && boneNamePrefix && "HitboxLayout : argument is null." );
	for ( const PartDefinition& definition : kPartDefinitions ) {
		int anchor = findBone( data, boneNamePrefix, definition.anchorBone );
		if ( anchor < 0 ) {
			GameLib::cout << "HitboxLayout: bone not found: " << boneNamePrefix << definition.anchorBone << GameLib::endl;
			continue;
		}
		std::vector< bool > mask( data->boneNumber(), false );
		for ( const char* name : definition.bones ) {
			if ( !name ) {
				break;
			}
			markBones( data, boneNamePrefix, name, &mask );
		}
		std::vector< Vector3 > points;
		data->collectBoneSpaceVertices( mask, anchor, &points );
		// 長い辺の向き（箱を付ける骨の空間で）。関節が見つからなければ渡さない（頂点の広がりから決める）。
		int axisFrom = findBone( data, boneNamePrefix, definition.axisFrom );
		int axisTo = findBone( data, boneNamePrefix, definition.axisTo );
		Vector3 longAxis;
		const Vector3* longAxisOrNull = nullptr;
		if ( axisFrom >= 0 && axisTo >= 0 ) {
			longAxis.setSub( data->boneSpaceJointPosition( anchor, axisTo ), data->boneSpaceJointPosition( anchor, axisFrom ) );
			longAxisOrNull = &longAxis;
		}

		Box box;
		if ( !BoxFitter::fit( points, longAxisOrNull, kMinHalfExtent, &box.local ) ) {
			GameLib::cout << "HitboxLayout: no vertex for part of: " << boneNamePrefix << definition.anchorBone << GameLib::endl;
			continue;
		}
		box.part = definition.part;
		box.anchorBone = anchor;
		mBoxes.push_back( box );
	}
}

const std::vector< HitboxLayout::Box >& HitboxLayout::boxes() const {
	return mBoxes;
}
