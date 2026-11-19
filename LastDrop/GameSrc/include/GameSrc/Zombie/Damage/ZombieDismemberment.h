#ifndef INCLUDED_ZOMBIE_DISMEMBERMENT_H
#define INCLUDED_ZOMBIE_DISMEMBERMENT_H

#include "GameSrc/Render/SkinnedMesh/BonePoseModifier.h"
#include "GameSrc/Hit/BodyPart.h"
#include "GameSrc/Zombie/Damage/ZombieSeverLayout.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"
#include <vector>

// ゾンビ1体ぶんの「失った部位」。切った部位を覚え、描く直前に、無くなった骨を切り口の関節の1点へ潰す（BonePoseModifier）
// 骨ごとに自分の原点へ潰すと、手と前腕の間の三角形が線や面として残るので、無くなる骨は全部同じ1点へ集める
// 親の骨と混ざって付いている頂点（肩・肘のまわり）は関節へ向かってすぼまるので、切り口に穴は空かない（蓋は作らない）
// 潰すのは見せる姿勢だけ。部位の当たり判定（HitboxSet）は持ち主（Zombie）が部位ごとに外す
// どの関節で切り、どの骨と部位が無くなるかは ZombieSeverLayout（種類ごとに共有）を借りる
class ZombieDismemberment : public BonePoseModifier{
public:
	explicit ZombieDismemberment( const ZombieSeverLayout* layout );	// layout: 切り方の設計図。所有権は受け取らない。nullptr 不可。この部位欠損より長く生きていること（ZombieModel が持つ）

	bool sever( BodyPart part );		// part を切る。切れない部位（胴）・もう無い部位なら何もせず false。切れたら severedThisHit へ足す
	// 1 発ごとに切れた部位の一覧（持ち主が千切れた部位を飛ばすのに読む）。弾が当たるたびに、持ち主が先に空にする
	void clearSeveredThisHit();
	const std::vector< BodyPart >& severedThisHit() const;
	bool isLost( BodyPart part ) const;	// part がもう無いか（切った部位の先の部位も、無い）
	bool hasLostFooting() const;		// 立っていられない部位（脚）を失ったか
	// 切った部位の骨を、切った順に関節の1点へ潰す（BonePoseModifier）
	// のけぞり（ZombieFlinch）の後に掛ける（のけぞった肩の位置で切り口を作るため）
	virtual void apply( std::vector< GameLib::Math::Matrix34 >* pose ) override;

	static GameLib::Math::Matrix34 collapsedAt( const GameLib::Math::Vector3& point );	// 頂点を全部 point の1点へ集める骨の姿勢（縮尺 0・位置 point）。千切れた部位（SeveredLimb）が、残った体の側を潰すのにも使う
	// 1点へ潰すと切り口が尖る部位（首）は、代わりに point のまわりへ低く丸めて縮める
	// stumpAxisOf: 切り口の軸（手前の骨から関節への向き。長さ 1）。丸めない部位・向きが決まらないときは false
	// roundedAt: 骨の姿勢 bonePose の頂点を、point を中心に軸の向きは短く・横は細く縮める姿勢。axis は長さ 1
	static bool stumpAxisOf( const ZombieSeverLayout::Cut& cut, const std::vector< GameLib::Math::Matrix34 >& pose, GameLib::Math::Vector3* axis );
	static GameLib::Math::Matrix34 roundedAt( const GameLib::Math::Vector3& point, const GameLib::Math::Vector3& axis, const GameLib::Math::Matrix34& bonePose );

private:
	// 骨の姿勢 bonePose の頂点を、point を中心に axis の向きは lengthScale 倍・横は widthScale 倍に縮める姿勢。axis は長さ 1
	static GameLib::Math::Matrix34 scaledAlong( const GameLib::Math::Vector3& point, const GameLib::Math::Vector3& axis,
		float lengthScale, float widthScale, const GameLib::Math::Matrix34& bonePose );

private:
	const ZombieSeverLayout* mLayout; // 借りているだけ
	bool mLost[ static_cast< int >( BodyPart::Count ) ]; // BodyPart の番号で引く
	std::vector< BodyPart > mCutParts; // 切った部位（切った順。前腕の後に上腕を切ったら、肩の点で潰し直す）
	std::vector< BodyPart > mSeveredThisHit; // 最後に clearSeveredThisHit してから切れた部位
};

#endif
